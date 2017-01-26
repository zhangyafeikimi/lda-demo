// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//

#include "lda/rand.h"
#include "lda/sampler.h"
#include "lda/x.h"

// u is in [0, 1)
static int SampleCDF(const std::vector<double>& cdf, double u) {
  int size = (int)cdf.size();
  const double sample = u * cdf[size - 1];
  if (size < 128) {
    // brute force search
    int i;
    for (i = 0; i < size; i++) {
      if (cdf[i] >= sample) {
        break;
      }
    }
    return i;
  } else  {
    // binary search
    int count = size, half_count;
    int first = 0, middle;
    while (count > 0) {
      half_count = count / 2;
      middle = first + half_count;
      if (sample <= cdf[middle]) {
        count = half_count;
      } else {
        first = middle + 1;
        count -= (half_count + 1);
      }
    }
    return first;
  }
}

static inline int SampleCDF(const std::vector<double>& cdf) {
  return SampleCDF(cdf, Rand::Double01());
}

/************************************************************************/
/* SamplerBase */
/************************************************************************/
int SamplerBase::LoadModel(const std::string& prefix) {
  Log("Loading model.\n");
  if (LoadMVK(prefix + "-stat") != 0) {
    return -1;
  }
  if (LoadHPAlpha(prefix + "-alpha") != 0) {
    return -2;
  }
  if (LoadHPBeta(prefix + "-beta") != 0) {
    return -3;
  }
  if (LoadTopicsCount(prefix + "-topic-count") != 0) {
    return -4;
  }
  if (LoadWordsTopicsCount(prefix + "-word-topic-count") != 0) {
    return -5;
  }
  Log("Done.\n");
  return 0;
}

void SamplerBase::SaveModel(const std::string& prefix) const {
  Log("Saving model.\n");
  SaveMVK(prefix + "-stat");
  {
    Array2D<double> theta;
    CollectTheta(&theta);
    SaveArray2D(prefix + "-doc-topic-prob", theta);
  }
  {
    Array2D<double> phi;
    CollectPhi(&phi);
    SaveArray2D(prefix + "-topic-word-prob", phi);
  }
  SaveTopicsCount(prefix + "-topic-count");
  SaveWordsTopicsCount(prefix + "-word-topic-count");
  SaveHPAlpha(prefix + "-alpha");
  SaveHPBeta(prefix + "-beta");
  Log("Done.\n");
}

int SamplerBase::InitializeSampler() {
  mode_ = kSampleMode;

  if (hp_sum_alpha_ <= 0.0) {
    const double avg_doc_len = (double)words_.size() / docs_.size();
    hp_alpha_.resize(K_, avg_doc_len / K_);
    hp_sum_alpha_ = avg_doc_len;
  } else {
    hp_alpha_.resize(K_, hp_sum_alpha_);
    hp_sum_alpha_ = hp_sum_alpha_ * K_;
  }

  if (hp_beta_ <= 0.0) {
    hp_beta_ = 0.1;
  }
  hp_sum_beta_ = V_ * hp_beta_;

  if (hp_opt_) {
    if (hp_opt_interval_ == 0) {
      hp_opt_interval_ = 5;
    }
    if (hp_opt_alpha_scale_ == 0.0) {
      hp_opt_alpha_scale_ = 100000.0;
    }
    if (hp_opt_alpha_iteration_ == 0) {
      hp_opt_alpha_iteration_ = 2;
    }
    if (hp_opt_beta_iteration_ == 0) {
      hp_opt_beta_iteration_ = 200;
    }
  }

  if (total_iteration_ == 0) {
    total_iteration_ = 200;
  }

  topics_count_.InitDense(K_);
  docs_topics_count_.Init(M_, K_, storage_type_);
  words_topics_count_.Init(V_, K_, storage_type_);

  // random initialize topics
  for (int m = 0; m < M_; m++) {
    const Doc& doc = docs_[m];
    Word* word = &words_[doc.index];
    IntTable& doc_topics_count = docs_topics_count_[m];
    for (int n = 0; n < doc.N; n++, word++) {
      const int v = word->v;
      const int new_topic = (int)Rand::UInt(K_);
      word->k = new_topic;
      ++topics_count_[new_topic];
      ++doc_topics_count[new_topic];
      ++words_topics_count_[v][new_topic];
    }
  }

  // const double llh = LogLikelihood();
  // Log("LogLikelihood(total/word)=%lg/%lg\n", llh, llh / words_.size());
  return 0;
}

double SamplerBase::LogLikelihood() const {
  double sum = 0.0;
  Array2D<double> phi;

  phi.Init(K_, V_);
  CollectPhi(&phi);

  for (int m = 0; m < M_; m++) {
    const Doc& doc = docs_[m];
    const Word* word = &words_[doc.index];
    const IntTable& doc_topics_count = docs_topics_count_[m];
    for (int n = 0; n < doc.N; n++, word++) {
      const int v = word->v;
      double word_sum = 0.0;
      for (int k = 0; k < K_; k++) {
        word_sum += (doc_topics_count[k] + hp_alpha_[k]) * phi[k][v];
      }
      word_sum /= (doc.N + hp_sum_alpha_);
      sum += log(word_sum);
    }
  }
  return sum;
}

int SamplerBase::Train() {
  time_t begin, end;
  time(&begin);

  Log("Training.\n");

  if (InitializeSampler() != 0) {
    return -1;
  }

  for (iteration_ = 1; iteration_ <= total_iteration_; iteration_++) {
    time_t iter_begin, iter_end;
    int cpu;
    uint64_t mem, vmem;

    (void)GetCPUUsage();
    time(&iter_begin);
    Log("Iteration %d started.\n", iteration_);
    PreSampleCorpus();
    SampleCorpus();
    PostSampleCorpus();
    time(&iter_end);
    cpu = GetCPUUsage();
    (void)GetMemoryUsage(&mem, &vmem);
    Log("Iteration %d ended, cost %d seconds, "
        "%d%% CPU, %.0lf bytes memory.\n",
        iteration_, (int)(iter_end - iter_begin), cpu, (double)mem);

    if (iteration_ > burnin_iteration_
        && iteration_ % log_likelihood_interval_ == 0) {
      time(&iter_begin);
      Log("Calculating LogLikelihood.\n");
      const double llh = LogLikelihood();
      time(&iter_end);
      Log("LogLikelihood(total/word)=%lg/%lg, cost %d seconds.\n",
          llh, llh / words_.size(), (int)(iter_end - iter_begin));
    }
  }

  time(&end);
  Log("Training completed, cost %d seconds.\n", (int)(end - begin));
  return 0;
}

void SamplerBase::PreSampleCorpus() {
  HPOpt_Initialize();
}

void SamplerBase::PostSampleCorpus() {
  HPOpt_Optimize();
}

void SamplerBase::SampleCorpus() {
  for (int m = 0; m < M_; m++) {
    PreSampleDocument(m);
    SampleDocument(m);
    PostSampleDocument(m);
  }
}

void SamplerBase::PreSampleDocument(int m) {}

void SamplerBase::PostSampleDocument(int m) {
  HPOpt_PostSampleDocument(m);
}

void SamplerBase::SampleDocument(int m) {
  const Doc& doc = docs_[m];
  Word* word = &words_[doc.index];
  IntTable& doc_topics_count = docs_topics_count_[m];
  SampleDocument(word, doc.N, &doc_topics_count);
}

void SamplerBase::SampleDocument(Word* word, int doc_length,
                                 IntTable* doc_topics_count) {
  Error("SampleDocument is not implemented.\n");
}

void SamplerBase::HPOpt_Initialize() {
  if (!HPOpt_Enabled()) {
    return;
  }

  Log("Hyper optimization will be carried out in this iteration.\n");
  docs_topic_count_hist_.clear();
  docs_topic_count_hist_.resize(K_);
  doc_len_hist_.clear();
  word_topic_count_hist_.clear();
  topic_len_hist_.clear();
}

void SamplerBase::HPOpt_Optimize() {
  if (!HPOpt_Enabled()) {
    return;
  }

  if (hp_opt_alpha_iteration_ > 0) {
    Log("Hyper optimizing alpha.\n");
    HPOpt_OptimizeAlpha();
  }
  if (hp_opt_beta_iteration_ > 0) {
    Log("Hyper optimizing beta.\n");
    HPOpt_PrepareOptimizeBeta();
    HPOpt_OptimizeBeta();
  }
}

void SamplerBase::HPOpt_OptimizeAlpha() {
  for (int i = 0; i < hp_opt_alpha_iteration_; i++) {
    double denom = 0.0;
    double diff_digamma = 0.0;
    for (int j = 1, size = (int)doc_len_hist_.size(); j < size; j++) {
      diff_digamma += 1.0 / (j - 1 + hp_sum_alpha_);
      denom += doc_len_hist_[j] * diff_digamma;
    }
    denom -= 1.0 / hp_opt_alpha_scale_;

    hp_sum_alpha_ = 0.0;
    for (int k = 0, size = (int)docs_topic_count_hist_.size();
         k < size; k++) {
      double num = 0.0;
      double alpha_k = hp_alpha_[k];
      const std::vector<int>& docs_topic_k_count_hist =
        docs_topic_count_hist_[k];
      diff_digamma = 0.0;
      for (int j = 1, size = (int)docs_topic_count_hist_[k].size();
           j < size; j++) {
        diff_digamma += 1.0 / (j - 1 + alpha_k);
        num += docs_topic_k_count_hist[j] * diff_digamma;
      }
      alpha_k = (alpha_k * num + hp_opt_alpha_shape_) / denom;
      hp_alpha_[k] = alpha_k;
      hp_sum_alpha_ += alpha_k;
    }
  }
}

void SamplerBase::HPOpt_PrepareOptimizeBeta() {
  for (int m = 0; m < M_; m++) {
    const IntTable& doc_topics_count = docs_topics_count_[m];
    for (int k = 0; k < K_; k++) {
      const int count = doc_topics_count[k];
      if (count == 0) {
        continue;
      }
      if ((int)word_topic_count_hist_.size() <= count) {
        word_topic_count_hist_.resize(count + 1);
      }
      word_topic_count_hist_[count]++;
    }
  }

  for (int k = 0; k < K_; k++) {
    const int count = topics_count_[k];
    if (count == 0) {
      continue;
    }
    if ((int)topic_len_hist_.size() <= count) {
      topic_len_hist_.resize(count + 1);
    }
    topic_len_hist_[count]++;
  }
}

void SamplerBase::HPOpt_OptimizeBeta() {
  for (int i = 0; i < hp_opt_beta_iteration_; i++) {
    double num = 0.0;
    double diff_digamma = 0.0;
    for (int j = 1, size = (int)word_topic_count_hist_.size();
         j < size; j++) {
      diff_digamma += 1.0 / (j - 1 + hp_beta_);
      num += diff_digamma * word_topic_count_hist_[j];
    }

    double denom = 0.0;
    diff_digamma = 0.0;
    for (int j = 1, size = (int)topic_len_hist_.size(); j < size; j++) {
      diff_digamma += 1.0 / (j - 1 + hp_sum_beta_);
      denom += diff_digamma * topic_len_hist_[j];
    }
    hp_sum_beta_ = hp_beta_ * num / denom;
    hp_beta_ = hp_sum_beta_ / V_;
  }
}

void SamplerBase::HPOpt_PostSampleDocument(int m) {
  if (!HPOpt_Enabled()) {
    return;
  }

  if (hp_opt_alpha_iteration_ > 0) {
    const Doc& doc = docs_[m];
    const IntTable& doc_topics_count = docs_topics_count_[m];
    for (int k = 0; k < K_; k++) {
      const int count = doc_topics_count[k];
      if (count == 0) {
        continue;
      }
      std::vector<int>& docs_topic_k_count_hist = docs_topic_count_hist_[k];
      if ((int)docs_topic_k_count_hist.size() <= count) {
        docs_topic_k_count_hist.resize(count + 1);
      }
      docs_topic_k_count_hist[count]++;
    }

    if (doc.N) {
      if ((int)doc_len_hist_.size() <= doc.N) {
        doc_len_hist_.resize(doc.N + 1);
      }
      doc_len_hist_[doc.N]++;
    }
  }
}

int SamplerBase::InitializeInfer() {
  if (K_ == 0) {
    Error("Please load a valid model by \"LoadModel\".\n");
    return -1;
  }

  mode_ = kInferMode;

  if (total_iteration_ == 0) {
    total_iteration_ = 20;
  }

  return 0;
}

void SamplerBase::InferDocument(Word* word, int doc_length,
                                IntTable* doc_topics_count) {
  for (int i = 0; i < doc_length; i++) {
    Word& w = word[i];
    w.k = (int)Rand::UInt(K_);
    ++(*doc_topics_count)[w.k];
  }

  for (int i = 1; i <= total_iteration_; i++) {
    SampleDocument(word, doc_length, doc_topics_count);
  }
}

void SamplerBase::InferDocument(Word* word, int doc_length,
                                int* most_prob_topic) {
  IntTable doc_topics_count;
  doc_topics_count.InitHash();

  InferDocument(word, doc_length, &doc_topics_count);

  IntTable::const_iterator first = doc_topics_count.begin();
  IntTable::const_iterator last = doc_topics_count.end();
  int max_k = -1;
  int max_count = 0;
  for (; first != last; ++first) {
    const int count = first.count();
    if (max_count < count) {
      max_count = count;
      max_k = first.id();
    }
  }
  *most_prob_topic = max_k;
}

void SamplerBase::InferDocument(int* word_ids, int doc_length,
                                IntTable* doc_topics_count) {
  std::vector<Word> doc;
  for (int i = 0; i < doc_length; i++) {
    Word word;
    word.v = word_ids[i];
    doc.push_back(word);
  }
  InferDocument(&doc[0], doc_length, doc_topics_count);
}

void SamplerBase::InferDocument(int* word_ids, int doc_length,
                                int* most_prob_topic) {
  std::vector<Word> doc;
  for (int i = 0; i < doc_length; i++) {
    Word word;
    word.v = word_ids[i];
    doc.push_back(word);
  }
  InferDocument(&doc[0], doc_length, most_prob_topic);
}

/************************************************************************/
/* GibbsSampler */
/************************************************************************/
int GibbsSampler::InitializeSampler() {
  if (SamplerBase::InitializeSampler() != 0) {
    return -1;
  }

  word_topic_cdf_.resize(K_);
  return 0;
}

int GibbsSampler::InitializeInfer() {
  if (SamplerBase::InitializeInfer() != 0) {
    return -1;
  }

  word_topic_cdf_.resize(K_);
  return 0;
}

void GibbsSampler::SampleDocument(Word* word, int doc_length,
                                  IntTable* doc_topics_count) {
  for (int n = 0; n < doc_length; n++, word++) {
    const int v = word->v;
    const int old_k = word->k;
    IntTable& word_topics_count = words_topics_count_[v];
    int k, new_k;

    if (mode_ ==  kSampleMode) {
      --topics_count_[old_k];
      --word_topics_count[old_k];
    }
    --(*doc_topics_count)[old_k];

    word_topic_cdf_[0] = 0.0;
    for (k = 0; k < K_ - 1; k++) {
      word_topic_cdf_[k] += (word_topics_count[k] + hp_beta_)
                            / (topics_count_[k] + hp_sum_beta_)
                            * ((*doc_topics_count)[k] + hp_alpha_[k]);
      word_topic_cdf_[k + 1] = word_topic_cdf_[k];
    }
    word_topic_cdf_[k] += (word_topics_count[k] + hp_beta_)
                          / (topics_count_[k] + hp_sum_beta_)
                          * ((*doc_topics_count)[k] + hp_alpha_[k]);

    new_k = SampleCDF(word_topic_cdf_);
    if (mode_ ==  kSampleMode) {
      ++topics_count_[new_k];
      ++word_topics_count[new_k];
    }
    ++(*doc_topics_count)[new_k];
    word->k = new_k;
  }
}

/************************************************************************/
/* SparseLDASampler */
/************************************************************************/
int SparseLDASampler::InitializeSampler() {
  if (SamplerBase::InitializeSampler() != 0) {
    return -1;
  }

  smooth_pdf_.resize(K_);
  doc_pdf_.resize(K_);
  word_pdf_.resize(K_);
  cache_.resize(K_);
  PrepareSmoothBucket();
  return 0;
}

void SparseLDASampler::PostSampleCorpus() {
  SamplerBase::PostSampleCorpus();

  if (HPOpt_Enabled()) {
    PrepareSmoothBucket();
  }
}

void SparseLDASampler::PostSampleDocument(int m) {
  const IntTable& doc_topics_count = docs_topics_count_[m];
  IntTable::const_iterator first = doc_topics_count.begin();
  IntTable::const_iterator last = doc_topics_count.end();
  for (; first != last; ++first) {
    const int k = first.id();
    cache_[k] = hp_alpha_[k] / (topics_count_[k] + hp_sum_beta_);
  }

  SamplerBase::HPOpt_PostSampleDocument(m);
}

void SparseLDASampler::SampleDocument(int m) {
  PrepareDocBucket(m);

  const Doc& doc = docs_[m];
  Word* word = &words_[doc.index];

  for (int n = 0; n < doc.N; n++, word++) {
    const int v = word->v;
    const int old_k = word->k;
    RemoveOrAddWordTopic(m, v, old_k, 1);
    PrepareWordBucket(v);
    const int new_k = SampleDocumentWord(m, v);
    RemoveOrAddWordTopic(m, v, new_k, 0);
    word->k = new_k;
  }
}

void SparseLDASampler::RemoveOrAddWordTopic(int m, int v, int k, int remove) {
  IntTable& doc_topics_count = docs_topics_count_[m];
  IntTable& word_topics_count = words_topics_count_[v];
  double& smooth_bucket_k = smooth_pdf_[k];
  double& doc_bucket_k = doc_pdf_[k];
  const double hp_alpha_k = hp_alpha_[k];
  int doc_topic_count;
  int topic_count;

  smooth_sum_ -= smooth_bucket_k;
  doc_sum_ -= doc_bucket_k;

  if (remove) {
    topic_count = --topics_count_[k];
    --word_topics_count[k];
    doc_topic_count = --doc_topics_count[k];
  } else {
    topic_count = ++topics_count_[k];
    ++word_topics_count[k];
    doc_topic_count = ++doc_topics_count[k];
  }

  smooth_bucket_k = hp_alpha_k * hp_beta_ / (topic_count + hp_sum_beta_);
  doc_bucket_k = doc_topic_count * hp_beta_ / (topic_count + hp_sum_beta_);
  smooth_sum_ += smooth_bucket_k;
  doc_sum_ += doc_bucket_k;
  cache_[k] = (doc_topic_count + hp_alpha_k) / (topic_count + hp_sum_beta_);
}

int SparseLDASampler::SampleDocumentWord(int m, int v) {
  const double sum = smooth_sum_ + doc_sum_ + word_sum_;
  double sample = Rand::Double01() * sum;
  int new_k = -1;

  if (sample < word_sum_) {
    const IntTable& word_topics_count = words_topics_count_[v];
    IntTable::const_iterator first = word_topics_count.begin();
    IntTable::const_iterator last = word_topics_count.end();
    for (; first != last; ++first) {
      const int k = first.id();
      sample -= word_pdf_[k];
      if (sample <= 0.0) {
        break;
      }
    }
    new_k = first.id();
  } else {
    sample -= word_sum_;
    if (sample < doc_sum_) {
      const IntTable& doc_topics_count = docs_topics_count_[m];
      IntTable::const_iterator first = doc_topics_count.begin();
      IntTable::const_iterator last = doc_topics_count.end();
      for (; first != last; ++first) {
        const int k = first.id();
        sample -= doc_pdf_[k];
        if (sample <= 0.0) {
          break;
        }
      }
      new_k = first.id();
    } else {
      sample -= doc_sum_;
      int k;
      for (k = 0; k < K_; k++) {
        sample -= smooth_pdf_[k];
        if (sample <= 0.0) {
          break;
        }
      }
      new_k = k;
    }
  }

  return new_k;
}

void SparseLDASampler::PrepareSmoothBucket() {
  smooth_sum_ = 0.0;
  for (int k = 0; k < K_; k++) {
    const double tmp = hp_alpha_[k] / (topics_count_[k] + hp_sum_beta_);
    const double pdf = tmp * hp_beta_;
    smooth_pdf_[k] = pdf;
    smooth_sum_ += pdf;
    cache_[k] = tmp;
  }
}

void SparseLDASampler::PrepareDocBucket(int m) {
  doc_sum_ = 0.0;
  doc_pdf_.assign(K_, 0);
  const IntTable& doc_topics_count = docs_topics_count_[m];
  IntTable::const_iterator first = doc_topics_count.begin();
  IntTable::const_iterator last = doc_topics_count.end();
  for (; first != last; ++first) {
    const int k = first.id();
    const double tmp = topics_count_[k] + hp_sum_beta_;
    const double pdf = first.count() * hp_beta_ / tmp;
    doc_pdf_[k] = pdf;
    doc_sum_ += pdf;
    cache_[k] = (first.count() + hp_alpha_[k]) / tmp;
  }
}

void SparseLDASampler::PrepareWordBucket(int v) {
  word_sum_ = 0.0;
  word_pdf_.assign(K_, 0);
  const IntTable& word_topics_count = words_topics_count_[v];
  IntTable::const_iterator first = word_topics_count.begin();
  IntTable::const_iterator last = word_topics_count.end();
  for (; first != last; ++first) {
    const int k = first.id();
    const double pdf = first.count() * cache_[k];
    word_pdf_[k] = pdf;
    word_sum_ += pdf;
  }
}

/************************************************************************/
/* AliasLDASampler */
/************************************************************************/
int AliasLDASampler::InitializeSampler() {
  if (SamplerBase::InitializeSampler() != 0) {
    return -1;
  }

  p_pdf_.resize(K_);
  q_sums_.resize(V_);
  q_samples_.resize(V_);
  q_pdf_.resize(K_);
  if (mh_step_ == 0) {
    mh_step_ = 8;
  }
  return 0;
}

int AliasLDASampler::InitializeInfer() {
  if (SamplerBase::InitializeInfer() != 0) {
    return -1;
  }

  p_pdf_.resize(K_);
  q_sums_.resize(V_);
  q_samples_.resize(V_);
  q_pdf_.resize(K_);
  if (mh_step_ == 0) {
    mh_step_ = 8;
  }
  return 0;
}

void AliasLDASampler::SampleDocument(Word* word, int doc_length,
                                     IntTable* doc_topics_count) {
  int s, t;
  // Macro SMOLA_ALIAS_LDA implements the pure algorithm from
  // Alex Smola's paper. Otherwise,
  // some of my changes are made to make it easier and faster.
  // #define SMOLA_ALIAS_LDA
#if defined SMOLA_ALIAS_LDA
  int N_s, N_vs, N_ms, N_t, N_vt, N_mt;
#endif
  int N_s_prime, N_vs_prime, N_ms_prime;
  int N_t_prime, N_vt_prime, N_mt_prime;
  double hp_alpha_s, hp_alpha_t;
  double accept_rate;
  double temp_s, temp_t;
  double p_sum, q_sum;
  double sample;

  for (int n = 0; n < doc_length; n++, word++) {
    const int v = word->v;
    IntTable& word_topics_count = words_topics_count_[v];
    const int old_k = word->k;
    s = old_k;

    if (mode_ == kSampleMode) {
      N_s_prime = --topics_count_[s];
      N_vs_prime = --word_topics_count[s];
#if defined SMOLA_ALIAS_LDA
      N_s = N_s_prime + 1;
      N_vs = N_vs_prime + 1;
#endif
    } else {
      N_s_prime = topics_count_[s];
      N_vs_prime = word_topics_count[s];
#if defined SMOLA_ALIAS_LDA
      N_s = N_s_prime;
      N_vs = N_vs_prime;
#endif
    }
    N_ms_prime = --(*doc_topics_count)[s];
#if defined SMOLA_ALIAS_LDA
    N_ms = N_ms_prime + 1;
#endif

    temp_s = (N_vs_prime + hp_beta_) / (N_s_prime + hp_sum_beta_);
    hp_alpha_s = hp_alpha_[s];

    // construct p: first part of the proposal
    p_sum = 0.0;
    IntTable::const_iterator first = doc_topics_count->begin();
    IntTable::const_iterator last = doc_topics_count->end();
    for (; first != last; ++first) {
      const int k = first.id();
      double& pdf = p_pdf_[k];
      pdf = first.count()
            * (word_topics_count[k] + hp_beta_)
            / (topics_count_[k] + hp_sum_beta_);
      p_sum  += pdf;
    }

    // prepare samples from q: second part of the proposal
    // TODO(yafei) unchanged during inference
    q_sum = 0.0;
    std::vector<int>& word_v_q_samples = q_samples_[v];
    const int word_v_q_samples_size = (int)word_v_q_samples.size();
    if (word_v_q_samples_size < mh_step_) {
      // construct q
      if (storage_type_ == kHashTable || storage_type_ == kSparseTable) {
        q_pdf_.assign(K_, 0.0);
        IntTable::const_iterator first = word_topics_count.begin();
        IntTable::const_iterator last = word_topics_count.end();
        for (; first != last; ++first) {
          const int k = first.id();
          double& pdf = q_pdf_[k];
          pdf = hp_alpha_[k]
                * (word_topics_count[k] + hp_beta_)
                / (topics_count_[k] + hp_sum_beta_);
          q_sum += pdf;
        }
        for (int k = 0; k < K_; k++) {
          double& pdf = q_pdf_[k];
          if (pdf == 0.0) {
            pdf = hp_alpha_[k]
                  * hp_beta_
                  / (topics_count_[k] + hp_sum_beta_);
            q_sum += pdf;
          }
        }
      } else {
        for (int k = 0; k < K_; k++) {
          double& pdf = q_pdf_[k];
          pdf = hp_alpha_[k]
                * (word_topics_count[k] + hp_beta_)
                / (topics_count_[k] + hp_sum_beta_);
          q_sum += pdf;
        }
      }
#if defined SMOLA_ALIAS_LDA
      double& pdf_s = q_pdf_[s];
      q_sum -= pdf_s;
      pdf_s = hp_alpha_s
              * (N_vs + hp_beta_)
              / (N_s + hp_sum_beta_);
      q_sum += pdf_s;
#endif
      q_sums_[v] = q_sum;
      q_alias_table_.Build(q_pdf_, q_sum);

      const int cached_samples = K_ * mh_step_;
      word_v_q_samples.reserve(cached_samples);
      for (int i = word_v_q_samples_size; i < cached_samples; i++) {
        word_v_q_samples.push_back(q_alias_table_.Sample());
      }
    } else {
      q_sum = q_sums_[v];
    }

    for (int step = 0; step < mh_step_; step++) {
      sample = Rand::Double01() * (p_sum + q_sum);
      if (sample < p_sum) {
        // sample from p
        IntTable::const_iterator first = doc_topics_count->begin();
        IntTable::const_iterator last = doc_topics_count->end();
        for (; first != last; ++first) {
          const int k = first.id();
          sample -= p_pdf_[k];
          if (sample <= 0.0) {
            break;
          }
        }
        t = first.id();
      } else {
        // sample from q
        t = word_v_q_samples.back();
        word_v_q_samples.pop_back();
      }

      if (s != t) {
        N_t_prime = topics_count_[t];
        N_vt_prime = word_topics_count[t];
        N_mt_prime = (*doc_topics_count)[t];
#if defined SMOLA_ALIAS_LDA
        N_t = N_t_prime;
        N_vt = N_vt_prime;
        N_mt = N_mt_prime;
        if (old_k == t) {
          if (mode_ == kSampleMode) {
            N_t++;
            N_vt++;
          }
          N_mt++;
        }
#endif
        temp_t = (N_vt_prime + hp_beta_) / (N_t_prime + hp_sum_beta_);
        hp_alpha_t = hp_alpha_[t];
#if defined SMOLA_ALIAS_LDA
        accept_rate =
          (N_mt_prime + hp_alpha_t) / (N_ms_prime + hp_alpha_s)
          * temp_t / temp_s
          * (N_ms_prime * temp_s
             + hp_alpha_s * (N_vs + hp_beta_) / (N_s + hp_sum_beta_))
          / (N_mt_prime * temp_t
             + hp_alpha_t* (N_vt + hp_beta_) / (N_t + hp_sum_beta_));
#else
        accept_rate =
          (N_mt_prime + hp_alpha_t) / (N_ms_prime + hp_alpha_s)
          * temp_t / temp_s
          * (N_ms_prime + hp_alpha_s) * temp_s
          / (N_mt_prime + hp_alpha_t) / temp_t;
#endif
        assert(accept_rate >= 0.0);
        if (/*accept_rate >= 1.0 || */Rand::Double01() < accept_rate) {
          word->k = t;
          s = t;
#if defined SMOLA_ALIAS_LDA
          N_s = N_t;
          N_vs = N_vt;
          N_ms = N_mt;
#endif
          N_s_prime = N_t_prime;
          N_vs_prime = N_vt_prime;
          N_ms_prime = N_mt_prime;
          temp_s = temp_t;
          hp_alpha_s = hp_alpha_t;
        }
      }
    }

    if (mode_ == kSampleMode) {
      ++topics_count_[s];
      ++word_topics_count[s];
    }
    ++(*doc_topics_count)[s];
  }
}

/************************************************************************/
/* LightLDASampler */
/************************************************************************/
int LightLDASampler::InitializeSampler() {
  if (SamplerBase::InitializeSampler() != 0) {
    return -1;
  }

  hp_alpha_alias_table_.Build(hp_alpha_, hp_sum_alpha_);
  word_topics_pdf_.resize(K_);
  words_topic_samples_.resize(V_);
  if (mh_step_ == 0) {
    mh_step_ = 8;
  }
  return 0;
}

int LightLDASampler::InitializeInfer() {
  if (SamplerBase::InitializeInfer() != 0) {
    return -1;
  }

  hp_alpha_alias_table_.Build(hp_alpha_, hp_sum_alpha_);
  word_topics_pdf_.resize(K_);
  words_topic_samples_.resize(V_);
  if (mh_step_ == 0) {
    mh_step_ = 8;
  }
  return 0;
}

void LightLDASampler::PostSampleCorpus() {
  SamplerBase::PostSampleCorpus();

  if (HPOpt_Enabled()) {
    if (hp_opt_alpha_iteration_ > 0) {
      hp_alpha_alias_table_.Build(hp_alpha_, hp_sum_alpha_);
    }
  }
}

void LightLDASampler::SampleDocument(Word* word, int doc_length,
                                     IntTable* doc_topics_count) {
  int s, t;
  int N_s, N_vs, N_ms, N_t, N_vt, N_mt;
  int N_s_prime, N_vs_prime, N_ms_prime;
  int N_t_prime, N_vt_prime, N_mt_prime;
  double hp_alpha_s, hp_alpha_t;
  double accept_rate;
  Word* word0 = word;

  for (int n = 0; n < doc_length; n++, word++) {
    const int v = word->v;
    IntTable& word_topics_count = words_topics_count_[v];
    const int old_k = word->k;
    s = old_k;

    N_s_prime = N_s = topics_count_[s];
    N_vs_prime = N_vs = word_topics_count[s];
    N_ms_prime = N_ms = (*doc_topics_count)[s];
    if (old_k == s) {
      if (mode_ == kSampleMode) {
        N_s_prime--;
        N_vs_prime--;
      }
      N_ms_prime--;
    }
    hp_alpha_s = hp_alpha_[s];

    for (int step = 0; step < mh_step_; step++) {
      if (enable_word_proposal_) {
        // sample new topic from word proposal
        t = SampleWithWord(v);

        if (s != t) {
          // calculate accept rate from topic s to topic t:
          // (N^{'}_{mt} + \alpha_t)(N^{'}_{vt} + \beta)
          // -----------------------------------------------
          // (N^{'}_{ms} + \alpha_s)(N^{'}_{vs} + \beta)
          // *
          // (N^{'}_s + \sum\beta)
          // -----------------------
          // (N^{'}_t + \sum\beta)
          // *
          // (N_{vs} + \beta)(N_t + \sum\beta)
          // ---------------------------------
          // (N_{vt} + \beta)(N_s + \sum\beta)
          N_t_prime = N_t = topics_count_[t];
          N_vt_prime = N_vt = word_topics_count[t];
          N_mt_prime = N_mt = (*doc_topics_count)[t];
          if (old_k == t) {
            if (mode_ == kSampleMode) {
              N_t_prime--;
              N_vt_prime--;
            }
            N_mt_prime--;
          }
          hp_alpha_t = hp_alpha_[t];

          accept_rate =
            (N_mt_prime + hp_alpha_t) / (N_ms_prime + hp_alpha_s)
            * (N_vt_prime + hp_beta_) / (N_vs_prime + hp_beta_)
            * (N_s_prime + hp_sum_beta_) / (N_t_prime + hp_sum_beta_)
            * (N_vs + hp_beta_) / (N_vt + hp_beta_)
            * (N_t + hp_sum_beta_) / (N_s + hp_sum_beta_);

          assert(accept_rate >= 0.0);
          if (/*accept_rate >= 1.0 || */Rand::Double01() < accept_rate) {
            word->k = t;
            s = t;
            N_s = N_t;
            N_vs = N_vt;
            N_ms = N_mt;
            N_s_prime = N_t_prime;
            N_vs_prime = N_vt_prime;
            N_ms_prime = N_mt_prime;
            hp_alpha_s = hp_alpha_t;
          }
        }
      }

      if (enable_doc_proposal_) {
        t = SampleWithDoc(word0, doc_length, v);
        if (s != t) {
          // calculate accept rate from topic s to topic t:
          // (N^{'}_{mt} + \alpha_t)(N^{'}_{vt} + \beta)
          // -----------------------------------------------
          // (N^{'}_{ms} + \alpha_s)(N^{'}_{vs} + \beta)
          // *
          // (N^{'}_s + \sum\beta)
          // -----------------------
          // (N^{'}_t + \sum\beta)
          // *
          // (N_{ms} + \alpha_s)
          // -------------------
          // (N_{mt} + \alpha_t)
          N_t_prime = N_t = topics_count_[t];
          N_vt_prime = N_vt = word_topics_count[t];
          N_mt_prime = N_mt = (*doc_topics_count)[t];
          if (old_k == t) {
            if (mode_ == kSampleMode) {
              N_t_prime--;
              N_vt_prime--;
            }
            N_mt_prime--;
          }
          hp_alpha_t = hp_alpha_[t];

          accept_rate =
            (N_mt_prime + hp_alpha_t) / (N_ms_prime + hp_alpha_s)
            * (N_vt_prime + hp_beta_) / (N_vs_prime + hp_beta_)
            * (N_s_prime + hp_sum_beta_) / (N_t_prime + hp_sum_beta_)
            * (N_ms + hp_alpha_s) / (N_mt + hp_alpha_t);

          assert(accept_rate >= 0.0);
          if (/*accept_rate >= 1.0 || */Rand::Double01() < accept_rate) {
            word->k = t;
            s = t;
            N_s = N_t;
            N_vs = N_vt;
            N_ms = N_mt;
            N_s_prime = N_t_prime;
            N_vs_prime = N_vt_prime;
            N_ms_prime = N_mt_prime;
            hp_alpha_s = hp_alpha_t;
          }
        }
      }
    }

    if (old_k != s) {
      if (mode_ ==  kSampleMode) {
        --topics_count_[old_k];
        --word_topics_count[old_k];
        ++topics_count_[s];
        ++word_topics_count[s];
      }
      --(*doc_topics_count)[old_k];
      ++(*doc_topics_count)[s];
    }
  }
}

int LightLDASampler::SampleWithWord(int v) {
  // word proposal: (N_vk + beta)/(N_k + sum_beta)
  // TODO(yafei) unchanged during inference
  std::vector<int>& word_v_topic_samples = words_topic_samples_[v];
  if (word_v_topic_samples.empty()) {
    double sum = 0.0;
    const IntTable& word_topics_count = words_topics_count_[v];
    if (storage_type_ == kHashTable || storage_type_ == kSparseTable) {
      word_topics_pdf_.assign(K_, 0.0);
      IntTable::const_iterator first = word_topics_count.begin();
      IntTable::const_iterator last = word_topics_count.end();
      for (; first != last; ++first) {
        const int k = first.id();
        double& pdf = word_topics_pdf_[k];
        pdf = (first.count() + hp_beta_) / (topics_count_[k] + hp_sum_beta_);
        sum += pdf;
      }

      for (int k = 0; k < K_; k++) {
        double& pdf = word_topics_pdf_[k];
        if (pdf == 0.0) {
          pdf = hp_beta_ / (topics_count_[k] + hp_sum_beta_);
          sum += pdf;
        }
      }
    } else {
      for (int k = 0; k < K_; k++) {
        double& pdf = word_topics_pdf_[k];
        pdf = (word_topics_count[k] + hp_beta_)
              / (topics_count_[k] + hp_sum_beta_);
        sum += pdf;
      }
    }

    word_alias_table_.Build(word_topics_pdf_, sum);
    int cached_samples = K_ * mh_step_;
    word_v_topic_samples.reserve(cached_samples);
    for (int i = 0; i < cached_samples; i++) {
      word_v_topic_samples.push_back(word_alias_table_.Sample());
    }
  }

  const int new_k = word_v_topic_samples.back();
  word_v_topic_samples.pop_back();
  return new_k;
}

int LightLDASampler::SampleWithDoc(Word* word, int doc_length, int v) {
  // doc proposal: N_mk + alpha_k
  double sample = Rand::Double01() * (hp_sum_alpha_ + doc_length);
  if (sample < hp_sum_alpha_) {
    return hp_alpha_alias_table_.Sample(sample / hp_sum_alpha_);
  } else {
    int index = (int)(sample - hp_sum_alpha_);
    if (index == doc_length) {
      // rare numerical errors may lie in this branch
      index--;
    }
    return word[index].k;
  }
}

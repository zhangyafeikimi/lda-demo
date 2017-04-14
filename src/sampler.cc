// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//

#include "sampler.h"
#include <random>
#include "x.h"

// u is in [0, 1)
static int SampleCDF(const std::vector<double>& cdf, double u) {
  int size = static_cast<int>(cdf.size());
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
  } else {
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

static inline int SampleCDF(const std::vector<double>& cdf, Random* gen) {
  return SampleCDF(cdf, gen->GetNext());
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

void GibbsSampler::SampleDocument(Word* word, int doc_length,
                                  TableType* doc_topics_count) {
  for (int n = 0; n < doc_length; n++, word++) {
    const int v = word->v;
    const int old_k = word->k;
    auto& word_topics_count = words_topics_count_[v];
    int k, new_k;

    --topics_count_[old_k];
    --word_topics_count[old_k];
    --(*doc_topics_count)[old_k];

    word_topic_cdf_[0] = 0.0;
    for (k = 0; k < K_ - 1; k++) {
      word_topic_cdf_[k] += (word_topics_count[k] + hp_beta_) /
                            (topics_count_[k] + hp_sum_beta_) *
                            ((*doc_topics_count)[k] + hp_alpha_[k]);
      word_topic_cdf_[k + 1] = word_topic_cdf_[k];
    }
    word_topic_cdf_[k] += (word_topics_count[k] + hp_beta_) /
                          (topics_count_[k] + hp_sum_beta_) *
                          ((*doc_topics_count)[k] + hp_alpha_[k]);

    new_k = SampleCDF(word_topic_cdf_, &random_);
    ++topics_count_[new_k];
    ++word_topics_count[new_k];
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
  const auto& doc_topics_count = docs_topics_count_[m];
  auto first = doc_topics_count.begin();
  auto last = doc_topics_count.end();
  for (; first != last; ++first) {
    const int k = first.id();
    cache_[k] = hp_alpha_[k] / (topics_count_[k] + hp_sum_beta_);
  }

  SamplerBase::HPOpt_PostSampleDocument(m);
}

void SparseLDASampler::SampleDocument(int m) {
  PrepareDocBucket(m);

  const int N = docs_[m + 1] - docs_[m];
  Word* word = &words_[docs_[m]];
  for (int n = 0; n < N; n++, word++) {
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
  auto& doc_topics_count = docs_topics_count_[m];
  auto& word_topics_count = words_topics_count_[v];
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
  double sample = random_.GetNext() * sum;
  int new_k = -1;

  if (sample < word_sum_) {
    const auto& word_topics_count = words_topics_count_[v];
    auto first = word_topics_count.begin();
    auto last = word_topics_count.end();
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
      const auto& doc_topics_count = docs_topics_count_[m];
      auto first = doc_topics_count.begin();
      auto last = doc_topics_count.end();
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
  const auto& doc_topics_count = docs_topics_count_[m];
  auto first = doc_topics_count.begin();
  auto last = doc_topics_count.end();
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
  const auto& word_topics_count = words_topics_count_[v];
  auto first = word_topics_count.begin();
  auto last = word_topics_count.end();
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

void AliasLDASampler::SampleDocument(Word* word, int doc_length,
                                     HashTable* doc_topics_count) {
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
    HashTable& word_topics_count = words_topics_count_[v];
    const int old_k = word->k;
    s = old_k;

    N_s_prime = --topics_count_[s];
    N_vs_prime = --word_topics_count[s];
#if defined SMOLA_ALIAS_LDA
    N_s = N_s_prime + 1;
    N_vs = N_vs_prime + 1;
#endif
    N_ms_prime = --(*doc_topics_count)[s];
#if defined SMOLA_ALIAS_LDA
    N_ms = N_ms_prime + 1;
#endif

    temp_s = (N_vs_prime + hp_beta_) / (N_s_prime + hp_sum_beta_);
    hp_alpha_s = hp_alpha_[s];

    // construct p: first part of the proposal
    p_sum = 0.0;
    auto first = doc_topics_count->begin();
    auto last = doc_topics_count->end();
    for (; first != last; ++first) {
      const int k = first.id();
      double& pdf = p_pdf_[k];
      pdf = first.count() * (word_topics_count[k] + hp_beta_) /
            (topics_count_[k] + hp_sum_beta_);
      p_sum += pdf;
    }

    // prepare samples from q: second part of the proposal
    q_sum = 0.0;
    std::vector<int>& word_v_q_samples = q_samples_[v];
    const int word_v_q_samples_size = static_cast<int>(word_v_q_samples.size());
    if (word_v_q_samples_size < mh_step_) {
      // construct q
      q_pdf_.assign(K_, 0.0);
      auto first = word_topics_count.begin();
      auto last = word_topics_count.end();
      for (; first != last; ++first) {
        const int k = first.id();
        double& pdf = q_pdf_[k];
        pdf = hp_alpha_[k] * (word_topics_count[k] + hp_beta_) /
              (topics_count_[k] + hp_sum_beta_);
        q_sum += pdf;
      }
      for (int k = 0; k < K_; k++) {
        double& pdf = q_pdf_[k];
        if (pdf == 0.0) {
          pdf = hp_alpha_[k] * hp_beta_ / (topics_count_[k] + hp_sum_beta_);
          q_sum += pdf;
        }
      }
#if defined SMOLA_ALIAS_LDA
      double& pdf_s = q_pdf_[s];
      q_sum -= pdf_s;
      pdf_s = hp_alpha_s * (N_vs + hp_beta_) / (N_s + hp_sum_beta_);
      q_sum += pdf_s;
#endif
      q_sums_[v] = q_sum;
      q_alias_table_.Build(&q_alias_, &q_pdf_, q_sum);

      const int cached_samples = K_ * mh_step_;
      word_v_q_samples.reserve(cached_samples);
      for (int i = word_v_q_samples_size; i < cached_samples; i++) {
        word_v_q_samples.push_back(q_alias_.Sample(random_.GetNext()));
      }
    } else {
      q_sum = q_sums_[v];
    }

    for (int step = 0; step < mh_step_; step++) {
      sample = random_.GetNext() * (p_sum + q_sum);
      if (sample < p_sum) {
        // sample from p
        auto first = doc_topics_count->begin();
        auto last = doc_topics_count->end();
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
          N_t++;
          N_vt++;
          N_mt++;
        }
#endif
        temp_t = (N_vt_prime + hp_beta_) / (N_t_prime + hp_sum_beta_);
        hp_alpha_t = hp_alpha_[t];
#if defined SMOLA_ALIAS_LDA
        accept_rate = (N_mt_prime + hp_alpha_t) / (N_ms_prime + hp_alpha_s) *
                      temp_t / temp_s *
                      (N_ms_prime * temp_s +
                       hp_alpha_s * (N_vs + hp_beta_) / (N_s + hp_sum_beta_)) /
                      (N_mt_prime * temp_t +
                       hp_alpha_t * (N_vt + hp_beta_) / (N_t + hp_sum_beta_));
#else
        accept_rate = (N_mt_prime + hp_alpha_t) / (N_ms_prime + hp_alpha_s) *
                      temp_t / temp_s * (N_ms_prime + hp_alpha_s) * temp_s /
                      (N_mt_prime + hp_alpha_t) / temp_t;
#endif
        DCHECK(accept_rate >= 0.0);
        if (random_.GetNext() < accept_rate) {
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

    ++topics_count_[s];
    ++word_topics_count[s];
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

  std::vector<double> hp_alpha = hp_alpha_;
  hp_alpha_alias_table_.Build(&hp_alpha_alias_, &hp_alpha, hp_sum_alpha_);
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
      std::vector<double> hp_alpha = hp_alpha_;
      hp_alpha_alias_table_.Build(&hp_alpha_alias_, &hp_alpha_, hp_sum_alpha_);
    }
  }
}

void LightLDASampler::SampleDocument(Word* word, int doc_length,
                                     HashTable* doc_topics_count) {
  int s, t;
  int N_s, N_vs, N_ms, N_t, N_vt, N_mt;
  int N_s_prime, N_vs_prime, N_ms_prime;
  int N_t_prime, N_vt_prime, N_mt_prime;
  double hp_alpha_s, hp_alpha_t;
  double accept_rate;
  Word* word0 = word;

  for (int n = 0; n < doc_length; n++, word++) {
    const int v = word->v;
    HashTable& word_topics_count = words_topics_count_[v];
    const int old_k = word->k;
    s = old_k;

    N_s_prime = N_s = topics_count_[s];
    N_vs_prime = N_vs = word_topics_count[s];
    N_ms_prime = N_ms = (*doc_topics_count)[s];
    if (old_k == s) {
      N_s_prime--;
      N_vs_prime--;
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
            N_t_prime--;
            N_vt_prime--;
            N_mt_prime--;
          }
          hp_alpha_t = hp_alpha_[t];

          accept_rate = (N_mt_prime + hp_alpha_t) / (N_ms_prime + hp_alpha_s) *
                        (N_vt_prime + hp_beta_) / (N_vs_prime + hp_beta_) *
                        (N_s_prime + hp_sum_beta_) /
                        (N_t_prime + hp_sum_beta_) * (N_vs + hp_beta_) /
                        (N_vt + hp_beta_) * (N_t + hp_sum_beta_) /
                        (N_s + hp_sum_beta_);

          DCHECK(accept_rate >= 0.0);
          if (random_.GetNext() < accept_rate) {
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
            N_t_prime--;
            N_vt_prime--;
            N_mt_prime--;
          }
          hp_alpha_t = hp_alpha_[t];

          accept_rate = (N_mt_prime + hp_alpha_t) / (N_ms_prime + hp_alpha_s) *
                        (N_vt_prime + hp_beta_) / (N_vs_prime + hp_beta_) *
                        (N_s_prime + hp_sum_beta_) /
                        (N_t_prime + hp_sum_beta_) * (N_ms + hp_alpha_s) /
                        (N_mt + hp_alpha_t);

          DCHECK(accept_rate >= 0.0);
          if (random_.GetNext() < accept_rate) {
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
      --topics_count_[old_k];
      --word_topics_count[old_k];
      ++topics_count_[s];
      ++word_topics_count[s];
      --(*doc_topics_count)[old_k];
      ++(*doc_topics_count)[s];
    }
  }
}

int LightLDASampler::SampleWithWord(int v) {
  // word proposal: (N_vk + beta)/(N_k + sum_beta)
  std::vector<int>& word_v_topic_samples = words_topic_samples_[v];
  if (word_v_topic_samples.empty()) {
    double sum = 0.0;
    const HashTable& word_topics_count = words_topics_count_[v];
    word_topics_pdf_.assign(K_, 0.0);
    auto first = word_topics_count.begin();
    auto last = word_topics_count.end();
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

    word_alias_table_.Build(&word_alias_, &word_topics_pdf_, sum);
    int cached_samples = K_ * mh_step_;
    word_v_topic_samples.reserve(cached_samples);
    for (int i = 0; i < cached_samples; i++) {
      word_v_topic_samples.push_back(word_alias_.Sample(random_.GetNext()));
    }
  }

  const int new_k = word_v_topic_samples.back();
  word_v_topic_samples.pop_back();
  return new_k;
}

int LightLDASampler::SampleWithDoc(Word* word, int doc_length, int v) {
  // doc proposal: N_mk + alpha_k
  double sample = random_.GetNext() * (hp_sum_alpha_ + doc_length);
  if (sample < hp_sum_alpha_) {
    return hp_alpha_alias_.Sample(sample / hp_sum_alpha_);
  } else {
    int index = static_cast<int>(sample - hp_sum_alpha_);
    if (index == doc_length) {
      // rare numerical errors may lie in this branch
      index--;
    }
    return word[index].k;
  }
}

// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// sampler and inference
//

#ifndef SAMPLER_H_
#define SAMPLER_H_

#include <string>
#include <vector>
#include "alias.h"
#include "model.h"
#include "rand.h"
#include "table.h"
#include "x.h"

/************************************************************************/
/* SamplerBase */
/************************************************************************/
template <class Tables>
class SamplerBase : public Model<Tables> {
 protected:
  typedef Model<Tables> BaseType;
  using BaseType::docs_;
  using BaseType::words_;
  using BaseType::M_;
  using BaseType::V_;
  using BaseType::K_;
  using BaseType::topics_count_;
  using BaseType::docs_topics_count_;
  using BaseType::words_topics_count_;
  using BaseType::hp_alpha_;
  using BaseType::hp_sum_alpha_;
  using BaseType::hp_beta_;
  using BaseType::hp_sum_beta_;
  using BaseType::hp_opt_;
  using BaseType::hp_opt_interval_;
  using BaseType::hp_opt_alpha_shape_;
  using BaseType::hp_opt_alpha_scale_;
  using BaseType::hp_opt_alpha_iteration_;
  using BaseType::hp_opt_beta_iteration_;
  using BaseType::docs_topic_count_hist_;
  using BaseType::doc_len_hist_;
  using BaseType::word_topic_count_hist_;
  using BaseType::topic_len_hist_;
  using BaseType::total_iteration_;
  using BaseType::burnin_iteration_;
  using BaseType::log_likelihood_interval_;
  using BaseType::iteration_;
  Random random_;

 public:
  typedef Tables TablesType;
  typedef typename TablesType::TableType TableType;

 public:
  virtual int InitializeSampler();
  virtual double LogLikelihood() const;
  virtual int Train();
  virtual void PreSampleCorpus();
  virtual void PostSampleCorpus();
  virtual void SampleCorpus();
  virtual void PreSampleDocument(int m);
  virtual void PostSampleDocument(int m);
  virtual void SampleDocument(int m);
  virtual void SampleDocument(Word* word, int doc_length,
                              TableType* doc_topics_count);
  virtual void HPOpt_Initialize();
  virtual void HPOpt_Optimize();
  virtual void HPOpt_OptimizeAlpha();
  virtual void HPOpt_PrepareOptimizeBeta();
  virtual void HPOpt_OptimizeBeta();
  virtual void HPOpt_PostSampleDocument(int m);

  bool HPOpt_Enabled() const {
    if (hp_opt_ && iteration_ > burnin_iteration_ &&
        (iteration_ % hp_opt_interval_) == 0) {
      return true;
    }
    return false;
  }
};

template <class Tables>
int SamplerBase<Tables>::InitializeSampler() {
  if (hp_sum_alpha_ <= 0.0) {
    const double avg_doc_len = (double)words_.size() / M_;
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

  topics_count_.Init(K_);
  docs_topics_count_.Init(M_, K_);
  words_topics_count_.Init(V_, K_);

  // random initialize topics
  for (int m = 0; m < M_; m++) {
    const int N = docs_[m + 1] - docs_[m];
    Word* word = &words_[docs_[m]];
    auto& doc_topics_count = docs_topics_count_[m];
    for (int n = 0; n < N; n++, word++) {
      const int v = word->v;
      const int new_topic = random_.GetNext(K_);
      word->k = new_topic;
      ++topics_count_[new_topic];
      ++doc_topics_count[new_topic];
      ++words_topics_count_[v][new_topic];
    }
  }

  return 0;
}

template <class Tables>
double SamplerBase<Tables>::LogLikelihood() const {
  double sum = 0.0;
#if defined _OPENMP
#pragma omp parallel for schedule(static) reduction(+ : sum)
#endif
  for (int m = 0; m < M_; m++) {
    const int N = docs_[m + 1] - docs_[m];
    const Word* word = &words_[docs_[m]];
    const auto& doc_topics_count = docs_topics_count_[m];
    for (int n = 0; n < N; n++, word++) {
      const int v = word->v;
      double word_sum = 0.0;
      for (int k = 0; k < K_; k++) {
        double phi_kv = (words_topics_count_[v][k] + hp_beta_) /
                        (topics_count_[k] + hp_sum_beta_);
        word_sum += (doc_topics_count[k] + hp_alpha_[k]) * phi_kv;
      }
      word_sum /= (N + hp_sum_alpha_);
      sum += log(word_sum);
    }
  }
  return sum;
}

template <class Tables>
int SamplerBase<Tables>::Train() {
  INFO("Training begins.");

  if (InitializeSampler() != 0) {
    return -1;
  }

  for (iteration_ = 1; iteration_ <= total_iteration_; iteration_++) {
    INFO("Iteration %d begins.", iteration_);
    PreSampleCorpus();
    SampleCorpus();
    PostSampleCorpus();

    if (iteration_ > burnin_iteration_ &&
        iteration_ % log_likelihood_interval_ == 0) {
      INFO("Calculating LogLikelihood.");
      const double llh = LogLikelihood();
      INFO("LogLikelihood(total/word)=%lg/%lg.", llh, llh / words_.size());
    }
  }

  INFO("Training ended.");
  return 0;
}

template <class Tables>
void SamplerBase<Tables>::PreSampleCorpus() {
  HPOpt_Initialize();
}

template <class Tables>
void SamplerBase<Tables>::PostSampleCorpus() {
  HPOpt_Optimize();
}

template <class Tables>
void SamplerBase<Tables>::SampleCorpus() {
  for (int m = 0; m < M_; m++) {
    PreSampleDocument(m);
    SampleDocument(m);
    PostSampleDocument(m);
  }
}

template <class Tables>
void SamplerBase<Tables>::PreSampleDocument(int m) {}

template <class Tables>
void SamplerBase<Tables>::PostSampleDocument(int m) {
  HPOpt_PostSampleDocument(m);
}

template <class Tables>
void SamplerBase<Tables>::SampleDocument(int m) {
  const int N = docs_[m + 1] - docs_[m];
  Word* word = &words_[docs_[m]];
  auto& doc_topics_count = docs_topics_count_[m];
  SampleDocument(word, N, &doc_topics_count);
}

template <class Tables>
void SamplerBase<Tables>::SampleDocument(Word* word, int doc_length,
                                         TableType* doc_topics_count) {}

template <class Tables>
void SamplerBase<Tables>::HPOpt_Initialize() {
  if (!HPOpt_Enabled()) {
    return;
  }

  INFO("Hyper optimization will be carried out in this iteration.");
  docs_topic_count_hist_.clear();
  docs_topic_count_hist_.resize(K_);
  doc_len_hist_.clear();
  word_topic_count_hist_.clear();
  topic_len_hist_.clear();
}

template <class Tables>
void SamplerBase<Tables>::HPOpt_Optimize() {
  if (!HPOpt_Enabled()) {
    return;
  }

  if (hp_opt_alpha_iteration_ > 0) {
    INFO("Hyper optimizing alpha.");
    HPOpt_OptimizeAlpha();
  }
  if (hp_opt_beta_iteration_ > 0) {
    INFO("Hyper optimizing beta.");
    HPOpt_PrepareOptimizeBeta();
    HPOpt_OptimizeBeta();
  }
}

template <class Tables>
void SamplerBase<Tables>::HPOpt_OptimizeAlpha() {
  for (int i = 0; i < hp_opt_alpha_iteration_; i++) {
    double denom = 0.0;
    double diff_digamma = 0.0;
    for (int j = 1, size = static_cast<int>(doc_len_hist_.size()); j < size;
         j++) {
      diff_digamma += 1.0 / (j - 1 + hp_sum_alpha_);
      denom += doc_len_hist_[j] * diff_digamma;
    }
    denom -= 1.0 / hp_opt_alpha_scale_;

    hp_sum_alpha_ = 0.0;
    for (int k = 0, size = static_cast<int>(docs_topic_count_hist_.size());
         k < size; k++) {
      double num = 0.0;
      double alpha_k = hp_alpha_[k];
      const std::vector<int>& docs_topic_k_count_hist =
          docs_topic_count_hist_[k];
      diff_digamma = 0.0;
      for (int j = 1, size = static_cast<int>(docs_topic_count_hist_[k].size());
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

template <class Tables>
void SamplerBase<Tables>::HPOpt_PrepareOptimizeBeta() {
  for (int m = 0; m < M_; m++) {
    const auto& doc_topics_count = docs_topics_count_[m];
    for (int k = 0; k < K_; k++) {
      const int count = doc_topics_count[k];
      if (count == 0) {
        continue;
      }
      if (static_cast<int>(word_topic_count_hist_.size()) <= count) {
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
    if (static_cast<int>(topic_len_hist_.size()) <= count) {
      topic_len_hist_.resize(count + 1);
    }
    topic_len_hist_[count]++;
  }
}

template <class Tables>
void SamplerBase<Tables>::HPOpt_OptimizeBeta() {
  for (int i = 0; i < hp_opt_beta_iteration_; i++) {
    double num = 0.0;
    double diff_digamma = 0.0;
    for (int j = 1, size = static_cast<int>(word_topic_count_hist_.size());
         j < size; j++) {
      diff_digamma += 1.0 / (j - 1 + hp_beta_);
      num += diff_digamma * word_topic_count_hist_[j];
    }

    double denom = 0.0;
    diff_digamma = 0.0;
    for (int j = 1, size = static_cast<int>(topic_len_hist_.size()); j < size;
         j++) {
      diff_digamma += 1.0 / (j - 1 + hp_sum_beta_);
      denom += diff_digamma * topic_len_hist_[j];
    }
    hp_sum_beta_ = hp_beta_ * num / denom;
    hp_beta_ = hp_sum_beta_ / V_;
  }
}

template <class Tables>
void SamplerBase<Tables>::HPOpt_PostSampleDocument(int m) {
  if (!HPOpt_Enabled()) {
    return;
  }

  if (hp_opt_alpha_iteration_ > 0) {
    const int N = docs_[m + 1] - docs_[m];
    const auto& doc_topics_count = docs_topics_count_[m];
    for (int k = 0; k < K_; k++) {
      const int count = doc_topics_count[k];
      if (count == 0) {
        continue;
      }
      std::vector<int>& docs_topic_k_count_hist = docs_topic_count_hist_[k];
      if (static_cast<int>(docs_topic_k_count_hist.size()) <= count) {
        docs_topic_k_count_hist.resize(count + 1);
      }
      docs_topic_k_count_hist[count]++;
    }

    if (N > 0) {
      if (static_cast<int>(doc_len_hist_.size()) <= N) {
        doc_len_hist_.resize(N + 1);
      }
      doc_len_hist_[N]++;
    }
  }
}

/************************************************************************/
/* GibbsSampler */
/************************************************************************/
class GibbsSampler : public SamplerBase<HashTables> {
 private:
  std::vector<double> word_topic_cdf_;  // cached

 public:
  GibbsSampler() {}
  virtual int InitializeSampler();
  virtual void SampleDocument(Word* word, int doc_length,
                              TableType* doc_topics_count);
};

/************************************************************************/
/* SparseLDASampler */
/************************************************************************/
class SparseLDASampler : public SamplerBase<SparseTables> {
 private:
  double smooth_sum_;
  double doc_sum_;
  double word_sum_;
  std::vector<double> smooth_pdf_;
  std::vector<double> doc_pdf_;
  std::vector<double> word_pdf_;
  std::vector<double> cache_;

 public:
  SparseLDASampler() {}
  virtual int InitializeSampler();
  virtual void PostSampleCorpus();
  virtual void PostSampleDocument(int m);
  virtual void SampleDocument(int m);

 private:
  void RemoveOrAddWordTopic(int m, int v, int k, int remove);
  int SampleDocumentWord(int m, int v);
  void PrepareSmoothBucket();
  void PrepareDocBucket(int m);
  void PrepareWordBucket(int v);
};

/************************************************************************/
/* AliasLDASampler */
/************************************************************************/
class AliasLDASampler : public SamplerBase<HashTables> {
 private:
  std::vector<double> p_pdf_;
  std::vector<double> q_sums_;                // for each word v
  std::vector<std::vector<int> > q_samples_;  // for each word v
  std::vector<double> q_pdf_;
  AliasBuilder q_alias_table_;
  AliasD q_alias_;

  int mh_step_;

 public:
  AliasLDASampler() : mh_step_(0) {}

  // setters
  int& mh_step() { return mh_step_; }
  // end of setters

  virtual int InitializeSampler();
  virtual void SampleDocument(Word* word, int doc_length,
                              TableType* doc_topics_count);
};

/************************************************************************/
/* LightLDASampler */
/************************************************************************/
class LightLDASampler : public SamplerBase<HashTables> {
 private:
  AliasBuilder hp_alpha_alias_table_;
  AliasD hp_alpha_alias_;
  AliasBuilder word_alias_table_;
  AliasD word_alias_;
  std::vector<double> word_topics_pdf_;
  std::vector<std::vector<int> > words_topic_samples_;
  int mh_step_;
  int enable_word_proposal_;
  int enable_doc_proposal_;

 public:
  LightLDASampler()
      : mh_step_(0), enable_word_proposal_(1), enable_doc_proposal_(1) {}

  // setters
  int& mh_step() { return mh_step_; }

  int& enable_word_proposal() { return enable_word_proposal_; }

  int& enable_doc_proposal() { return enable_doc_proposal_; }
  // end of setters

  virtual int InitializeSampler();
  virtual void PostSampleCorpus();
  virtual void SampleDocument(Word* word, int doc_length,
                              TableType* doc_topics_count);

 private:
  int SampleWithWord(int v);
  int SampleWithDoc(Word* word, int doc_length, int v);
};

#endif  // SAMPLER_H_

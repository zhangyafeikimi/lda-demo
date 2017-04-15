// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// sampler and inference
//

#ifndef SAMPLER_H_
#define SAMPLER_H_

#include <math.h>
#include <string>
#include <vector>
#include "alias.h"
#include "model.h"
#include "table.h"
#include "x.h"

/************************************************************************/
/* Sampler */
/************************************************************************/
template <class Tables>
class Sampler : public Model<Tables> {
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
  using BaseType::random_;
  using BaseType::Init;

  // hyper parameters optimizations
  int hp_opt_;
  int hp_opt_interval_;
  double hp_opt_alpha_shape_;
  double hp_opt_alpha_scale_;
  int hp_opt_alpha_iteration_;
  int hp_opt_beta_iteration_;
  // hp_opt_docs_topic_count_hist_[k][n]:
  // # of documents in which topic "k" occurs "n" times.
  std::vector<std::vector<int> > hp_opt_docs_topic_count_hist_;
  // hp_opt_doc_len_hist_[n]:
  // # of documents whose length are "n".
  std::vector<int> hp_opt_doc_len_hist_;
  // hp_opt_word_topic_count_hist_[n]:
  // # of words which are assigned to a topic "n" times.
  std::vector<int> hp_opt_word_topic_count_hist_;
  // hp_opt_topic_len_hist_a[n]:
  // # of topics which occurs "n" times.
  std::vector<int> hp_opt_topic_len_hist_a;

  // iteration variables
  int total_iteration_;
  int burnin_iteration_;
  int log_likelihood_interval_;
  int iteration_;

 public:
  typedef Tables TablesType;
  typedef typename TablesType::TableType TableType;

  Sampler()
      : hp_opt_(0),
        hp_opt_interval_(0),
        hp_opt_alpha_shape_(0.0),
        hp_opt_alpha_scale_(0.0),
        hp_opt_alpha_iteration_(0),
        hp_opt_beta_iteration_(0),
        total_iteration_(0),
        burnin_iteration_(0),
        log_likelihood_interval_(0),
        iteration_(0) {}

  int& hp_opt() { return hp_opt_; }
  int& hp_opt_interval() { return hp_opt_interval_; }
  double& hp_opt_alpha_shape() { return hp_opt_alpha_shape_; }
  double& hp_opt_alpha_scale() { return hp_opt_alpha_scale_; }
  int& hp_opt_alpha_iteration() { return hp_opt_alpha_iteration_; }
  int& hp_opt_beta_iteration() { return hp_opt_beta_iteration_; }
  int& total_iteration() { return total_iteration_; }
  int& burnin_iteration() { return burnin_iteration_; }
  int& log_likelihood_interval() { return log_likelihood_interval_; }

  virtual double LogLikelihood() const;
  virtual void Train();
  virtual void PreSampleCorpus();
  virtual void PostSampleCorpus();
  virtual void SampleCorpus();
  virtual void PreSampleDocument(int m);
  virtual void PostSampleDocument(int m);
  virtual void SampleDocument(int m);
  virtual void SampleDocument(Word* word, int doc_length,
                              TableType* doc_topics_count);
  void HPOpt_Init();
  void HPOpt_Optimize();
  void HPOpt_OptimizeAlpha();
  void HPOpt_PrepareOptimizeBeta();
  void HPOpt_OptimizeBeta();
  void HPOpt_PostSampleDocument(int m);

  bool HPOpt_Enabled() const {
    if (hp_opt_ && iteration_ > burnin_iteration_ &&
        (iteration_ % hp_opt_interval_) == 0) {
      return true;
    }
    return false;
  }
};

template <class Tables>
double Sampler<Tables>::LogLikelihood() const {
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
      const auto& word_topics_count = words_topics_count_[v];
      double word_sum = 0.0;
      for (int k = 0; k < K_; k++) {
        const double phi_kv = (word_topics_count[k] + hp_beta_) /
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
void Sampler<Tables>::Train() {
  INFO("Training begins.");
  Init();
  for (iteration_ = 1; iteration_ <= total_iteration_; iteration_++) {
    INFO("Iteration %d begins.", iteration_);
    PreSampleCorpus();
    SampleCorpus();
    PostSampleCorpus();

    if ((iteration_ > burnin_iteration_) &&
        (iteration_ % log_likelihood_interval_ == 0)) {
      INFO("Calculating LogLikelihood.");
      const double llh = LogLikelihood();
      INFO("LogLikelihood(total/word)=%lg/%lg.", llh, llh / words_.size());
    }
  }
  INFO("Training ended.");
}

template <class Tables>
void Sampler<Tables>::PreSampleCorpus() {
  HPOpt_Init();
}

template <class Tables>
void Sampler<Tables>::PostSampleCorpus() {
  HPOpt_Optimize();
}

template <class Tables>
void Sampler<Tables>::SampleCorpus() {
  for (int m = 0; m < M_; m++) {
    PreSampleDocument(m);
    SampleDocument(m);
    PostSampleDocument(m);
  }
}

template <class Tables>
void Sampler<Tables>::PreSampleDocument(int m) {}

template <class Tables>
void Sampler<Tables>::PostSampleDocument(int m) {
  HPOpt_PostSampleDocument(m);
}

template <class Tables>
void Sampler<Tables>::SampleDocument(int m) {
  const int N = docs_[m + 1] - docs_[m];
  Word* word = &words_[docs_[m]];
  auto& doc_topics_count = docs_topics_count_[m];
  SampleDocument(word, N, &doc_topics_count);
}

template <class Tables>
void Sampler<Tables>::SampleDocument(Word* word, int doc_length,
                                     TableType* doc_topics_count) {}

template <class Tables>
void Sampler<Tables>::HPOpt_Init() {
  if (!HPOpt_Enabled()) {
    return;
  }

  INFO("Hyper optimization will be carried out in this iteration.");
  hp_opt_docs_topic_count_hist_.clear();
  hp_opt_docs_topic_count_hist_.resize(K_);
  hp_opt_doc_len_hist_.clear();
  hp_opt_word_topic_count_hist_.clear();
  hp_opt_topic_len_hist_a.clear();
}

template <class Tables>
void Sampler<Tables>::HPOpt_Optimize() {
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
void Sampler<Tables>::HPOpt_OptimizeAlpha() {
  for (int i = 0; i < hp_opt_alpha_iteration_; i++) {
    double denom = 0;
    double diff_digamma = 0;
    for (int j = 1, size = static_cast<int>(hp_opt_doc_len_hist_.size());
         j < size; j++) {
      diff_digamma += 1.0 / (j - 1 + hp_sum_alpha_);
      denom += hp_opt_doc_len_hist_[j] * diff_digamma;
    }
    denom -= 1.0 / hp_opt_alpha_scale_;

    hp_sum_alpha_ = 0;
    for (int k = 0,
             size = static_cast<int>(hp_opt_docs_topic_count_hist_.size());
         k < size; k++) {
      double num = 0;
      double alpha_k = hp_alpha_[k];
      const auto& docs_topic_k_count_hist = hp_opt_docs_topic_count_hist_[k];
      diff_digamma = 0;
      for (int j = 1,
               size = static_cast<int>(hp_opt_docs_topic_count_hist_[k].size());
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
void Sampler<Tables>::HPOpt_PrepareOptimizeBeta() {
  for (int m = 0; m < M_; m++) {
    const auto& doc_topics_count = docs_topics_count_[m];
    for (int k = 0; k < K_; k++) {
      const int count = doc_topics_count[k];
      if (count == 0) {
        continue;
      }
      if (static_cast<int>(hp_opt_word_topic_count_hist_.size()) <= count) {
        hp_opt_word_topic_count_hist_.resize(count + 1);
      }
      hp_opt_word_topic_count_hist_[count]++;
    }
  }

  for (int k = 0; k < K_; k++) {
    const int count = topics_count_[k];
    if (count == 0) {
      continue;
    }
    if (static_cast<int>(hp_opt_topic_len_hist_a.size()) <= count) {
      hp_opt_topic_len_hist_a.resize(count + 1);
    }
    hp_opt_topic_len_hist_a[count]++;
  }
}

template <class Tables>
void Sampler<Tables>::HPOpt_OptimizeBeta() {
  for (int i = 0; i < hp_opt_beta_iteration_; i++) {
    double num = 0;
    double diff_digamma = 0;
    for (int j = 1,
             size = static_cast<int>(hp_opt_word_topic_count_hist_.size());
         j < size; j++) {
      diff_digamma += 1.0 / (j - 1 + hp_beta_);
      num += diff_digamma * hp_opt_word_topic_count_hist_[j];
    }

    double denom = 0;
    diff_digamma = 0;
    for (int j = 1, size = static_cast<int>(hp_opt_topic_len_hist_a.size());
         j < size; j++) {
      diff_digamma += 1.0 / (j - 1 + hp_sum_beta_);
      denom += diff_digamma * hp_opt_topic_len_hist_a[j];
    }
    hp_sum_beta_ = hp_beta_ * num / denom;
    hp_beta_ = hp_sum_beta_ / V_;
  }
}

template <class Tables>
void Sampler<Tables>::HPOpt_PostSampleDocument(int m) {
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
      auto& docs_topic_k_count_hist = hp_opt_docs_topic_count_hist_[k];
      if (static_cast<int>(docs_topic_k_count_hist.size()) <= count) {
        docs_topic_k_count_hist.resize(count + 1);
      }
      docs_topic_k_count_hist[count]++;
    }

    if (N > 0) {
      if (static_cast<int>(hp_opt_doc_len_hist_.size()) <= N) {
        hp_opt_doc_len_hist_.resize(N + 1);
      }
      hp_opt_doc_len_hist_[N]++;
    }
  }
}

/************************************************************************/
/* GibbsSampler */
/************************************************************************/
class GibbsSampler : public Sampler<HashTables> {
 private:
  std::vector<double> word_topic_cdf_;  // cached

 public:
  GibbsSampler() {}
  virtual void Init() override;
  virtual void SampleDocument(Word* word, int doc_length,
                              TableType* doc_topics_count) override;
};

/************************************************************************/
/* SparseLDASampler */
/************************************************************************/
class SparseLDASampler : public Sampler<SparseTables> {
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
  virtual void Init() override;
  virtual void PostSampleCorpus() override;
  virtual void PostSampleDocument(int m) override;
  virtual void SampleDocument(int m) override;

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
class AliasLDASampler : public Sampler<HashTables> {
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
  int& mh_step() { return mh_step_; }
  virtual void Init() override;
  virtual void SampleDocument(Word* word, int doc_length,
                              TableType* doc_topics_count) override;
};

/************************************************************************/
/* LightLDASampler */
/************************************************************************/
class LightLDASampler : public Sampler<HashTables> {
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
  int& mh_step() { return mh_step_; }
  int& enable_word_proposal() { return enable_word_proposal_; }
  int& enable_doc_proposal() { return enable_doc_proposal_; }

  virtual void Init() override;
  virtual void PostSampleCorpus() override;
  virtual void SampleDocument(Word* word, int doc_length,
                              TableType* doc_topics_count) override;

 private:
  int SampleWithWord(int v);
  int SampleWithDoc(Word* word, int doc_length, int v);
};

#endif  // SAMPLER_H_

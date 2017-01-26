// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// sampler and inference
//

#ifndef SRC_LDA_SAMPLER_H_
#define SRC_LDA_SAMPLER_H_

#include <string>
#include <vector>
#include "lda/alias.h"
#include "lda/model.h"

/************************************************************************/
/* SamplerBase */
/************************************************************************/
class SamplerBase : public Model {
 public:
  virtual int LoadModel(const std::string& prefix);
  virtual void SaveModel(const std::string& prefix) const;

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
                              IntTable* doc_topics_count);
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

 public:
  virtual int InitializeInfer();
  virtual void InferDocument(Word* word, int doc_length,
                             IntTable* doc_topics_count);
  virtual void InferDocument(Word* word, int doc_length, int* most_prob_topic);
  virtual void InferDocument(int* word_ids, int doc_length,
                             IntTable* doc_topics_count);
  virtual void InferDocument(int* word_ids, int doc_length,
                             int* most_prob_topic);
};

/************************************************************************/
/* GibbsSampler */
/************************************************************************/
class GibbsSampler : public SamplerBase {
 private:
  std::vector<double> word_topic_cdf_;  // cached

 public:
  GibbsSampler() {}
  virtual int InitializeSampler();
  virtual int InitializeInfer();
  virtual void SampleDocument(Word* word, int doc_length,
                              IntTable* doc_topics_count);
};

/************************************************************************/
/* SparseLDASampler */
/************************************************************************/
class SparseLDASampler : public SamplerBase {
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
class AliasLDASampler : public SamplerBase {
 private:
  std::vector<double> p_pdf_;
  std::vector<double> q_sums_;                // for each word v
  std::vector<std::vector<int> > q_samples_;  // for each word v
  std::vector<double> q_pdf_;
  Alias q_alias_table_;

  int mh_step_;

 public:
  AliasLDASampler() : mh_step_(0) {}

  // setters
  int& mh_step() { return mh_step_; }
  // end of setters

  virtual int InitializeSampler();
  virtual int InitializeInfer();
  virtual void SampleDocument(Word* word, int doc_length,
                              IntTable* doc_topics_count);
};

/************************************************************************/
/* LightLDASampler */
/************************************************************************/
class LightLDASampler : public SamplerBase {
 private:
  Alias hp_alpha_alias_table_;
  Alias word_alias_table_;
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
  virtual int InitializeInfer();
  virtual void PostSampleCorpus();
  virtual void SampleDocument(Word* word, int doc_length,
                              IntTable* doc_topics_count);

 private:
  int SampleWithWord(int v);
  int SampleWithDoc(Word* word, int doc_length, int v);
};

#endif  // SRC_LDA_SAMPLER_H_

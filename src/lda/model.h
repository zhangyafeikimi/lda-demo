// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// document, word and model
//

#ifndef SRC_LDA_MODEL_H_
#define SRC_LDA_MODEL_H_

#include <string>
#include <vector>
#include "lda/array.h"
#include "lda/table.h"

struct Doc {
  int index;  // index in "SamplerBase::words_"
  int N;  // # of words
};

struct Word {
  int v;  // word id in vocabulary, starts from 0
  int k;  // topic id assign to this word, starts from 0
};

int LoadTable(const std::string& filename, IntTable* table);
void SaveTable(const std::string& filename, const IntTable& table);
int LoadTables(const std::string& filename, IntTables* tables);
void SaveTables(const std::string& filename, const IntTables& tables);
int LoadArray2D(const std::string& filename, Array2D<double>* a);
void SaveArray2D(const std::string& filename, const Array2D<double>& a);

enum {
  kSampleMode = 0,
  kInferMode = 1,
};

class Model {
 protected:
  int mode_;

  // corpus
  std::vector<std::string> doc_ids_;
  std::vector<Doc> docs_;
  std::vector<Word> words_;
  int M_;  // # of docs
  int V_;  // # of vocabulary

  // model parameters
  int K_;  // # of topics
  // topics_count_[k]: # of words assigned to topic k
  IntTable topics_count_;
  // docs_topics_count_[m][k]: # of words in doc m assigned to topic k
  IntTables docs_topics_count_;
  // words_topics_count_[v][k]: # of word v assigned to topic k
  IntTables words_topics_count_;

  // model hyper parameters
  // hp_alpha_[k]: asymmetric doc-topic prior for topic k
  std::vector<double> hp_alpha_;
  double hp_sum_alpha_;
  // beta: symmetric topic-word prior for topic k
  double hp_beta_;
  double hp_sum_beta_;

  // hyper parameters optimizations
  int hp_opt_;
  int hp_opt_interval_;
  double hp_opt_alpha_shape_;
  double hp_opt_alpha_scale_;
  int hp_opt_alpha_iteration_;
  int hp_opt_beta_iteration_;
  // docs_topic_count_hist_[k][n]:
  // # of documents in which topic "k" occurs "n" times.
  std::vector<std::vector<int> > docs_topic_count_hist_;
  // doc_len_hist_[n]:
  // # of documents whose length are "n".
  std::vector<int> doc_len_hist_;
  // word_topic_count_hist_[n]:
  // # of words which are assigned to a topic "n" times.
  std::vector<int> word_topic_count_hist_;
  // topic_len_hist_[n]:
  // # of topics which occurs "n" times.
  std::vector<int> topic_len_hist_;

  // iteration variables
  int total_iteration_;
  int burnin_iteration_;
  int log_likelihood_interval_;
  int iteration_;
  int storage_type_;

 public:
  Model() : mode_(kSampleMode),
    M_(0),
    V_(0),
    K_(0),
    hp_sum_alpha_(0.0),
    hp_beta_(0.0),
    hp_opt_(0),
    hp_opt_interval_(0),
    hp_opt_alpha_shape_(0.0),
    hp_opt_alpha_scale_(0.0),
    hp_opt_alpha_iteration_(0),
    hp_opt_beta_iteration_(0),
    total_iteration_(0),
    burnin_iteration_(0),
    log_likelihood_interval_(0),
    iteration_(0),
    storage_type_(kHashTable) {}
  virtual ~Model() {}

  // getters & setters
  int M() {
    return M_;
  }

  int V() {
    return V_;
  }

  int& K() {
    return K_;
  }

  double& alpha() {
    return hp_sum_alpha_;
  }

  double& beta() {
    return hp_beta_;
  }

  int& hp_opt() {
    return hp_opt_;
  }

  int& hp_opt_interval() {
    return hp_opt_interval_;
  }

  double& hp_opt_alpha_shape() {
    return hp_opt_alpha_shape_;
  }

  double& hp_opt_alpha_scale() {
    return hp_opt_alpha_scale_;
  }

  int& hp_opt_alpha_iteration() {
    return hp_opt_alpha_iteration_;
  }

  int& hp_opt_beta_iteration() {
    return hp_opt_beta_iteration_;
  }

  int& total_iteration() {
    return total_iteration_;
  }

  int& burnin_iteration() {
    return burnin_iteration_;
  }

  int& log_likelihood_interval() {
    return log_likelihood_interval_;
  }

  int& storage_type() {
    return storage_type_;
  }
  // end of getters & setters

  int LoadCorpus(const std::string& filename, int with_id);

  void CollectTheta(Array2D<double>* theta) const;
  void CollectPhi(Array2D<double>* phi) const;

  int LoadMVK(const std::string& filename);
  int LoadTopicsCount(const std::string& filename);
  int LoadWordsTopicsCount(const std::string& filename);
  int LoadDocsTopicsCount(const std::string& filename);
  int LoadHPAlpha(const std::string& filename);
  int LoadHPBeta(const std::string& filename);

  void SaveMVK(const std::string& filename) const;
  void SaveTopicsCount(const std::string& filename) const;
  void SaveWordsTopicsCount(const std::string& filename) const;
  void SaveDocsTopicsCount(const std::string& filename) const;
  void SaveHPAlpha(const std::string& filename) const;
  void SaveHPBeta(const std::string& filename) const;
};

#endif  // SRC_LDA_MODEL_H_
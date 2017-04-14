// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// LDA model including documents and words
//

#ifndef MODEL_H_
#define MODEL_H_

#include <string>
#include <vector>
#include "table.h"

struct Doc {
  int index;  // index in "SamplerBase::words_"
  int N;      // # of words
};

struct Word {
  int v;  // word id in vocabulary, starts from 0
  int k;  // topic id assign to this word, starts from 0
};

class Model {
 protected:
  // corpus
  std::vector<Doc> docs_;
  std::vector<Word> words_;
  int M_;  // # of docs
  int V_;  // # of vocabulary

  // model parameters
  int K_;  // # of topics
  // topics_count_[k]: # of words assigned to topic k
  DenseTable topics_count_;
  // docs_topics_count_[m][k]: # of words in doc m assigned to topic k
  HashTables docs_topics_count_;
  // words_topics_count_[v][k]: # of word v assigned to topic k
  HashTables words_topics_count_;

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

 public:
  Model()
      : 
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
        iteration_(0) {}
  virtual ~Model() {}

  // getters & setters
  int M() { return M_; }
  int V() { return V_; }
  int& K() { return K_; }
  double& alpha() { return hp_sum_alpha_; }
  double& beta() { return hp_beta_; }
  int& hp_opt() { return hp_opt_; }
  int& hp_opt_interval() { return hp_opt_interval_; }
  double& hp_opt_alpha_shape() { return hp_opt_alpha_shape_; }
  double& hp_opt_alpha_scale() { return hp_opt_alpha_scale_; }
  int& hp_opt_alpha_iteration() { return hp_opt_alpha_iteration_; }
  int& hp_opt_beta_iteration() { return hp_opt_beta_iteration_; }
  int& total_iteration() { return total_iteration_; }
  int& burnin_iteration() { return burnin_iteration_; }
  int& log_likelihood_interval() { return log_likelihood_interval_; }
  // end of getters & setters

  bool LoadCorpus(const std::string& filename, int with_id);

  bool LoadMVK(const std::string& filename);
  bool LoadTopicsCount(const std::string& filename);
  bool LoadWordsTopicsCount(const std::string& filename);
  bool LoadDocsTopicsCount(const std::string& filename);
  bool LoadHPAlpha(const std::string& filename);
  bool LoadHPBeta(const std::string& filename);

  bool SaveMVK(const std::string& filename) const;
  bool SaveTopicsCount(const std::string& filename) const;
  bool SaveWordsTopicsCount(const std::string& filename) const;
  bool SaveDocsTopicsCount(const std::string& filename) const;
  bool SaveHPAlpha(const std::string& filename) const;
  bool SaveHPBeta(const std::string& filename) const;
};

#endif  // MODEL_H_

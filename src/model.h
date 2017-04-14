// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// LDA model including corpus, counts, hyper parameters
//

#ifndef MODEL_H_
#define MODEL_H_

#include <fstream>
#include <string>
#include <vector>
#include "corpus.h"
#include "table.h"
#include "x.h"

template <class Tables>
class Model : public Corpus {
 public:
  typedef Tables TablesType;
  typedef typename TablesType::TableType TableType;

 protected:
  int K_;  // # of topics

  // topics_count_[k]: # of words assigned to topic k
  DenseTable topics_count_;
  // docs_topics_count_[m][k]: # of words in doc m assigned to topic k
  TablesType docs_topics_count_;
  // words_topics_count_[v][k]: # of word v assigned to topic k
  TablesType words_topics_count_;

  // hyper parameters
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
      : K_(0),
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

  bool SaveModel(const std::string& prefix) const {
    INFO("Saving model.");
    return SaveMeta(prefix + "-meta") &&
           SaveTopicCount(prefix + "-topic-count") &&
           SaveWordTopicCount(prefix + "-word-topic-count");
  }

 private:
  bool SaveMeta(const std::string& filename) const {
    std::ofstream ofs(filename.c_str());
    if (!ofs.is_open()) {
      ERROR("Failed to open \"%s\".", filename.c_str());
      return false;
    }

    ofs << "M=" << M_ << std::endl;
    ofs << "V=" << V_ << std::endl;
    ofs << "K=" << K_ << std::endl;
    for (int k = 0; k < K_; k++) {
      ofs << "a=" << hp_alpha_[k] << std::endl;
    }
    ofs << "b=" << hp_beta_ << std::endl;
    return true;
  }

  bool SaveTopicCount(const std::string& filename) const {
    return topics_count_.Save(filename);
  }

  bool SaveWordTopicCount(const std::string& filename) const {
    return words_topics_count_.Save(filename);
  }
};

#endif  // MODEL_H_

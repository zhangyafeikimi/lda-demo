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
#include "rand.h"
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

  Random random_;

 public:
  Model() : K_(0), hp_sum_alpha_(0.0), hp_beta_(0.0) {}

  int& K() { return K_; }
  double& alpha() { return hp_sum_alpha_; }
  double& beta() { return hp_beta_; }

  virtual void Init() {
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

    if (hp_sum_alpha_ <= 0) {
      const double avg_doc_len = words_.size() * 1.0 / M_;
      hp_alpha_.resize(K_, avg_doc_len / K_);
      hp_sum_alpha_ = avg_doc_len;
    } else {
      hp_alpha_.resize(K_, hp_sum_alpha_);
      hp_sum_alpha_ = hp_sum_alpha_ * K_;
    }

    if (hp_beta_ <= 0) {
      hp_beta_ = 0.1;
    }
    hp_sum_beta_ = V_ * hp_beta_;
  }

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

// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//

#include "model.h"
#include <fstream>
#include <string>
#include "x.h"

bool Model::LoadMVK(const std::string& filename) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  int r = 0;
  r += fscanf(fp, "M=%d\n", &M_);
  r += fscanf(fp, "V=%d\n", &V_);
  r += fscanf(fp, "K=%d\n", &K_);
  if (r != 3) {
    ERROR("Loading \"%s\" failed\n", filename.c_str());
    return false;
  }
  return true;
}

bool Model::LoadTopicsCount(const std::string& filename) {
  topics_count_.Init(K_);
  return topics_count_.Load(filename);
}

bool Model::LoadWordsTopicsCount(const std::string& filename) {
  words_topics_count_.Init(V_, K_);
  return words_topics_count_.Load(filename);
}

bool Model::LoadDocsTopicsCount(const std::string& filename) {
  docs_topics_count_.Init(M_, K_);
  return docs_topics_count_.Load(filename);
}

bool Model::LoadHPAlpha(const std::string& filename) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  int r = 0;
  hp_alpha_.resize(K_);
  for (int k = 0; k < K_; k++) {
    r += fscanf(fp, "%lg\n", &hp_alpha_[k]);
  }
  if (r != K_) {
    ERROR("Loading \"%s\" failed\n", filename.c_str());
    return false;
  }

  hp_sum_alpha_ = 0.0;
  for (int k = 0; k < K_; k++) {
    hp_sum_alpha_ += hp_alpha_[k];
  }
  return true;
}

bool Model::LoadHPBeta(const std::string& filename) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  int r = fscanf(fp, "%lg\n", &hp_beta_);
  if (r != 1) {
    ERROR("Loading \"%s\" failed\n", filename.c_str());
    return false;
  }
  hp_sum_beta_ = hp_beta_ * V_;
  return true;
}

bool Model::SaveMVK(const std::string& filename) const {
  ScopedFile fp(filename.c_str(), ScopedFile::Write);
  fprintf(fp, "M=%d\n", M_);
  fprintf(fp, "V=%d\n", V_);
  fprintf(fp, "K=%d\n", K_);
  return true;
}

bool Model::SaveTopicsCount(const std::string& filename) const {
  return topics_count_.Save(filename);
}

bool Model::SaveWordsTopicsCount(const std::string& filename) const {
  return words_topics_count_.Save(filename);
}

bool Model::SaveDocsTopicsCount(const std::string& filename) const {
  return docs_topics_count_.Save(filename);
}

bool Model::SaveHPAlpha(const std::string& filename) const {
  ScopedFile fp(filename.c_str(), ScopedFile::Write);
  for (int k = 0; k < K_; k++) {
    fprintf(fp, "%lg\n", hp_alpha_[k]);
  }
  return true;
}

bool Model::SaveHPBeta(const std::string& filename) const {
  ScopedFile fp(filename.c_str(), ScopedFile::Write);
  fprintf(fp, "%lg\n", hp_beta_);
  return true;
}

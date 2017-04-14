// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//

#include "model.h"
#include <fstream>
#include <string>
#include "x.h"

bool Model::LoadCorpus(const std::string& filename, int with_id) {
  int line_no = 0;
  char* endptr;
  char* doc_id = NULL;
  char* word_id;
  char* word_count;
  char* word_begin;
  Doc doc;
  Word word;
  int id, i, count;
  std::string line;
  std::ifstream ifs(filename.c_str());
  // ScopedFile fp(filename.c_str(), ScopedFile::Read);

  Log("Loading corpus.\n");
  V_ = 0;

  while (std::getline(ifs, line)) {
    line_no++;

    doc.index = (int)words_.size();
    doc.N = 0;

    if (with_id) {
      doc_id = strtok(&line[0], DELIMITER);
      if (doc_id == NULL) {
        Error("line %d, empty line.\n", line_no);
        continue;
      }
      word_begin = NULL;
    } else {
      word_begin = &line[0];
    }

    for (;;) {
      word_id = strtok(word_begin, DELIMITER);
      word_begin = NULL;
      if (word_id == NULL) {
        break;
      }

      word_count = strrchr(word_id, ':');
      if (word_count) {
        if (word_count == word_id) {
          Error("line %d, word id is empty.\n", line_no);
          continue;
        }
        *word_count = '\0';
        word_count++;
        count = (int)strtoll(word_count, &endptr, 10);
        if (*endptr != '\0') {
          Error("line %d, word count error \"%s\".\n", line_no, word_count);
          continue;
        }
      } else {
        count = 1;
      }

      id = (int)strtoll(word_id, &endptr, 10);
      if (*endptr != '\0') {
        Error("line %d, word id error \"%s\".\n", line_no, word_id);
        continue;
      }
      if (id < 0) {
        Error("line %d, word id must be ge than 0.\n", line_no);
        continue;
      }

      if (id >= V_) {
        V_ = id + 1;
      }
      word.v = id;
      for (i = 0; i < count; i++) {
        words_.push_back(word);
        doc.N++;
      }
    }

    if (doc.N) {
      docs_.push_back(doc);
    }
  }

  M_ = (int)docs_.size();

  if (M_ == 0) {
    Error("Loaded zero documents.\n");
    return false;
  }
  if (V_ == 0) {
    Error("Loaded an empty vocabulary.\n");
    return false;
  }

  Log("Loaded %d documents with a %d-size vocabulary.\n", M_, V_);
  return true;
}

bool Model::LoadMVK(const std::string& filename) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  int r = 0;
  r += fscanf(fp, "M=%d\n", &M_);
  r += fscanf(fp, "V=%d\n", &V_);
  r += fscanf(fp, "K=%d\n", &K_);
  if (r != 3) {
    Error("Loading \"%s\" failed\n", filename.c_str());
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
    Error("Loading \"%s\" failed\n", filename.c_str());
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
    Error("Loading \"%s\" failed\n", filename.c_str());
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

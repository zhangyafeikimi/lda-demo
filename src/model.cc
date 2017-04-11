// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//

#include "model.h"
#include <fstream>
#include <string>
#include "line-reader.h"
#include "x.h"

int LoadTable(const std::string& filename, IntTable* table) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  int id, count;
  int r;
  for (;;) {
    r = fscanf(fp, "%d\t%d\n", &id, &count);
    if (r != 2) {
      break;
    }
    // table is empty
    (*table)[id] += count;
  }
  return 0;
}

void SaveTable(const std::string& filename, const IntTable& table) {
  ScopedFile fp(filename.c_str(), ScopedFile::Write);
  IntTable::const_iterator first = table.begin();
  IntTable::const_iterator last = table.end();
  for (; first != last; ++first) {
    fprintf(fp, "%d\t%d\n", first.id(), first.count());
  }
}

int LoadTables(const std::string& filename, IntTables* tables) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  int i, id, count;
  int r;
  for (;;) {
    r = fscanf(fp, "%d\t%d\t%d\n", &i, &id, &count);
    if (r != 3) {
      break;
    }
    IntTable& table = (*tables)[i];
    table[id] += count;
  }
  return 0;
}

void SaveTables(const std::string& filename, const IntTables& tables) {
  ScopedFile fp(filename.c_str(), ScopedFile::Write);
  const int d1 = tables.d1();
  for (int i = 0; i < d1; i++) {
    const IntTable& table = tables[i];
    IntTable::const_iterator first = table.begin();
    IntTable::const_iterator last = table.end();
    for (; first != last; ++first) {
      fprintf(fp, "%d\t%d\t%d\n", i, first.id(), first.count());
    }
  }
}

int LoadArray2D(const std::string& filename, Array2D* a) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  const int d1 = a->d1();
  const int d2 = a->d2();
  int r;
  for (int i = 0; i < d1; i++) {
    r = 0;
    for (int j = 0; j < d2 - 1; j++) {
      r += fscanf(fp, "%lg ", &(*a)[i][j]);
    }
    r += fscanf(fp, "%lg\n", &(*a)[i][d2 - 1]);
    if (r != d2) {
      Error("Loading \"%s\" failed\n", filename.c_str());
      return -1;
    }
  }
  return 0;
}

void SaveArray2D(const std::string& filename, const Array2D& a) {
  ScopedFile fp(filename.c_str(), ScopedFile::Write);
  const int d1 = a.d1();
  const int d2 = a.d2();
  for (int i = 0; i < d1; i++) {
    for (int j = 0; j < d2 - 1; j++) {
      fprintf(fp, "%lg ", a[i][j]);
    }
    fprintf(fp, "%lg\n", a[i][d2 - 1]);
  }
}

int Model::LoadCorpus(const std::string& filename, int with_id) {
  LineReader line_reader;
  int line_no = 0;
  char* endptr;
  char* doc_id = NULL;
  char* word_id;
  char* word_count;
  char* word_begin;
  Doc doc;
  Word word;
  int id, i, count;
  ScopedFile fp(filename.c_str(), ScopedFile::Read);

  Log("Loading corpus.\n");
  V_ = 0;
  while (line_reader.ReadLine(fp) != NULL) {
    line_no++;

    doc.index = (int)words_.size();
    doc.N = 0;

    if (with_id) {
      doc_id = strtok(line_reader.buf, DELIMITER);
      if (doc_id == NULL) {
        Error("line %d, empty line.\n", line_no);
        continue;
      }
      word_begin = NULL;
    } else {
      word_begin = line_reader.buf;
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
      if (with_id) {
        doc_ids_.push_back(doc_id);
      }
      docs_.push_back(doc);
    }
  }

  M_ = (int)docs_.size();

  if (M_ == 0) {
    Error("Loaded zero documents.\n");
    return -1;
  }
  if (V_ == 0) {
    Error("Loaded an empty vocabulary.\n");
    return -2;
  }

  Log("Loaded %d documents with a %d-size vocabulary.\n", M_, V_);
  return 0;
}

void Model::CollectTheta(Array2D* theta) const {
  theta->Init(M_, K_);
  for (int m = 0; m < M_; m++) {
    const Doc& doc = docs_[m];
    const IntTable& doc_topics_count = docs_topics_count_[m];
    double* theta_m = (*theta)[m];
    for (int k = 0; k < K_; k++) {
      theta_m[k] =
          (doc_topics_count[k] + hp_alpha_[k]) / (doc.N + hp_sum_alpha_);
    }
  }
}

void Model::CollectPhi(Array2D* phi) const {
  phi->Init(K_, V_);
  for (int k = 0; k < K_; k++) {
    const int topics_count_k = topics_count_[k];
    double* phi_k = (*phi)[k];
    for (int v = 0; v < V_; v++) {
      phi_k[v] = (words_topics_count_[v][k] + hp_beta_) /
                 (topics_count_k + hp_sum_beta_);
    }
  }
}

int Model::LoadMVK(const std::string& filename) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  int r = 0;
  r += fscanf(fp, "M=%d\n", &M_);
  r += fscanf(fp, "V=%d\n", &V_);
  r += fscanf(fp, "K=%d\n", &K_);
  if (r != 3) {
    Error("Loading \"%s\" failed\n", filename.c_str());
    return -1;
  }
  return 0;
}

int Model::LoadTopicsCount(const std::string& filename) {
  topics_count_.InitDense(K_);
  return LoadTable(filename, &topics_count_);
}

int Model::LoadWordsTopicsCount(const std::string& filename) {
  words_topics_count_.Init(V_, K_, storage_type_);
  return LoadTables(filename, &words_topics_count_);
}

int Model::LoadDocsTopicsCount(const std::string& filename) {
  docs_topics_count_.Init(M_, K_, storage_type_);
  return LoadTables(filename, &docs_topics_count_);
}

int Model::LoadHPAlpha(const std::string& filename) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  int r = 0;
  hp_alpha_.resize(K_);
  for (int k = 0; k < K_; k++) {
    r += fscanf(fp, "%lg\n", &hp_alpha_[k]);
  }
  if (r != K_) {
    Error("Loading \"%s\" failed\n", filename.c_str());
    return -1;
  }

  hp_sum_alpha_ = 0.0;
  for (int k = 0; k < K_; k++) {
    hp_sum_alpha_ += hp_alpha_[k];
  }
  return 0;
}

int Model::LoadHPBeta(const std::string& filename) {
  ScopedFile fp(filename.c_str(), ScopedFile::Read);
  int r = fscanf(fp, "%lg\n", &hp_beta_);
  if (r != 1) {
    Error("Loading \"%s\" failed\n", filename.c_str());
    return -1;
  }
  hp_sum_beta_ = hp_beta_ * V_;
  return 0;
}

void Model::SaveMVK(const std::string& filename) const {
  ScopedFile fp(filename.c_str(), ScopedFile::Write);
  fprintf(fp, "M=%d\n", M_);
  fprintf(fp, "V=%d\n", V_);
  fprintf(fp, "K=%d\n", K_);
}

void Model::SaveTopicsCount(const std::string& filename) const {
  SaveTable(filename, topics_count_);
}

void Model::SaveWordsTopicsCount(const std::string& filename) const {
  SaveTables(filename, words_topics_count_);
}

void Model::SaveDocsTopicsCount(const std::string& filename) const {
  SaveTables(filename, docs_topics_count_);
}

void Model::SaveHPAlpha(const std::string& filename) const {
  ScopedFile fp(filename.c_str(), ScopedFile::Write);
  for (int k = 0; k < K_; k++) {
    fprintf(fp, "%lg\n", hp_alpha_[k]);
  }
}

void Model::SaveHPBeta(const std::string& filename) const {
  ScopedFile fp(filename.c_str(), ScopedFile::Write);
  fprintf(fp, "%lg\n", hp_beta_);
}

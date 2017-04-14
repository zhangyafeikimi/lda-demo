// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//

#include "corpus.h"
#include <string.h>
#include <fstream>
#include "x.h"

#if defined _MSC_VER
#define strtoll _strtoi64
#endif

bool Corpus::LoadCorpus(const std::string& filename, bool doc_with_id) {
  std::ifstream ifs(filename.c_str());
  if (!ifs.is_open()) {
    ERROR("Failed to open \"%s\".", filename.c_str());
    return false;
  }

  int line_no = 0;
  char* endptr;
  char* doc_id = nullptr;
  char* word_id;
  char* word_count;
  char* word_begin;
  Doc doc;
  Word word;
  int id, i, count;
  std::string line;

  INFO("Loading corpus from \"%s\".", filename.c_str());
  V_ = 0;

  while (std::getline(ifs, line)) {
    line_no++;
    doc.index = static_cast<int>(words_.size());
    doc.N = 0;

    if (doc_with_id) {
      doc_id = strtok(&line[0], " \t|\n");
      if (doc_id == nullptr) {
        continue;
      }
      word_begin = nullptr;
    } else {
      word_begin = &line[0];
    }

    for (;;) {
      word_id = strtok(word_begin, " \t|\n");
      word_begin = nullptr;
      if (word_id == nullptr) {
        break;
      }

      word_count = strrchr(word_id, ':');
      if (word_count) {
        if (word_count == word_id) {
          ERROR("line %d, word id is empty.", line_no);
          continue;
        }
        *word_count = '\0';
        word_count++;
        count = static_cast<int>(strtoll(word_count, &endptr, 10));
        if (*endptr != '\0') {
          ERROR("line %d, word count error \"%s\".", line_no, word_count);
          continue;
        }
      } else {
        count = 1;
      }

      id = static_cast<int>(strtoll(word_id, &endptr, 10));
      if (*endptr != '\0') {
        ERROR("line %d, word id error \"%s\".", line_no, word_id);
        continue;
      }
      if (id < 0) {
        ERROR("line %d, word id must be ge than 0.", line_no);
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

    if (doc.N != 0) {
      docs_.push_back(doc);
    }
  }

  M_ = static_cast<int>(docs_.size());
  if (M_ == 0) {
    ERROR("Loaded an empty corpus.");
    return false;
  }
  if (V_ == 0) {
    ERROR("Loaded an empty vocabulary.");
    return false;
  }
  INFO("Loaded %d documents, %d unique words.", M_, V_);
  return true;
}

// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// line reader
//

#ifndef LINE_READER_H_
#define LINE_READER_H_

#include "x.h"

struct LineReader {
 private:
  size_t len_;

 public:
  char* buf;

  LineReader() : len_(4096) { buf = _Malloc(char, len_); }

  ~LineReader() { free(buf); }

  char* ReadLine(FILE* fp) {
    size_t new_len;
    if (fgets(buf, (int)len_, fp) == NULL) {
      return NULL;
    }

    while (strrchr(buf, '\n') == NULL) {
      len_ *= 2;
      buf = _Realloc(buf, char, len_);
      new_len = strlen(buf);
      if (fgets(buf + new_len, (int)(len_ - new_len), fp) == NULL) {
        break;
      }
    }
    return buf;
  }
};

#endif  // LINE_READER_H_

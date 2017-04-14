// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// common utils
//

#ifndef X_H_
#define X_H_

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined _NDEBUG || defined NDEBUG
#define DEBUG(...)
#else
#define DEBUG(...)                                                          \
  do {                                                                      \
    time_t t;                                                               \
    struct tm* tm_info;                                                     \
    time(&t);                                                               \
    tm_info = localtime(&t);                                                \
    fprintf(stderr, "[%.4d-%.2d-%.2d %.2d:%.2d:%.2d]",                      \
            1900 + tm_info->tm_year, 1 + tm_info->tm_mon, tm_info->tm_mday, \
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);            \
    fprintf(stderr, "[DEBUG]");                                             \
    fprintf(stderr, __VA_ARGS__);                                           \
    fprintf(stderr, "\n");                                                  \
    fflush(stderr);                                                         \
  } while (0)
#endif

#define INFO(...)                                                           \
  do {                                                                      \
    time_t t;                                                               \
    struct tm* tm_info;                                                     \
    time(&t);                                                               \
    tm_info = localtime(&t);                                                \
    fprintf(stderr, "[%.4d-%.2d-%.2d %.2d:%.2d:%.2d]",                      \
            1900 + tm_info->tm_year, 1 + tm_info->tm_mon, tm_info->tm_mday, \
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);            \
    fprintf(stderr, "[INFO]");                                              \
    fprintf(stderr, __VA_ARGS__);                                           \
    fprintf(stderr, "\n");                                                  \
    fflush(stderr);                                                         \
  } while (0)

#define ERROR(...)                                                          \
  do {                                                                      \
    time_t t;                                                               \
    struct tm* tm_info;                                                     \
    time(&t);                                                               \
    tm_info = localtime(&t);                                                \
    fprintf(stderr, "[%.4d-%.2d-%.2d %.2d:%.2d:%.2d]",                      \
            1900 + tm_info->tm_year, 1 + tm_info->tm_mon, tm_info->tm_mday, \
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);            \
    fprintf(stderr, "[ERROR]");                                             \
    fprintf(stderr, __VA_ARGS__);                                           \
    fprintf(stderr, "\n");                                                  \
    fflush(stderr);                                                         \
  } while (0)

#define CHECK(expr)                   \
  do {                                \
    auto r = expr;                    \
    if (!r) {                         \
      ERROR("\"%s\" failed.", #expr); \
      exit(1);                        \
    }                                 \
  } while (0)

#define MISSING_ARG(argc, argv, i) ERROR("\"%s\" wants a value.", argv[i])

#define COMSUME_1_ARG(argc, argv, i)     \
  do {                                   \
    for (int j = i; j < argc - 1; j++) { \
      argv[j] = argv[j + 1];             \
    }                                    \
    argc -= 1;                           \
  } while (0)

#define COMSUME_2_ARG(argc, argv, i)     \
  do {                                   \
    for (int j = i; j < argc - 2; j++) { \
      argv[j] = argv[j + 2];             \
    }                                    \
    argc -= 2;                           \
  } while (0)

#define CHECK_MISSING_ARG(argc, argv, i, action) \
  do {                                           \
    if (i + 1 == argc) {                         \
      MISSING_ARG(argc, argv, i);                \
      action;                                    \
    }                                            \
  } while (0)

inline FILE* xfopen(const char* filename, const char* mode) {
  FILE* fp = fopen(filename, mode);
  if (fp == nullptr) {
    ERROR("Open \"%s\" failed.\n", filename);
    exit(1);
  }
  return fp;
}

inline double xatod(const char* str) {
  char* endptr;
  double d;
  errno = 0;
  d = strtod(str, &endptr);
  if (errno != 0 || str == endptr) {
    ERROR("%s is not a double.\n", str);
    exit(1);
  }
  return d;
}

inline int xatoi(const char* str) {
  char* endptr;
  int i;
  errno = 0;
  i = (int)strtol(str, &endptr, 10);
  if (errno != 0 || str == endptr) {
    ERROR("%s is not an integer.\n", str);
    exit(1);
  }
  return i;
}

class ScopedFile {
 private:
  FILE* px_;
  ScopedFile(const ScopedFile&);
  ScopedFile& operator=(const ScopedFile&);

 public:
  enum Mode {
    Read = 0,
    Write,
    ReadBinary,
    WriteBinary,
  };

  ScopedFile() : px_(nullptr) {}

  ScopedFile(FILE* p) : px_(p) {}  // NOLINT

  ScopedFile(const char* filename, Mode mode) {
    static const char* mode_map[] = {"r", "w", "rb", "wb"};
    if (strcmp(filename, "-") == 0) {
      if (mode == Read || mode == ReadBinary) {
        px_ = stdin;
      } else {
        px_ = stdout;
      }
    } else {
      px_ = xfopen(filename, mode_map[mode]);
    }
  }

  ~ScopedFile() { Close(); }

  void Close() {
    if (px_ == stdin || px_ == stdout || px_ == stderr) {
      px_ = nullptr;
      return;
    }

    if (px_) {
      fclose(px_);
      px_ = nullptr;
    }
  }

  operator FILE*() { return px_; }
  operator const FILE*() const { return px_; }
};

#endif  // X_H_

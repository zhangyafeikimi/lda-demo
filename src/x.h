// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// common utils
//

#ifndef X_H_
#define X_H_

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

inline void __LOG(const char* file, int line, const char* level,
                  const char* format, ...) {
  char buf[1024 * 10];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  time_t t;
  struct tm* tm_info;
  time(&t);
  tm_info = localtime(&t);
  fprintf(stderr, "[%.4d-%.2d-%.2d %.2d:%.2d:%.2d %s:%d][%s]%s\n",
          1900 + tm_info->tm_year, 1 + tm_info->tm_mon, tm_info->tm_mday,
          tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, file, line, level,
          buf);
  fflush(stderr);
}

#define INFO(...) __LOG(__FILE__, __LINE__, "[INFO]", __VA_ARGS__);
#define ERROR(...) __LOG(__FILE__, __LINE__, "[ERROR]", __VA_ARGS__);

#define CHECK(expr)                         \
  do {                                      \
    auto r = expr;                          \
    if (!r) {                               \
      ERROR("\"%s\" check failed.", #expr); \
      exit(1);                              \
    }                                       \
  } while (0)

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

#define CHECK_MISSING_ARG(argc, argv, i, action)  \
  do {                                            \
    if (i + 1 == argc) {                          \
      ERROR("\"%s\" requires a value.", argv[i]); \
      action;                                     \
    }                                             \
  } while (0)

inline double xatod(const char* str) {
  char* endptr;
  double d;
  errno = 0;
  d = strtod(str, &endptr);
  if (errno != 0 || str == endptr) {
    ERROR("\"%s\" is not a double.", str);
    exit(1);
  }
  return d;
}

inline int xatoi(const char* str) {
  char* endptr;
  int i;
  errno = 0;
  i = static_cast<int>(strtol(str, &endptr, 10));
  if (errno != 0 || str == endptr) {
    ERROR("\"%s\" is not an integer.", str);
    exit(1);
  }
  return i;
}

inline bool PrintDone() {
  INFO("Done.");
  return true;
}

#endif  // X_H_

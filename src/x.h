// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// common utils
//

#ifndef X_H_
#define X_H_

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
  fprintf(stderr, "%s %.2d:%.2d:%.2d %s:%d %s\n", level, tm_info->tm_hour,
          tm_info->tm_min, tm_info->tm_sec, file, line, buf);
  fflush(stderr);
}

#define INFO(...) __LOG(__FILE__, __LINE__, "I", __VA_ARGS__);
#define ERROR(...) __LOG(__FILE__, __LINE__, "E", __VA_ARGS__);
#define CHECK(expr)                         \
  do {                                      \
    auto r = expr;                          \
    if (!r) {                               \
      ERROR("\"%s\" check failed.", #expr); \
      exit(1);                              \
    }                                       \
  } while (0)

#endif  // X_H_

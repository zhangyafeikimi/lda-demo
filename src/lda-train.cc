// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// LDA train main
//

#include <errno.h>
#include <stdlib.h>
#include <string>
#include "sampler.h"
#include "x.h"

namespace {

// input options
int doc_with_id;
std::string input_corpus_filename;

// output options
std::string output_prefix;

// sampler options
std::string sampler = "lightlda";
int K = 10;
double alpha = 0.1;
double beta = 0.1;
int hp_opt = 0;
int hp_opt_interval = 5;
double hp_opt_alpha_shape = 0.0;
double hp_opt_alpha_scale = 100000.0;
int hp_opt_alpha_iteration = 2;
int hp_opt_beta_iteration = 200;
int total_iteration = 200;
int burnin_iteration = 10;
int log_likelihood_interval = 10;
int mh_step = 2;
int enable_word_proposal = 1;
int enable_doc_proposal = 1;

void Usage() {
  fprintf(
      stderr,
      "Usage: lda-train [options] INPUT_FILE [OUTPUT_PREFIX]\n"
      "  INPUT_FILE: input corpus filename.\n"
      "  OUTPUT_PREFIX: output filename prefix.\n"
      "    Default is the same as INPUT_FILE.\n"
      "\n"
      "  Options:\n"
      "    -doc_with_id 0/1\n"
      "      Whether the first column of INPUT_FILE is doc ID, and skip it.\n"
      "      Default is \"%d\".\n"
      "    -sampler lda/sparselda/aliaslda/lightlda\n"
      "      Different sampling algorithms.\n"
      "      Default is \"%s\".\n"
      "    -K TOPIC\n"
      "      Number of topics.\n"
      "      Default is \"%d\".\n"
      "    -alpha A\n"
      "      Doc-topic prior. "
      "0.0 enables a smart prior according to corpus.\n"
      "      Default is \"%lg\".\n"
      "    -beta B\n"
      "      Topic-word prior.\n"
      "      Default is \"%lg\".\n"
      "    -hp_opt 0/1\n"
      "      Whether to optimize ALPHA and BETA.\n"
      "      Default is \"%d\".\n"
      "    -hp_opt_interval INTERVAL\n"
      "      Interval of optimizing hyper parameters.\n"
      "      Default is \"%d\".\n"
      "    -hp_opt_alpha_shape SHAPE\n"
      "      One parameter used in optimizing ALPHA.\n"
      "      Default is \"%lg\".\n"
      "    -hp_opt_alpha_scale SCALE\n"
      "      One parameter used in optimizing ALPHA.\n"
      "      Default is \"%lg\".\n"
      "    -hp_opt_alpha_iteration ITER\n"
      "      Iterations of optimization ALPHA. 0 disables it.\n"
      "      Default is \"%d\".\n"
      "    -hp_opt_beta_iteration ITER\n"
      "      Iterations of optimization BETA. 0 disables it.\n"
      "      Default is \"%d\".\n"
      "    -total_iteration ITER\n"
      "      Iterations of training.\n"
      "      Default is \"%d\".\n"
      "    -burnin_iteration ITER\n"
      "      Iterations of burn in period,\n"
      "      during which both optimization of ALPHA and BETA\n"
      "      and log likelihood are disabled. 0 disables it.\n"
      "      Default is \"%d\".\n"
      "    -log_likelihood_interval INTERVAL\n"
      "      Interval of calculating log likelihood. 0 disables it.\n"
      "      Default is \"%d\".\n"
      "    -mh_step MH_STEP\n"
      "      Number of MH steps(sampler=aliaslda/lightlda).\n"
      "      Default is \"%d\".\n"
      "    -enable_word_proposal 0/1\n"
      "      Enable word proposal(sampler=lightlda).\n"
      "      Default is \"%d\".\n"
      "    -enable_doc_proposal 0/1\n"
      "      Enable doc proposal(sampler=lightlda).\n"
      "      Default is \"%d\".\n",
      doc_with_id, sampler.c_str(), K, alpha, beta, hp_opt, hp_opt_interval,
      hp_opt_alpha_shape, hp_opt_alpha_scale, hp_opt_alpha_iteration,
      hp_opt_beta_iteration, total_iteration, burnin_iteration,
      log_likelihood_interval, mh_step, enable_word_proposal,
      enable_doc_proposal);
  exit(1);
}

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

void ParseArgs(int argc, char** argv) {
  if (argc == 1) {
    Usage();
  }

  int i = 1;
  for (;;) {
    std::string s = argv[i];
    if (s == "-h" || s == "-help" || s == "--help") {
      Usage();
    }

    if (s.size() >= 2 && s[0] == '-' && s[1] == '-') {
      s.erase(s.begin());
    }

    if (s == "-doc_with_id") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      doc_with_id = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-sampler") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      sampler = argv[i + 1];
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-K") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      K = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-alpha") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      alpha = xatod(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-beta") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      beta = xatod(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-hp_opt") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      hp_opt = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-hp_opt_interval") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      hp_opt_interval = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-hp_opt_alpha_shape") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      hp_opt_alpha_shape = xatod(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-hp_opt_alpha_scale") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      hp_opt_alpha_scale = xatod(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-hp_opt_alpha_iteration") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      hp_opt_alpha_iteration = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-hp_opt_beta_iteration") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      hp_opt_beta_iteration = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-total_iteration") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      total_iteration = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-burnin_iteration") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      burnin_iteration = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-log_likelihood_interval") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      log_likelihood_interval = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-mh_step") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      mh_step = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-enable_word_proposal") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      enable_word_proposal = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else if (s == "-enable_doc_proposal") {
      CHECK_MISSING_ARG(argc, argv, i, Usage());
      enable_doc_proposal = xatoi(argv[i + 1]);
      COMSUME_2_ARG(argc, argv, i);
    } else {
      i++;
    }
    if (i == argc) {
      break;
    }
  }

  if (argc == 1) {
    Usage();
  }

  CHECK(doc_with_id == 0 || doc_with_id == 1);
  CHECK(sampler == "lda" || sampler == "sparselda" || sampler == "aliaslda" ||
        sampler == "lightlda");
  CHECK(K >= 2);
  CHECK(alpha >= 0.0);
  CHECK(beta > 0.0);
  CHECK(hp_opt >= 0 && hp_opt <= 1);
  CHECK(hp_opt_interval > 0);
  CHECK(hp_opt_alpha_shape >= 0.0);
  CHECK(hp_opt_alpha_scale > 0.0);
  CHECK(hp_opt_alpha_iteration >= 0);
  CHECK(hp_opt_beta_iteration >= 0);
  CHECK(total_iteration > 0);
  CHECK(burnin_iteration >= 0);
  CHECK(total_iteration > burnin_iteration);
  CHECK(log_likelihood_interval >= 0);
  if (sampler == "aliaslda" || sampler == "lightlda") {
    CHECK(mh_step > 0);
  }
  if (sampler == "lightlda") {
    CHECK(enable_word_proposal >= 0 && enable_word_proposal <= 1);
    CHECK(enable_doc_proposal >= 0 && enable_doc_proposal <= 1);
    CHECK(enable_word_proposal + enable_doc_proposal != 0);
  }

  input_corpus_filename = argv[1];
  if (argc >= 3) {
    output_prefix = argv[2];
  } else {
    output_prefix = input_corpus_filename;
  }
}

template <class Sampler>
void Train(Sampler* p) {
  p->K() = K;
  p->alpha() = alpha;
  p->beta() = beta;
  p->hp_opt() = hp_opt;
  p->hp_opt_interval() = hp_opt_interval;
  p->hp_opt_alpha_shape() = hp_opt_alpha_shape;
  p->hp_opt_alpha_scale() = hp_opt_alpha_scale;
  p->hp_opt_alpha_iteration() = hp_opt_alpha_iteration;
  p->hp_opt_beta_iteration() = hp_opt_beta_iteration;
  p->total_iteration() = total_iteration;
  p->burnin_iteration() = burnin_iteration;
  p->log_likelihood_interval() = log_likelihood_interval;

  CHECK(p->LoadCorpus(input_corpus_filename, doc_with_id != 0));
  p->Train();
  CHECK(p->SaveModel(output_prefix));
  delete p;
}

}  // namespace

int main(int argc, char** argv) {
  ParseArgs(argc, argv);

  if (sampler == "lda") {
    GibbsSampler* p = new GibbsSampler();
    Train(p);
  } else if (sampler == "sparselda") {
    SparseLDASampler* p = new SparseLDASampler();
    Train(p);
  } else if (sampler == "aliaslda") {
    AliasLDASampler* p = new AliasLDASampler();
    p->mh_step() = mh_step;
    Train(p);
  } else if (sampler == "lightlda") {
    LightLDASampler* p = new LightLDASampler();
    p->mh_step() = mh_step;
    p->enable_word_proposal() = enable_word_proposal;
    p->enable_doc_proposal() = enable_doc_proposal;
    Train(p);
  }
  return 0;
}

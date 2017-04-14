// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// LDA train main
//

#include <string>
#include "sampler.h"
#include "x.h"

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

// LightLDASampler options
int mh_step = 8;
int enable_word_proposal = 1;
int enable_doc_proposal = 1;

void Usage() {
  fprintf(stderr,
          "Usage: lda-train [options] INPUT_FILE [OUTPUT_PREFIX]\n"
          "  INPUT_FILE: input corpus filename.\n"
          "  OUTPUT_PREFIX: output filename prefix.\n"
          "    Default is the same as INPUT_FILE.\n"
          "\n"
          "  Options:\n"
          "    -doc_with_id 0/1\n"
          "      The first column of INPUT_FILE is doc ID.\n"
          "      Default is \"%d\".\n"
          "    -sampler SAMPLER\n"
          "      SAMPLER can be lda, sparselda, aliaslda, lightlda.\n"
          "      Default is \"%s\".\n"
          "    -K TOPIC\n"
          "      Number of topics.\n"
          "      Default is \"%d\".\n"
          "    -alpha ALPHA\n"
          "      Doc-topic prior. "
          "0.0 enables a smart prior according to corpus.\n"
          "      Default is \"%lg\".\n"
          "    -beta BETA\n"
          "      Topic-word prior.\n"
          "      Default is \"%lg\".\n"
          "    -hp_opt 0/1\n"
          "      Whether to optimize ALHPA and BETA.\n"
          "      Default is \"%d\".\n"
          "    -hp_opt_interval INTERVAL\n"
          "      Interval of optimizing hyper parameters.\n"
          "      Default is \"%d\".\n"
          "    -hp_opt_alpha_shape SHAPE\n"
          "      One parameter used in optimizing ALHPA.\n"
          "      Default is \"%lg\".\n"
          "    -hp_opt_alpha_scale SCALE\n"
          "      One parameter used in optimizing ALHPA.\n"
          "      Default is \"%lg\".\n"
          "    -hp_opt_alpha_iteration ITERATION\n"
          "      Iterations of optimization ALPHA. 0 disables it.\n"
          "      Default is \"%d\".\n"
          "    -hp_opt_beta_iteration ITERATION\n"
          "      Iterations of optimization BETA. 0 disables it.\n"
          "      Default is \"%d\".\n"
          "    -total_iteration ITERATION\n"
          "      Iterations of scanning corpus.\n"
          "      Default is \"%d\".\n"
          "    -burnin_iteration ITERATION\n"
          "      Iterations of burn in period,\n"
          "      during which neither optimization of ALPHA and BETA\n"
          "      nor log likelihood are disabled. 0 disables it.\n"
          "      Default is \"%d\".\n"
          "    -log_likelihood_interval INTERVAL\n"
          "      Interval of calculating log likelihood. 0 disables it.\n"
          "      Default is \"%d\".\n"
          "    -mh_step MH_STEP\n"
          "      Number of MH steps(aliaslda or lightlda).\n"
          "      Default is \"%d\".\n"
          "    -enable_word_proposal 0/1\n"
          "      Enable word proposal(lightlda).\n"
          "      Default is \"%d\".\n"
          "    -enable_doc_proposal 0/1\n"
          "      Enable doc proposal(lightlda).\n"
          "      Default is \"%d\".\n",
          doc_with_id, sampler.c_str(), K, alpha, beta, hp_opt, hp_opt_interval,
          hp_opt_alpha_shape, hp_opt_alpha_scale, hp_opt_alpha_iteration,
          hp_opt_beta_iteration, total_iteration, burnin_iteration,
          log_likelihood_interval, mh_step, enable_word_proposal,
          enable_doc_proposal);
  exit(1);
}

int main(int argc, char** argv) {
  if (argc == 1) {
    Usage();
  }

  int i = 1;
  for (;;) {
    std::string s = argv[i];
    if (s == "-h" || s == "-help" || s == "--help") {
      Usage();
    }

    if (strncmp(s.c_str(), "--", 2) == 0) {
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

#define CHECK_EXIT(condition)               \
  do {                                      \
    if (!(condition)) {                     \
      ERROR("Must have: %s", #condition); \
      exit(1);                              \
    }                                       \
  } while (0)

  CHECK_EXIT(doc_with_id >= 0 && doc_with_id <= 1);
  CHECK_EXIT(sampler == "lda" || sampler == "sparselda" ||
             sampler == "aliaslda" || sampler == "lightlda");
  CHECK_EXIT(K >= 2);
  CHECK_EXIT(alpha >= 0.0);
  CHECK_EXIT(beta > 0.0);
  CHECK_EXIT(hp_opt >= 0 && hp_opt <= 1);
  CHECK_EXIT(hp_opt_interval > 0);
  CHECK_EXIT(hp_opt_alpha_shape >= 0.0);
  CHECK_EXIT(hp_opt_alpha_scale > 0.0);
  CHECK_EXIT(hp_opt_alpha_iteration >= 0);
  CHECK_EXIT(hp_opt_beta_iteration >= 0);
  CHECK_EXIT(total_iteration > 0);
  CHECK_EXIT(burnin_iteration >= 0);
  CHECK_EXIT(total_iteration > burnin_iteration);
  CHECK_EXIT(log_likelihood_interval >= 0);
  CHECK_EXIT(mh_step > 0);
  CHECK_EXIT(enable_word_proposal >= 0 && enable_word_proposal <= 1);
  CHECK_EXIT(enable_doc_proposal >= 0 && enable_doc_proposal <= 1);
  CHECK_EXIT(enable_word_proposal + enable_doc_proposal != 0);

  input_corpus_filename = argv[1];
  if (argc >= 3) {
    output_prefix = argv[2];
  } else {
    output_prefix = input_corpus_filename;
  }

  SamplerBase* p = nullptr;
  if (sampler == "lda") {
    p = new GibbsSampler();
  } else if (sampler == "sparselda") {
    p = new SparseLDASampler();
  } else if (sampler == "aliaslda") {
    AliasLDASampler* pp = new AliasLDASampler();
    pp->mh_step() = mh_step;
    p = pp;
  } else if (sampler == "lightlda") {
    LightLDASampler* pp = new LightLDASampler();
    pp->mh_step() = mh_step;
    pp->enable_word_proposal() = enable_word_proposal;
    pp->enable_doc_proposal() = enable_doc_proposal;
    p = pp;
  }

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
  p->SaveModel(output_prefix);
  delete p;
  return 0;
}

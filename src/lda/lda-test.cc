// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// lda tests
//

#include "lda/alias.h"
#include "lda/sampler.h"
#include "lda/x.h"

#define TEST_DATA_DIR "../src/lda-test-data"

void TestAlias() {
  std::vector<double> prob;
  prob.push_back(0.01);
  prob.push_back(0.07);
  prob.push_back(0.13);
  prob.push_back(0.29);
  prob.push_back(0.45);
  prob.push_back(0.05);

  Alias alias;
  alias.Build(prob, 1.0);

  std::vector<int> count(alias.n());
  const int N = 10000;
  for (int i = 0; i < N; i++) {
    count[alias.Sample()]++;
  }
  for (int i = 0; i < alias.n(); i++) {
    Log("%lf should be %lf\n", count[i] / (double)N, prob[i]);
  }
}

void TestYahoo() {
  // GibbsSampler sampler;
  // SparseLDASampler sampler;
  // AliasLDASampler sampler;
  LightLDASampler sampler;
  sampler.mh_step() = 16;
  sampler.LoadCorpus(TEST_DATA_DIR "/yahoo-train", 0);
  sampler.K() = 3;
  sampler.alpha() = 0.1;
  sampler.beta() = 0.1;
  sampler.burnin_iteration() = 0;
  sampler.log_likelihood_interval() = 1;
  sampler.total_iteration() = 100;
  sampler.hp_opt() = 0;
  sampler.storage_type() = kHashTable;
  sampler.Train();
  sampler.SaveModel(TEST_DATA_DIR "/yahoo");
}

void TestInferYahoo() {
  // GibbsSampler sampler;
  // AliasLDASampler sampler;
  LightLDASampler sampler;
  sampler.LoadModel(TEST_DATA_DIR "/yahoo");
  sampler.InitializeInfer();

  int word_ids1[] = {
      756,  3608, 3154, 2661, 1110, 1111, 756,  3021, 2395, 95,   482,  1893,
      137,  2217, 717,  2482, 122,  20,   597,  757,  779,  1846, 3607, 3078,
      481,  3324, 1652, 2321, 866,  1650, 938,  3307, 3161, 113,  757,  1361,
      3062, 2611, 2421, 3435, 2224, 1763, 2592, 3435, 1457, 113,  2200, 2321,
      3208, 2391, 713,  2696, 885,  2200, 3208, 1908, 1090, 2977, 2395, 2626,
      756,  95,   3281, 1107, 3402, 543,  3452, 2634, 757,  3238, 479,  1168};
  int word_ids2[] = {
      509,  1422, 625,  2035, 3103, 1422, 2191, 1422, 3387, 1422, 2837, 1422,
      625,  3312, 2035, 3613, 813,  2282, 569,  508,  400,  2774, 1618, 2529,
      2488, 625,  3172, 3212, 2889, 1416, 722,  2867, 1387, 2622, 2186, 1855,
      1715, 1524, 1328, 2324, 350,  2932, 1087, 2404, 364,  3212, 2322, 1467,
      3495, 2969, 3238, 1893, 3186, 1184, 2812, 58,   508,  1618, 2497, 1422,
      452,  2306, 23,   399,  341,  895,  994,  1855, 3019, 3447, 2865, 1133,
      279,  2450, 625,  1422, 929,  1539, 636,  207,  1036, 2309, 798,  2554,
      3571, 2700, 2309, 1893, 3377, 1573, 2317, 2812, 1196, 648,  136,  928,
      627,  1758, 1679, 2474, 1837, 1291, 2195, 1380, 1855, 1784, 1082, 1834,
      1743, 3546, 2638, 2644, 2825, 213,  355,  1422, 625,  3212, 2985, 2631,
      3447, 1138, 2812, 2549, 1822, 77,   77,   625,  1422, 1097, 2812, 1629,
      2683, 341,  2549, 493,  2046, 404,  1148, 218,  1490, 985,  2440, 625,
      1133, 985,  811,  3164, 1422, 3212, 2990, 2093, 2920, 2236, 2014, 3243,
      92,   2670, 2028, 3289, 2343, 1422, 1028, 2812, 341,  3212, 2990, 1938,
      2085, 1715, 1524, 3479, 3104, 2836, 1846, 475,  2324, 2812, 1426, 2630,
      478,  738,  1524, 1846, 1618, 334,  350,  1089, 1429, 2867, 625,  522,
      2708, 3212, 2919, 3498, 2269, 2324, 3088, 254,  597,  1387, 2368, 2867,
      819,  2324, 1422, 1817, 122,  1807, 1819, 2324, 3297, 2465, 320,  1805,
      2647, 2197, 2197, 2919, 2324, 1422, 3088, 640,  3493, 3578, 792,  3212,
      527,  1935, 3023};
  int word_ids3[] = {
      642,  3410, 356,  829,  2744, 1495, 265,  2744, 1416, 655,  3038, 2537,
      745,  2287, 3212, 2775, 582,  2335, 1807, 991,  1318, 760,  3326, 2744,
      1416, 3074, 2429, 1229, 2865, 1698, 1909, 3391, 270,  2075, 1714, 1996,
      3016, 1691, 1871, 2092, 3165, 3208, 3577, 308,  2422, 1635, 1221, 991,
      2843, 3282, 390,  3074, 700,  2315, 1050, 3266, 1202, 524,  1793, 3560,
      1416, 1617, 3074, 1094, 27,   2,    642,  2401, 1793, 1509, 1575, 745,
      1416, 418,  1962, 3479, 2485, 2708, 1317, 2453, 3523, 2940, 764,  2335,
      1807, 640,  991,  760,  1221, 1115, 3479, 760,  1184, 3471, 2853, 2,
      642,  3410, 356,  26,   2555, 1668, 2992, 2895, 2477, 2328, 1416, 2927,
      2932, 2991, 2477, 1458, 2932, 1276, 2315, 2896, 1573, 3256, 1084, 3582,
      307,  3208, 24,   147,  1962, 3479, 2285, 2287, 1909, 2529, 2178, 2744,
      3479, 3119, 3391, 1866, 2138, 579,  2730, 2873, 1938, 3554, 643,  823,
      3091};
  int topic;
  sampler.InferDocument(word_ids1, DIM(word_ids1), &topic);
  Log("Inferred topic: %d\n", topic);
  sampler.InferDocument(word_ids2, DIM(word_ids2), &topic);
  Log("Inferred topic: %d\n", topic);
  sampler.InferDocument(word_ids3, DIM(word_ids3), &topic);
  Log("Inferred topic: %d\n", topic);
  Log("They should cover 0, 1, 2.\n");
}

void TestNIPS() {
  LightLDASampler sampler;
  sampler.mh_step() = 8;
  sampler.LoadCorpus(TEST_DATA_DIR "/nips-train", 0);
  sampler.K() = 50;
  sampler.alpha() = 0.1;
  sampler.beta() = 0.1;
  sampler.burnin_iteration() = 0;
  sampler.log_likelihood_interval() = 10;
  sampler.total_iteration() = 50;
  sampler.hp_opt() = 0;
  sampler.storage_type() = kHashTable;
  sampler.Train();
  sampler.SaveModel(TEST_DATA_DIR "/nips");
}

int main() {
  TestYahoo();
  TestInferYahoo();
  // TestNIPS();
  // TestAlias();
  return 0;
}

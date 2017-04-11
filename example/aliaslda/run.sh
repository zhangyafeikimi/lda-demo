#! /bin/bash

common_opt="-hp_opt 0 -K 3 -alpha 0.1 -beta 0.1 -total_iteration 200 -burnin_iteration 0 -log_likelihood_interval 10"

../../lda-train $common_opt -sampler aliaslda ../train aliaslda

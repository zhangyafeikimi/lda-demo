#! /bin/bash

cd $(dirname $0)
common_opt="-hp_opt 0 -K 3 -alpha 0.1 -beta 0.1 -total_iteration 200 -burnin_iteration 0 -log_likelihood_interval 10"

../../lda-train $common_opt -sampler lightlda -mh_step 2 ../train lightlda-mh2
../../lda-train $common_opt -sampler lightlda -mh_step 2 -hp_opt 1 ../train lightlda-mh2-hpopt
../../lda-train $common_opt -sampler lightlda -mh_step 2 -doc_with_id 1 \
    ../train-with-id lightlda-mh2-with-id
../../lda-train $common_opt -sampler lightlda -mh_step 4 ../train lightlda-mh4
../../lda-train $common_opt -sampler lightlda -mh_step 8 ../train lightlda-mh8

#!/bin/bash

./convert_histo.pl 1
#./run_det_sys.pl  #seg faults for some reason
./run_xf_sys.pl
xs_cov_matrix -r17
merge_hist -r0 -l0 -e2 | tee ../mc_stat/xs_tot.log
merge_hist_xs -r0 -l0

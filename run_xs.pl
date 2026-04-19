#!/usr/bin/perl


#central histogram preparation ...// Xs mode ...
system("./convert_histo.pl 1");

#Det sys + Flux sys run in parallel (independent: write to DetVar/ and XsFlux/ respectively)
system("./run_det_sys.pl &");
system("./run_xf_sys.pl &");
wait();

#Xs sys, POT, target nucleon ... cov_xs.root ...
system("./bin/xs_cov_matrix -r17");

# MC stat ... at M ...
system("./bin/merge_hist -r0 -l0 -e2 > ./mc_stat/xs_tot.log");

#central files ...
system("./bin/merge_hist_xs -r0 -l0");

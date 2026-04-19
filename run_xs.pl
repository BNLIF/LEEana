#!/usr/bin/perl


#central histogram preparation ...// Xs mode ...
system("./convert_histo.pl 1");

#Det sys
system("./run_det_sys.pl");

#Flux sys, GEANT4 1-->16, plus UBGenieFluxSmallUni (knob 17)
system("./run_xf_sys.pl");

#Xs sys, POT, target nucleon ... cov_xs.root ...
system("./bin/xs_cov_matrix -r17");

# MC stat ... at M ...
system("./bin/merge_hist -r0 -l0 -e2 > ./mc_stat/xs_tot.log");

#central files ...
system("./bin/merge_hist_xs -r0 -l0");

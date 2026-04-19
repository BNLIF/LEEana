#!/usr/bin/perl


#central histogram preparation ...
system("./convert_histo.pl");

#data statistical uncertainties
system("./bin/stat_pred_cov_matrix -r0"); # L-14 fix: run synchronously before merge_hist needs it

#Det sys
system("./run_det_sys.pl");

#data statistical uncertainties
# system("./bin/stat_cov_matrix -r0 &");

#Flux sys, GEANT4 1-->17
system("./run_xf_sys.pl");

# MC stat ... 
system("./bin/merge_hist -r0 -l0 -e2 > ./mc_stat/0.log");

#central files ...
system("./bin/merge_hist -r0 -l0");

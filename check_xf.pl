#!/usr/bin/perl

open(infile,"./configurations/xf_file.txt");
while(<infile>){
    @temp = split(/\s+/,$_);
    system("./bin/check_xf_weight_xs $temp[2]");
}

#!/usr/bin/perl

open(infile,"./configurations/xf_file.txt");
while(<infile>){
    @temp = split(/\s+/,$_);
    system("./bin/check_xf_weight_xs $temp[3]"); # L-18 fix: col 3 is the file path, not col 2
}

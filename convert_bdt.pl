#!/usr/bin/perl

# update BDT scores ....
open(infile,"filelist_bdt");
my $num = 0;
my $limit = 11;
while(<infile>){
    $filename = $_;
    chomp($filename);
    @temp = split(/\s+/,$filename);

    if ($temp[0] eq "end") {
	last;
    }
    # remove training events ... 
    if ($temp[4] == 1){
	if ($num %12 == $limit){
	    system("bin/bdt_convert $temp[0]$temp[1]  $temp[2]$temp[1] -l./training_list/list.dat");
	}else{
	    system("bin/bdt_convert $temp[0]$temp[1]  $temp[2]$temp[1] -l./training_list/list.dat &");
	}
    }elsif ($temp[4]==2){
	# special mode to remove training events ...
	# for variables that are not good
	if ($num %12 == $limit){
	    system("bin/bdt_convert $temp[0]$temp[1] $temp[2]$temp[1]  -g$temp[5]  -l./training_list/list.dat");
	}else{
	    system("bin/bdt_convert $temp[0]$temp[1] $temp[2]$temp[1]  -g$temp[5] -l./training_list/list.dat &");
	}
    }elsif ($temp[4]==3){
	# skip bad/good runs ...
	if ($num %12 == $limit){
	    system("bin/bdt_convert $temp[0]$temp[1] $temp[2]$temp[1] -s1");
	}else{
	    system("bin/bdt_convert $temp[0]$temp[1] $temp[2]$temp[1] -s1 &");
	}
    }elsif ($temp[4]==4){
	# NuMI running ...
	if ($num %12 == $limit){
	    system("bin/bdt_convert $temp[0]$temp[1] $temp[2]$temp[1] -n1 ");
	}else{
	    system("bin/bdt_convert $temp[0]$temp[1] $temp[2]$temp[1] -n1 &");
	}
    }else{
	# standard processing for BNB ...
	if ($num %12 == $limit){
	    system("bin/bdt_convert $temp[0]$temp[1] $temp[2]$temp[1] ");
	}else{
	    system("bin/bdt_convert $temp[0]$temp[1] $temp[2]$temp[1] &");
	}
    }
    
    
    $num++;
}

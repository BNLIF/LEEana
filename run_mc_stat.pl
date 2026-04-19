#!/usr/bin/perl

system("mkdir -p mc_stat");
die "mkdir mc_stat failed: $?" if $?;

for (my $i=0;$i != 100; $i++){
    my $lee_strength = 0.03 * $i;
    if ($i % 12 == 11){
	system("./bin/merge_hist -r0 -e2 -l$lee_strength > ./mc_stat/$i\.log");
    }else{
	system("./bin/merge_hist -r0 -e2 -l$lee_strength > ./mc_stat/$i\.log &");
    }
    
}

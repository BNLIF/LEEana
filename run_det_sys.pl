#!/usr/bin/perl
my $num1 = scalar(@ARGV);

for (my $i=1;$i<=10;$i++){
    if ($num1 ==0){
	if ($i==5) {
          print "Skip WireMod dEdx \n";
        }
	else {
	  system("./bin/det_cov_matrix -r$i -bholly_antinue_bdt&");
        }
    }else{
	print "Oscillation! \n"; 
	system("./bin/det_cov_matrix -r$i -o1 -bholly_antinue_bdt&");
    }
}

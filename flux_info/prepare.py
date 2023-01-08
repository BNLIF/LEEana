import ROOT
import pandas as pd
import re
import argparse

arg_desc='''\
utility script to convert compute xsec constants given configuration txt file
'''
parser = argparse.ArgumentParser(description=arg_desc)
parser.add_argument("-p", "--pot", help = "path to data file that contains POT info", required=True, type=str)
parser.add_argument("-f", "--flux", help = "path to flux file containing TGraphs in #/POT*GeV*cm2", required=True, type=str)
parser.add_argument("-m", "--macro", help = "path to calculate_num.C macro", required=True, type=str)
parser.add_argument("-x", "--xstxt", help = "path to xs configuration file containing analysis bins", required=True, type=str)
args = parser.parse_args()

def get_pot(f, tag):
    pot=0
    fileroot = ROOT.TFile(f, "read")
    print(f)
    t = fileroot.Get('wcpselection/T_pot')
    for ievt in t:
        if tag != 'mc_overlay':
            pot += ievt.pot_tor875good
        else:
            pot += ievt.pot_tor875
    
    fileroot.Close()
    if tag != 'mc_overlay' and tag != 'dirt':
        pot = 1.e12*pot
    return pot

df = pd.read_csv(args.xstxt, sep='\s*', engine='python', header=None)
parse_min = lambda a : int(re.sub(r'^.*gt\.([0-9]*).*$', '\\1', a))
parse_max = lambda a : int(re.sub(r'^.*le\.([0-9]*)\..*$', '\\1', a))
bintxt = list(df[2])
grs = {"numuCC" : ["gh_averaged_numu_total"],
       "nueCC" : ["gh_averaged_nue_total"],
       "NC" : ["gh_averaged_numu_total", "gh_averaged_nue_total", "gh_averaged_antinumu_total", "gh_averaged_antinue_total"]
       }

pot = get_pot(args.pot, 'data')
ROOT.gROOT.ProcessLine(".L "+args.macro)


for i in range(len(bintxt)):
    b = bintxt[i]
    final_constant = 0.
    if (b == "End") or ("#" in b): 
        continue
    bin_l = parse_min(b)/1000.
    bin_h = parse_max(b)/1000.
    print(bin_l, bin_h)
    gr = []
    if "numuCC" in b: gr = grs["numuCC"]
    if "nueCC" in b: gr = grs["nueCC"]
    if "NC" in b: gr = grs["NC"]
    
    c_l, c_h = 0., 0.
    e_l, e_h = 0., 5.
    if "Enu" not in b: 
        c_l, c_h = bin_l, bin_h
    else:
        e_l, e_h = bin_l, bin_h
    for g in gr:
        final_constant += ROOT.calculate_num(e_l, e_h, 1000, pot, c_l, c_h, args.flux, g)
    df[3][i] = final_constant

print(df)
df.to_csv('xs_real_bin.txt', sep='\t', index=False, header=False, float_format='%10.4f')

#
#  print ("Numu Constants\n*************")
#  ROOT.calculate_num(0., 5., 1000, pot, 0, 0, "../flux_info/gh_averaged_bnb_5GeV_flux.root", "gh_averaged_numu_total")
#
#  print ("Nue Constants\n*************")
#  ROOT.calculate_num(0., 5., 1000, pot, 0, 0, "../flux_info/gh_averaged_bnb_5GeV_flux.root", "gh_averaged_nue_total")
#
#  print ("Anti-numu Constants\n*************")
#  ROOT.calculate_num(0., 5., 1000, pot, 0, 0, "../flux_info/gh_averaged_bnb_5GeV_flux.root", "gh_averaged_antinumu_total")
#
#  print ("Anti-nue Constants\n*************")
#  ROOT.calculate_num(0., 5., 1000, pot, 0, 0, "../flux_info/gh_averaged_bnb_5GeV_flux.root", "gh_averaged_antinue_total")

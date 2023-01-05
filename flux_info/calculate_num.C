void calculate_num(double init_E = 0., double final_E = 5, 
                   int nstep = 1000, double POT = 5.327e19, 
                   double init_difval=0, double final_difval=0,
                   std::string filename="gh_averaged_numu_flux.root", std::string grname="gh_averaged_numu_flux"){
  TFile *file = new TFile(filename.c_str());
  TGraph *gflux = (TGraph*)file->Get(grname.c_str()); // numu/POT/GeV/cm2

  double density = 1.3836; //g/cm^3
  double volume = 5.82515e7; //cm^3
  double m_mol = 39.95; //g/mol
  double NA = 6.022e23; // N/mol

  double target_N = density * volume * NA/m_mol;
  std::cout << "Number of Target Argon: " << target_N << std::endl;

  double pb = 1e-36; // pbarn
  std::cout << "POT                   : " << POT << std::endl;
  
  double flux_sum = 0;
  for (Int_t i=0;i!=nstep;i++){
    double x = init_E + (final_E-init_E)*(i+0.5)/nstep;
    flux_sum += gflux->Eval(x)*(final_E-init_E)/nstep;
  }
  std::cout << "Flux                  : " << flux_sum << std::endl;

  std::cout << std::endl;
  double final_constant = flux_sum * POT * target_N * pb;

  if (final_difval!=init_difval and final_difval>0)
  std::cout << "Final constant        : " << final_constant * (final_difval - init_difval) << std::endl;
  else 
  std::cout << "Final constant        : " << final_constant << std::endl;
}

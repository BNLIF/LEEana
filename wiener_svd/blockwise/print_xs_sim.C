void set_cov_axis(TH2D* h ){
  h->GetXaxis()->SetLabelSize(0.04);
  h->GetXaxis()->SetTitleSize(0.055);
  h->GetXaxis()->SetTitleOffset(0.75);
  h->GetYaxis()->SetLabelSize(0.04);
  h->GetYaxis()->SetTitleSize(0.055);
  h->GetYaxis()->SetTitleOffset(0.6);
  //h->SetTitle("MicroBooNE");

  h->GetXaxis()->SetTitle("Truth bin index");
  h->GetYaxis()->SetTitle("Truth bin index");
}


void M2H(const TMatrixD& mat, TH2D* histo)
{
    // Fill matrix to histogram
    for(Int_t i=0; i<mat.GetNrows(); i++)
    {
        for(Int_t j=0; j<mat.GetNcols(); j++)
        {
            histo->SetBinContent(i+1, j+1, (mat)(i, j));
        }
    }
}

void M2H(const TMatrixD* mat, TH2D* histo)
{
    // Fill matrix to histogram
    for(Int_t i=0; i<mat->GetNrows(); i++)
    {
        for(Int_t j=0; j<mat->GetNcols(); j++)
        {
            histo->SetBinContent(i+1, j+1, (*mat)(i, j));
        }
    }
}

void func_canv_margin(TCanvas *canv, double left, double right, double top, double bot)
{
  canv->SetLeftMargin(left);
  canv->SetRightMargin(right);
  canv->SetTopMargin(top);
  canv->SetBottomMargin(bot);
}

void func_canv_margin(TVirtualPad *canv, double left, double right, double top, double bot)
{
  canv->SetLeftMargin(left);
  canv->SetRightMargin(right);
  canv->SetTopMargin(top);
  canv->SetBottomMargin(bot);
}

void func_canv_margin(TPad *canv, double left, double right, double top, double bot)
{
  canv->SetLeftMargin(left);
  canv->SetRightMargin(right);
  canv->SetTopMargin(top);
  canv->SetBottomMargin(bot);
}


// main function
void print_xs_sim(int print = 1){
  
  // 0 print the origional way
  // 1 print the xs
  // 2 print the ubt
  // 3 print the Ac
  // 4 print the cov

  // Split things up
  std::vector<int> xs_lst   = {0,0,  1,  2,2,   3,   4};
  std::vector<int> nbin_lst = {6,6,  9,  8,15,  17,  24};
  std::vector<int> is_0pNp = {-1,1,  0,  -1,1,  0,   0};
  int xs_bins[14] =  {12,9,23,17,24};

  double NUCLEONS = 40.; // Per N
  //double NUCLEONS = 1.; // Per Ar
  double XS_FACTOR = 1000./NUCLEONS;  // Cross section is in terms of 10e-36 cm^2/Ar if XS_FACTOR == 1., otherwise change axis
                                      // 1e-38 cm^2/Ar if XS_FACTOR == 100.
                                      // 1e-39 cm^2/Ar if XS_FACTOR == 1000.
                                      // 1e-40 cm^2/Ar if XS_FACTOR == 10000.

  // Are you using the RW sys
  bool rw_sys = false;

  bool one_wiener = false;

  int nch = xs_lst.size();
  std::vector<int> xs_lst_temp;
  for(int i=0; i<nch; i++){
    bool is_in_xs_lst = false;
    for(int j=0; j<xs_lst_temp.size(); j++){
      if( xs_lst.at(i)==xs_lst_temp.at(j) ){ is_in_xs_lst = true; std::cout<<xs_lst.at(i)<<std::endl; }
    }
    if(!is_in_xs_lst){ xs_lst_temp.push_back( xs_lst.at(i) ); }
  }
  int n_xs = xs_lst_temp.size();  
  std::cout<<n_xs<<std::endl;

  double xbins[1000];
  for(int i=0; i<1000; i++){xbins[i]=i;}

  double NUCLEONS = 40.;
  //double NUCLEONS = 1.;
  double XS_FACTOR = 1.;  // Cross section is in terms of 10e-36 cm^2/Ar if XS_FACTOR == 1., otherwise change axis
                                      // 1e-38 cm^2/Ar if XS_FACTOR == 100.
                                      // 1e-39 cm^2/Ar if XS_FACTOR == 1000.
                                      // 1e-40 cm^2/Ar if XS_FACTOR == 10000.

  XS_FACTOR = 1000./NUCLEONS;  // Only if you want XS normalized per nucleon
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////
  // Extract fractional uncertainties
  //////////////////////////////////////////////////////////////////////////////////////////////////////
  auto c1 = new TCanvas("c1","c1",800,600);
  c1->cd();

  TFile *file_wiener  = new TFile("wiener.root");
  
  TH2D *hcov_stat     = (TH2D*)file_wiener->Get("hcov_full_stat");
  TH2D *hcov_mcstat   = (TH2D*)file_wiener->Get("hcov_full_mcstat");
  TH2D *hcov_add      = (TH2D*)file_wiener->Get("hcov_full_add");
  TH2D *hcov_flux     = (TH2D*)file_wiener->Get("hcov_full_flux");
  TH2D *hcov_det      = (TH2D*)file_wiener->Get("hcov_full_det");
  TH2D *hcov_xs       = (TH2D*)file_wiener->Get("hcov_full_xs");
  TH2D *hcov_tot      = (TH2D*)file_wiener->Get("hcov_full_tot");
  TH2D *hcov_rw   = (TH2D*)file_wiener->Get("hcov_full_rw");
  TH2D *hcov_rwcor   = (TH2D*)file_wiener->Get("hcov_full_rwcor");
  
  TH1D *htrue_signal      = (TH1D*)file_wiener->Get("htrue_full_signal");
  TH1D *hmeas      = (TH1D*)file_wiener->Get("full_hmeas");

  std::vector < TMatrixD* > covRotation_lst;
  std::vector < TMatrixD* > covRotation_t_lst;
  std::vector < TH2D* > smear_lst;
  std::vector < TH1D* > unf_lst;
  std::vector < TH1D* > hmeas_lst;

  std::vector< int > m_lst;
  std::vector< int > n_lst;
  vector<int> nbin_true_lst;



  int indexm = 0;
  int indexn = 0;
  int n = 0;  //reco
  int m = 0;  //true

if(!one_wiener){
  for(int xs=0; xs<n_xs; xs++){
    std::string filename1     =     "output" + std::to_string(xs+1) + ".root"; 
    if(print==0){std::cout<<filename1<<std::endl;}
    TFile* file_output  = new TFile(filename1.c_str());
    covRotation_lst.push_back( (TMatrixD*)file_output->Get("covRotation") );
    covRotation_t_lst.push_back( (TMatrixD*)file_output->Get("covRotation_t") );
    smear_lst.push_back( (TH2D*)file_output->Get("smear") );
    n_lst.push_back( covRotation_lst.at(xs)->GetNcols() );
    m_lst.push_back( covRotation_lst.at(xs)->GetNrows() ); 
    n+=n_lst.at(xs);
    m+=m_lst.at(xs);
    nbin_true_lst.push_back(m_lst.at(xs));
  }
}else{
    std::string filename1     =     "blockwise_output.root";
    TFile* file_output  = new TFile(filename1.c_str());
    covRotation_lst.push_back( (TMatrixD*)file_output->Get("covRotation") );
    covRotation_t_lst.push_back( (TMatrixD*)file_output->Get("covRotation_t") );
    smear_lst.push_back( (TH2D*)file_output->Get("smear") );
    n_lst.push_back( covRotation_lst.at(0)->GetNcols() );
    m_lst.push_back( covRotation_lst.at(0)->GetNrows() );
    n+=n_lst.at(0);
    m+=m_lst.at(0);
}
//std::cout<<"n: "<<n<<std::endl;

  TMatrixD covRotation(m,n);
  TMatrixD covRotation_t(n,m);
  TMatrixD smear(m,m);
  TVectorD pred(n);
  TVectorD meas(n);  
  
  covRotation.Zero();
  covRotation_t.Zero();
  smear.Zero();

  for(int xs=0; xs<n_xs; xs++){
    for(int i=0; i<m_lst.at(xs); i++){
      for(int j=0; j<n_lst.at(xs); j++){
        covRotation(i+indexm,j+indexn) = (*covRotation_lst.at(xs))(i,j);
        covRotation_t(j+indexn,i+indexm) = (*covRotation_t_lst.at(xs))(j,i);
      }
    }
    indexn+=n_lst.at(xs);
    indexm+=m_lst.at(xs);
    if(one_wiener) break;
  }
  indexm=0;
  indexn=0;
  for(int xs=0; xs<n_xs; xs++){
    for(int i=0; i<m_lst.at(xs); i++){
      for(int j=0; j<m_lst.at(xs); j++){
        smear(i+indexm,j+indexm) = (smear_lst.at(xs))->GetBinContent(i+1,j+1);
      }
    }
    indexm+=m_lst.at(xs);
    if(one_wiener) break;
  }
  for(int i=0; i<n; i++){
    meas[i] = hmeas->GetBinContent(i+1);
  }
  TVectorD unf = covRotation*meas;
  TVectorD unf_untouched = unf;
  TVectorD vec_signal(m);
  TVectorD vec_signal_untouched(m);

  for (int i=0;i<m;i++) {
    unf[i] = unf[i]*XS_FACTOR;
    vec_signal(i) = htrue_signal->GetBinContent(i+1) * XS_FACTOR;
    vec_signal_untouched(i) = htrue_signal->GetBinContent(i+1);
  }

  TVectorD Ac_vec_signal = smear * vec_signal;
  TVectorD Ac_vec_signal_untouched = smear * vec_signal_untouched;

  TMatrixD mcov_stat(n,n);
  TMatrixD mcov_mcstat(n,n);
  TMatrixD mcov_add(n,n);
  TMatrixD mcov_flux(n,n);
  TMatrixD mcov_det(n,n);
  TMatrixD mcov_xs(n,n);
  TMatrixD mcov_tot(n,n);
  TMatrixD mcov_rw(n,n);
  TMatrixD mcov_rwcor(n,n);

  for (int i=0;i<n;i++) {
    for (int j=0;j<n;j++) {
      mcov_stat(i,j)   = hcov_stat->GetBinContent(  i+1,j+1);
      mcov_mcstat(i,j) = hcov_mcstat->GetBinContent(i+1,j+1);
      mcov_add(i,j)    = hcov_add->GetBinContent(   i+1,j+1);
      mcov_flux(i,j)   = hcov_flux->GetBinContent(  i+1,j+1);
      mcov_det(i,j)    = hcov_det->GetBinContent(   i+1,j+1);
      mcov_xs(i,j)     = hcov_xs->GetBinContent(    i+1,j+1);
      mcov_tot(i,j)    = hcov_tot->GetBinContent(   i+1,j+1);
      mcov_rw(i,j) = hcov_rw->GetBinContent(i+1,j+1);
      mcov_rwcor(i,j) = hcov_rwcor->GetBinContent(i+1,j+1);
    }
  }

  TMatrixD mcov_stat_rot   = (covRotation) * mcov_stat   * (covRotation_t);
  TMatrixD mcov_mcstat_rot = (covRotation) * mcov_mcstat * (covRotation_t);
  TMatrixD mcov_add_rot    = (covRotation) * mcov_add    * (covRotation_t);
  TMatrixD mcov_flux_rot   = (covRotation) * mcov_flux   * (covRotation_t);
  TMatrixD mcov_det_rot    = (covRotation) * mcov_det    * (covRotation_t);
  TMatrixD mcov_xs_rot     = (covRotation) * mcov_xs     * (covRotation_t);
  TMatrixD mcov_tot_rot    = (covRotation) * mcov_tot    * (covRotation_t);
  TMatrixD mcov_rw_rot     = (covRotation) * mcov_rw     * (covRotation_t);
  TMatrixD mcov_rwcor_rot     = (covRotation) * mcov_rwcor     * (covRotation_t);

  TH1D* herr_frac_stat_untouched   = new TH1D("herr_frac_stat_untouched",  "herr_frac_stat_untouched",  m,0.5,m+0.5);
  TH1D* herr_frac_mcstat_untouched = new TH1D("herr_frac_mcstat_untouched","herr_frac_mcstat_untouched",m,0.5,m+0.5);
  TH1D* herr_frac_add_untouched    = new TH1D("herr_frac_add_untouched",   "herr_frac_add_untouched",   m,0.5,m+0.5);
  TH1D* herr_frac_flux_untouched   = new TH1D("herr_frac_flux_untouched",  "herr_frac_flux_untouched",  m,0.5,m+0.5);
  TH1D* herr_frac_det_untouched    = new TH1D("herr_frac_det_untouched",   "herr_frac_det_untouched",   m,0.5,m+0.5);
  TH1D* herr_frac_xs_untouched     = new TH1D("herr_frac_xs_untouched",    "herr_frac_xs_untouched",    m,0.5,m+0.5);
  TH1D* herr_frac_tot_untouched    = new TH1D("herr_frac_tot_untouched",   "herr_frac_tot_untouched",   m,0.5,m+0.5);
  TH1D* herr_frac_rw_untouched     = new TH1D("herr_frac_rw_untouched",    "herr_frac_rw_untouched",    m,0.5,m+0.5);
  TH1D* herr_frac_rwcor_untouched     = new TH1D("herr_frac_rwcor_untouched",    "herr_frac_rwcor_untouched",    m,0.5,m+0.5);
  
  TH1D* herr_frac_stat   = new TH1D("herr_frac_stat",  "herr_frac_stat",  m,0.5,m+0.5);
  TH1D* herr_frac_mcstat = new TH1D("herr_frac_mcstat","herr_frac_mcstat",m,0.5,m+0.5);
  TH1D* herr_frac_add    = new TH1D("herr_frac_add",   "herr_frac_add",   m,0.5,m+0.5);
  TH1D* herr_frac_flux   = new TH1D("herr_frac_flux",  "herr_frac_flux",  m,0.5,m+0.5);
  TH1D* herr_frac_det    = new TH1D("herr_frac_det",   "herr_frac_det",   m,0.5,m+0.5);
  TH1D* herr_frac_xs     = new TH1D("herr_frac_xs",    "herr_frac_xs",    m,0.5,m+0.5);
  TH1D* herr_frac_tot    = new TH1D("herr_frac_tot",   "herr_frac_tot",   m,0.5,m+0.5);
  TH1D* herr_frac_rw     = new TH1D("herr_frac_rw",    "herr_frac_rw",    m,0.5,m+0.5);
  TH1D* herr_frac_rwcor     = new TH1D("herr_frac_rwcor",    "herr_frac_rwcor",    m,0.5,m+0.5);

  double vec_abs_stat_untouched[m];
  double vec_abs_syst_untouched[m];
  double vec_abs_tot_untouched[m];
  double vec_CV_untouched[m];

  double vec_err_stat_untouched[m];
  double vec_err_mcstat_untouched[m];
  double vec_err_dirt_untouched[m];
  double vec_err_flux_untouched[m];
  double vec_err_det_untouched[m];
  double vec_err_xs_untouched[m];
  double vec_err_rw_untouched[m];
  double vec_err_rwcor_untouched[m];

  double vec_abs_stat[m];
  double vec_abs_syst[m];
  double vec_abs_tot[m];
  double vec_CV[m];

  double vec_err_stat[m];
  double vec_err_mcstat[m];
  double vec_err_dirt[m];
  double vec_err_flux[m];
  double vec_err_det[m];
  double vec_err_xs[m];
  double vec_err_rw[m];
  double vec_err_rwcor[m];

  if(print==0){std::cout << "\nFractional Uncertainties -------------------------------------------- " << std::endl;}
  for (int i=0;i<m;i++) {
    double content_untouched = unf_untouched[i];
    double content = unf[i];
 
    if(print==0){
      std::cout << "--- BIN " << i+1 << "(/" << m <<") --------- " << std::endl;
      if (XS_FACTOR == 1.) std::cout << "CV = " << content << " [1e-36 cm^2 / GeV/c / Ar]" << std::endl;
      else if (XS_FACTOR == 100.) std::cout << "CV = " << content << " [1e-38 cm^2 / GeV/c / Ar]" << std::endl;
      else if (XS_FACTOR == 1000.) std::cout << "CV = " << content << " [1e-39 cm^2 / GeV/c / Ar]" << std::endl;
      else if (XS_FACTOR == 10000.) std::cout << "CV = " << content << " [1e-40 cm^2 / GeV/c / Ar]" << std::endl;

      else if (XS_FACTOR == 1./NUCLEONS) std::cout << "CV = " << content << " [1e-38 cm^2 / GeV/c / nucleon]" << std::endl;
      else if (XS_FACTOR == 100./NUCLEONS) std::cout << "CV = " << content << " [1e-38 cm^2 / GeV/c / nucleon]" << std::endl;
      else if (XS_FACTOR == 1000./NUCLEONS) std::cout << "CV = " << content << " [1e-39 cm^2 / GeV/c / nucleon]" << std::endl;
      else if (XS_FACTOR == 10000./NUCLEONS) std::cout << "CV = " << content << " [1e-40 cm^2 / GeV/c / nucleon]" << std::endl;
    }

    double abs_stat_untouched =   TMath::Sqrt(mcov_stat_rot(i,i));
    double abs_mcstat_untouched = TMath::Sqrt(mcov_mcstat_rot(i,i));
    double abs_add_untouched =    TMath::Sqrt(mcov_add_rot(i,i));
    double abs_flux_untouched =   TMath::Sqrt(mcov_flux_rot(i,i));
    double abs_det_untouched =    TMath::Sqrt(mcov_det_rot(i,i));
    double abs_xs_untouched =     TMath::Sqrt(mcov_xs_rot(i,i));
    double abs_tot_untouched =    TMath::Sqrt(mcov_tot_rot(i,i));
    double abs_rw_untouched =     TMath::Sqrt(mcov_rw_rot(i,i));
    double abs_rwcor_untouched =     TMath::Sqrt(mcov_rwcor_rot(i,i));

    double abs_stat =   TMath::Sqrt(mcov_stat_rot(i,i)) * XS_FACTOR;
    double abs_mcstat = TMath::Sqrt(mcov_mcstat_rot(i,i)) * XS_FACTOR;
    double abs_add =    TMath::Sqrt(mcov_add_rot(i,i)) * XS_FACTOR;
    double abs_flux =   TMath::Sqrt(mcov_flux_rot(i,i)) * XS_FACTOR;
    double abs_det =    TMath::Sqrt(mcov_det_rot(i,i)) * XS_FACTOR;
    double abs_xs =     TMath::Sqrt(mcov_xs_rot(i,i)) * XS_FACTOR;
    double abs_tot =    TMath::Sqrt(mcov_tot_rot(i,i)) * XS_FACTOR;
    double abs_rw =     TMath::Sqrt(mcov_rw_rot(i,i)) * XS_FACTOR;
    double abs_rwcor =     TMath::Sqrt(mcov_rwcor_rot(i,i)) * XS_FACTOR;

    double abs_syst_untouched = TMath::Sqrt( abs_mcstat_untouched*abs_mcstat_untouched + abs_add_untouched*abs_add_untouched + abs_flux_untouched*abs_flux_untouched + abs_det_untouched*abs_det_untouched + abs_xs_untouched*abs_xs_untouched );

    double abs_syst = TMath::Sqrt( abs_mcstat*abs_mcstat + abs_add*abs_add + abs_flux*abs_flux + abs_det*abs_det + abs_xs*abs_xs );

    if(rw_sys){
      abs_syst_untouched = TMath::Sqrt( abs_syst_untouched*abs_syst_untouched + abs_rw_untouched*abs_rw_untouched + abs_rwcor_untouched*abs_rwcor_untouched);
      abs_syst = TMath::Sqrt( abs_syst*abs_syst + abs_rw*abs_rw + abs_rwcor*abs_rwcor);
    }

    double abs_stat_syst_untouched = TMath::Sqrt( abs_stat_untouched*abs_stat_untouched + abs_syst_untouched*abs_syst_untouched );
    double abs_stat_syst = TMath::Sqrt( abs_stat*abs_stat + abs_syst*abs_syst );


    double frac_stat_untouched = abs_stat_untouched     / content_untouched;
    double frac_mcstat_untouched = abs_mcstat_untouched / content_untouched;
    double frac_add_untouched = abs_add_untouched       / content_untouched;
    double frac_flux_untouched = abs_flux_untouched     / content_untouched;
    double frac_det_untouched = abs_det_untouched       / content_untouched;
    double frac_xs_untouched = abs_xs_untouched         / content_untouched;
    double frac_tot_untouched = abs_tot_untouched       / content_untouched;
    double frac_rw_untouched = abs_rw_untouched         / content_untouched;
    double frac_rwcor_untouched = abs_rwcor_untouched         / content_untouched;

    double frac_syst_untouched = abs_syst_untouched / content_untouched;
    double frac_stat_syst_untouched = abs_stat_syst_untouched / content_untouched;

    double frac_stat = abs_stat     / content;
    double frac_mcstat = abs_mcstat / content;
    double frac_add = abs_add       / content;
    double frac_flux = abs_flux     / content;
    double frac_det = abs_det       / content;
    double frac_xs = abs_xs         / content;
    double frac_tot = abs_tot       / content;
    double frac_rw = abs_rw         / content;
    double frac_rwcor = abs_rwcor         / content;

    double frac_syst = abs_syst / content;
    double frac_stat_syst = abs_stat_syst / content;
    if(print==0){
      std::cout << "Err_stat = " << abs_stat << "(" << 100.*frac_stat << "%)" << std::endl;
      std::cout << "Err_mcstat = " << abs_mcstat << "(" << 100.*frac_mcstat << "%)" << std::endl;
      std::cout << "Err_dirt = " << abs_add << "(" << 100.*frac_add << "%)" << std::endl;
      std::cout << "Err_flux = " << abs_flux << "(" << 100.*frac_flux << "%)" << std::endl;
      std::cout << "Err_det = " << abs_det << "(" << 100.*frac_det << "%)" << std::endl;
      std::cout << "Err_xs = " << abs_xs << "(" << 100.*frac_xs << "%)" << std::endl;
      if(rw_sys) std::cout << "Err_rw = " << abs_rw << "(" << 100.*frac_rw << "%)" << std::endl;
      if(rw_sys) std::cout << "Err_rwcor = " << abs_rwcor << "(" << 100.*frac_rwcor << "%)" << std::endl;
      std::cout << "Err_tot = " << abs_tot << "(" << 100.*frac_tot << "%)" << std::endl;
      std::cout << "Err_syst = " << abs_syst << "(" << 100.*frac_syst << "%)" << std::endl;
      std::cout << "Err_stat_syst = " << abs_stat_syst << "(" << 100.*frac_stat_syst << "%)" << std::endl;
    }

    vec_err_stat_untouched[i] = 100.*frac_stat_untouched;
    vec_err_mcstat_untouched[i] = 100.*frac_mcstat_untouched;
    vec_err_dirt_untouched[i] = 100.*frac_add_untouched;
    vec_err_flux_untouched[i] = 100.*frac_flux_untouched;
    vec_err_det_untouched[i] = 100.*frac_det_untouched;
    vec_err_xs_untouched[i] = 100.*frac_xs_untouched;
    vec_err_rw_untouched[i] = 100.*frac_rw_untouched;
    vec_err_rwcor_untouched[i] = 100.*frac_rwcor_untouched;

    herr_frac_stat_untouched->SetBinContent(i+1,  frac_stat_untouched);
    herr_frac_mcstat_untouched->SetBinContent(i+1,frac_mcstat_untouched);
    herr_frac_add_untouched->SetBinContent(i+1,   frac_add_untouched);
    herr_frac_flux_untouched->SetBinContent(i+1,  frac_flux_untouched);
    herr_frac_det_untouched->SetBinContent(i+1,   frac_det_untouched);
    herr_frac_xs_untouched->SetBinContent(i+1,    frac_xs_untouched);
    herr_frac_tot_untouched->SetBinContent(i+1,   frac_tot_untouched);
    herr_frac_rw_untouched->SetBinContent(i+1,    frac_rw_untouched);
    herr_frac_rwcor_untouched->SetBinContent(i+1,    frac_rwcor_untouched);

    vec_abs_stat_untouched[i] = abs_stat_untouched;
    vec_abs_syst_untouched[i] = abs_syst_untouched;
    vec_abs_tot_untouched[i] = abs_tot_untouched;
    vec_CV_untouched[i] = content_untouched;


    vec_err_stat[i] = 100.*frac_stat;
    vec_err_mcstat[i] = 100.*frac_mcstat;
    vec_err_dirt[i] = 100.*frac_add;
    vec_err_flux[i] = 100.*frac_flux;
    vec_err_det[i] = 100.*frac_det;
    vec_err_xs[i] = 100.*frac_xs;
    vec_err_rw[i] = 100.*frac_rw;
    vec_err_rwcor[i] = 100.*frac_rwcor;

    herr_frac_stat->SetBinContent(i+1,  frac_stat);
    herr_frac_mcstat->SetBinContent(i+1,frac_mcstat);
    herr_frac_add->SetBinContent(i+1,   frac_add);
    herr_frac_flux->SetBinContent(i+1,  frac_flux);
    herr_frac_det->SetBinContent(i+1,   frac_det);
    herr_frac_xs->SetBinContent(i+1,    frac_xs);
    herr_frac_tot->SetBinContent(i+1,   frac_tot);
    herr_frac_rw->SetBinContent(i+1,    frac_rw);
    herr_frac_rwcor->SetBinContent(i+1,    frac_rwcor);

    vec_abs_stat[i] = abs_stat;
    vec_abs_syst[i] = abs_syst;
    vec_abs_tot[i] = abs_tot;
    vec_CV[i] = content;
  }
 
// Print off the xs
if(print==1){
  std::cout<<"GlobalXbin    Z"<<std::endl;
  int xs_index = 0;
  int bin_index = 0;
  for(int i=0; i<unf.GetNrows(); i++){
    if(bin_index>=xs_bins[xs_index]) {bin_index=0; xs_index++;}
    std::cout<<i<<"    "<<std::setprecision(16)<<unf[i]<<std::endl;
    bin_index++;
  }
  std::cout<<"end"<<std::endl;
}

// Print off the ubt cv
else if(print==2){
  std::cout<<"GlobalXbin    Z"<<std::endl;
  int xs_index = 0;
  int bin_index = 0;
  for(int i=0; i<vec_signal.GetNrows(); i++){
    if(bin_index>=xs_bins[xs_index]) {bin_index=0; xs_index++;}
    std::cout<<i<<"    "<<std::setprecision(16)<<vec_signal(i)<<std::endl;
    bin_index++;
  }
  std::cout<<"end"<<std::endl;
}

// Print off the Ac matrix
else if(print==3){
  std::cout<<"GlobalXbin    "<<"GlobalYbin    "<<"Z"<<std::endl;
  int x_xs_index = 0;
  int x_bin_index = 0;
  int y_xs_index = 0;
  int y_bin_index = 0;
  for(int i=0; i<smear.GetNrows(); i++){
    if(x_bin_index>=xs_bins[x_xs_index]) {x_bin_index=0; x_xs_index++;}
    for(int j=0; j<smear.GetNcols(); j++){
      double cov_val = smear(i,j);
      std::cout<<i<<"    "<<j<<"    "<<std::setprecision(16)<<cov_val<<std::endl;
      y_bin_index++;
    }
    y_bin_index=0;
    y_xs_index=0;
    x_bin_index++;
  }
  std::cout<<"end"<<std::endl;
}

// Print off the cov matrix
else if(print==4){
  std::cout<<"GlobalXbin    "<<"GlobalYbin    "<<"Z"<<std::endl;
  int x_xs_index = 0;
  int x_bin_index = 0;
  int y_xs_index = 0;
  int y_bin_index = 0;
  for(int i=0; i<mcov_tot_rot.GetNrows(); i++){
    if(x_bin_index>=xs_bins[x_xs_index]) {x_bin_index=0; x_xs_index++;}
    for(int j=0; j<mcov_tot_rot.GetNcols(); j++){
      if(y_bin_index>=xs_bins[y_xs_index]) {y_bin_index=0; y_xs_index++;}
      double cov_val = mcov_tot_rot(i,j)*XS_FACTOR*XS_FACTOR;
      std::cout<<i<<"    "<<j<<"    "<<std::setprecision(16)<<cov_val<<std::endl;
      y_bin_index++;
    }
    y_bin_index=0;
    y_xs_index=0;
    x_bin_index++;
  }
  std::cout<<"end"<<std::endl;
}
else{
  std::cout << "--------------------- THE END ----------------------- \n" << std::endl;

  std::cout << "err_stat = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_err_stat[i];
    else std::cout << ", " << vec_err_stat[i];
  }
  std::cout << "]" << std::endl;

  std::cout << "err_mcstat = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_err_mcstat[i];
    else std::cout << ", " << vec_err_mcstat[i];
  }
  std::cout << "]" << std::endl;

  std::cout << "err_dirt = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_err_dirt[i];
    else std::cout << ", " << vec_err_dirt[i];
  }
  std::cout << "]" << std::endl;

  std::cout << "err_flux = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_err_flux[i];
    else std::cout << ", " << vec_err_flux[i];
  }
  std::cout << "]" << std::endl;

  std::cout << "err_det = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_err_det[i];
    else std::cout << ", " << vec_err_det[i];
  }
  std::cout << "]" << std::endl;

  std::cout << "err_xs = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_err_xs[i];
    else std::cout << ", " << vec_err_xs[i];
  }
  std::cout << "]" << std::endl;
if(rw_sys){
  std::cout << "err_rw = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_err_rw[i];
    else std::cout << ", " << vec_err_rw[i];
  }
  std::cout << "]" << std::endl;

  std::cout << "err_rw = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_err_rwcor[i];
    else std::cout << ", " << vec_err_rwcor[i];
  }
  std::cout << "]" << std::endl;
}

  std::cout << "y = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << unf[i];
    else std::cout << ", " << unf[i];
  }
  std::cout << "]" << std::endl;

  std::cout << "yerr_stat = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_abs_stat[i];
    else std::cout << ", " << vec_abs_stat[i];
  }
  std::cout << "]" << std::endl;

  std::cout << "yerr_sys = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_abs_syst[i];
    else std::cout << ", " << vec_abs_syst[i];
  }
  std::cout << "]" << std::endl;

  std::cout << "yerr_tot = [";
  for (int i=0; i<m; i++){
    if (i==0) std::cout << vec_abs_tot[i];
    else std::cout << ", " << vec_abs_tot[i];
  }
  std::cout << "]" << std::endl;
}

  gStyle->SetOptStat(0);

  double xbins1[m+1];
  for(int i=0; i<m+1; i++) {
    xbins1[i] = i;
  }
  double ybins1[m+1];
  for(int i=0; i<m+1; i++) {
    ybins1[i] = i;
  }

  for(int i=0; i<mcov_tot_rot.GetNrows(); i++){
    for(int j=0; j<mcov_tot_rot.GetNrows(); j++){
      if(i!=j && abs(i-j)>0) mcov_tot_rot(i,j)*=1;
    }
  }

  auto  mcor_tot_rot_full_h = new TH2D("mcor_tot_rot_full_h","",m,xbins1, m,ybins1);
  auto mcor_tot_rot = mcov_tot_rot;
  for(int i=0; i<m; i++){
    for(int j=0; j<m; j++){
      double val1 = sqrt(mcov_tot_rot(i,i));
      double val2 = sqrt(mcov_tot_rot(j,j));
      mcor_tot_rot(i,j) = mcov_tot_rot(i,j)/(val1*val2);
    }
  }
  M2H(mcor_tot_rot,  mcor_tot_rot_full_h);
  gStyle->SetPalette(kTemperatureMap);
  mcor_tot_rot_full_h->GetZaxis()->SetRangeUser(-1, 1);
  mcor_tot_rot_full_h->SetContour(100);

  func_canv_margin(gPad, 0.1, 0.15, 0.05, 0.1);
  gStyle->SetTitleFontSize(0.03);
  gStyle->SetTitleOffset(-0.2);

  set_cov_axis(mcor_tot_rot_full_h);

  mcor_tot_rot_full_h->Draw("colz");

  int draw_index = 0;
  for(int xs=0; xs<n_xs-1; xs++){
    draw_index+=xs_bins[xs];
    auto lx_temp = new TLine(0,draw_index,m,draw_index);
    lx_temp->SetLineStyle(7);
    lx_temp->SetLineWidth(1);
    lx_temp->Draw("same");
    auto ly_temp = new TLine(draw_index,0,draw_index,m);
    ly_temp->SetLineStyle(7);
    ly_temp->SetLineWidth(1);
    ly_temp->Draw("same");
  }

  c1->Print("blockwise_cor.pdf");


  auto c2 = new TCanvas("c2","c2",800,600);
  c2->cd();
  auto  mcov_tot_rot_full_h = new TH2D("mcov_tot_rot_full_h","",m,xbins1, m,ybins1);
  M2H(mcov_tot_rot,  mcov_tot_rot_full_h);
  gStyle->SetPalette(kBird);
  mcov_tot_rot_full_h->SetContour(100);

  func_canv_margin(gPad, 0.1, 0.15, 0.05, 0.1);
  gStyle->SetTitleFontSize(0.03);
  gStyle->SetTitleOffset(-0.2);

  for(int i=0; i<m; i++){
    for(int j=0; j<m; j++){
      mcov_tot_rot(i,j) = mcov_tot_rot(i,j)*XS_FACTOR*XS_FACTOR;
    }
  }

  set_cov_axis(mcov_tot_rot_full_h);

  mcov_tot_rot_full_h->Draw("colz");
  draw_index = 0;
  for(int xs=0; xs<n_xs-1; xs++){
    draw_index+=xs_bins[xs];
    auto lx_temp = new TLine(0,draw_index,m,draw_index);
    lx_temp->SetLineStyle(2);
    lx_temp->SetLineWidth(1);
    lx_temp->Draw("same");
    auto ly_temp = new TLine(draw_index,0,draw_index,m);
    ly_temp->SetLineStyle(2);
    ly_temp->SetLineWidth(1);
    ly_temp->Draw("same");
  }

  c2->Print("blockwise_cov.pdf");

  auto mfraccov_tot_rot = mcov_tot_rot;
  auto c3 = new TCanvas("c3","c3",800,600);
  c3->cd();
  for(int i=0; i<m; i++){
    for(int j=0; j<m; j++){
      double val1 = unf[i];
      double val2 = unf[j];
      mfraccov_tot_rot(i,j) = mcov_tot_rot(i,j)/(val1*val2);
    }
  }
  auto  mfraccov_tot_rot_full_h = new TH2D("mfraccov_tot_rot_full_h","",m,xbins1, m,ybins1);
  M2H(mfraccov_tot_rot,  mfraccov_tot_rot_full_h);
  gStyle->SetPalette(kBird);
  gStyle->SetPalette(kColorPrintableOnGrey);
  gStyle->SetPalette(kRainBow);
  mfraccov_tot_rot_full_h->SetContour(100);

  func_canv_margin(gPad, 0.1, 0.15, 0.05, 0.1);
  gStyle->SetTitleFontSize(0.03);
  gStyle->SetTitleOffset(-0.2);

  set_cov_axis(mfraccov_tot_rot_full_h);

  mfraccov_tot_rot_full_h->GetZaxis()->SetRangeUser(-0.12, 0.3);

  mfraccov_tot_rot_full_h->Draw("colz");
  draw_index = 0;
  for(int xs=0; xs<n_xs-1; xs++){
    draw_index+=xs_bins[xs];
    auto lx_temp = new TLine(0,draw_index,m,draw_index);
    lx_temp->SetLineStyle(2);
    lx_temp->SetLineWidth(1);
    lx_temp->Draw("same");
    auto ly_temp = new TLine(draw_index,0,draw_index,m);
    ly_temp->SetLineStyle(2);
    ly_temp->SetLineWidth(1);
    ly_temp->Draw("same");
  }
  
  c3->Print("blockwise_fraccov.pdf");
}

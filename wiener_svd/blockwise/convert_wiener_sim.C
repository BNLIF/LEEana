// Does the entirety of convert wieners at once. has been check agains doing individually and yeild s the same results
// Requires setting n bins 2d and 3d and assumes you go FC PC and the Sig bckg EXT
// Uses the mc stat cor treatment without modification to the main blocks
// Ordering of tha channels is hardcoded, make sure it matches what is assumed here!

void convert_wiener_sim(){

  std::vector<int> n_diff_bins_2D = {2,1,2,1,4};
  std::vector<int> n_diff_bins_3D = {1,1,1,1,1};

  int n_xs = n_diff_bins_2D.size();

  bool just_stat_uncertainty      = false; // Turn off everything but data stat
  bool use_fakedata               = false; // Carefull with this flag, may break in some instances.

  bool mcstat_cor = true;

  bool one_wiener = false; // Only save a single file with everything

  bool rw_sys = false; // Are you including cov_18 and cov_19?

  bool split_target_pot = true; // Save these cov matricies seperatly
  bool split_geant = true; // Save these cov matricies seperatly

/////////////////////////////////////////////



 
  int nbin_hist = 0; // Total number of bins
  std::vector< int > nbin_hist_lst; // Number of bins for each ch
  std::vector< int > nbin_hist_xs_lst; // Number of bins for each xs

  int nbin_hist_FC = 0; // Total number of FC bins
  std::vector< int > nbin_hist_FC_lst; //number of FC bins for each ch
  std::vector< int > nbin_hist_FC_xs_lst; //number of FC bins for each xs

  int nbin_hist_PC = 0; // Total number of PC bins
  std::vector< int > nbin_hist_PC_lst; // Number of PC bins for each ch
  std::vector< int > nbin_hist_PC_xs_lst; // Number of PC bins for each xs

  int nbin_FC = 0; // Total number of collapsed FC bins
  std::vector< int > nbin_FC_lst; // Number of collapsed FC bins for each ch
  std::vector< int > nbin_FC_xs_lst; // Number of collapsed FC bins for each xs  

  int nbin_PC = 0; // Total number of collapsed PC bins
  std::vector< int > nbin_PC_lst; // Number of collapsed PC bins for each ch
  std::vector< int > nbin_PC_xs_lst; // Number of collapsed PC bins for each xs  

  int nbin_meas = 0; // Total number of collapsed bins
  std::vector< int > nbin_meas_lst; // Total numer of collapsed bins for each ch
  std::vector< int > nbin_meas_xs_lst; // Total numer of collapsed bins for each xs  

  int nbin_true = 0; // Total number of true bins
  std::vector< int > nbin_true_lst; // Number of true ins for each xs

  // Open files to get the binning structure
  std::cout<<"Forming binning structure using merge_xs.root files"<<std::endl;
  for(int xs=0;xs<n_xs;xs++){
    
    // Get file and the number of histograms in that file
    std::string filename1     =     "merge_xs" + std::to_string(xs+1) + ".root";
    std::cout<<"Getting binning for XS "<<xs<<" from: "<<filename1<<std::endl;
    TFile *file_temp = new TFile(filename1.c_str());
    TVectorD* vec_signal_temp =  (TVectorD*)file_temp->Get("vec_signal");
    int nhisto = 2*3*n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs);
    
    // For counting the number of bins
    int nbin_FC_temp = 0;
    int nbin_PC_temp = 0;
    int nbin_meas_temp = 0;
    int nbin_hist_temp = 0;
    int nbin_hist_FC_temp = 0;
    int nbin_hist_PC_temp = 0;

    for(int i=0; i<nhisto; i++){
      // Get the histogram
      std::string histo_str = "histo_"       + std::to_string(i+1);
      TH1F *histo_tmp       = (TH1F*)file_temp->Get(histo_str.c_str());

      // Number of bins in the histogram
      int nbin_ihist = histo_tmp->GetNbinsX()+1;
      nbin_hist_temp += nbin_ihist;
      nbin_hist += nbin_ihist;
      nbin_hist_lst.push_back(nbin_ihist);

      // Add to PC or FC
      if(i<n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs)) { // Sig and meas FC 
        nbin_FC += nbin_ihist;
        nbin_FC_lst.push_back(nbin_ihist); 
        nbin_FC_temp += nbin_ihist;
        nbin_meas += nbin_ihist;
        nbin_meas_lst.push_back(nbin_ihist);
        nbin_meas_temp += nbin_ihist; 
        nbin_hist_FC += nbin_ihist;
        nbin_hist_FC_temp += nbin_ihist;
        nbin_hist_FC_lst.push_back(nbin_ihist);
      }
      else if(i<n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs)*2) { // Sig and meas PC 
        nbin_PC+=nbin_ihist; 
        nbin_PC_lst.push_back(nbin_ihist);
        nbin_PC_temp += nbin_ihist; 
        nbin_meas += nbin_ihist;
        nbin_meas_temp += nbin_ihist;
        nbin_meas_lst.push_back(nbin_ihist);
        nbin_hist_PC += nbin_ihist;
        nbin_hist_PC_temp += nbin_ihist;
        nbin_hist_PC_lst.push_back(nbin_ihist);
      }
      else if(i<n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs)*3) { // Bckg FC
        nbin_hist_FC += nbin_ihist;
        nbin_hist_FC_temp += nbin_ihist;
        nbin_hist_FC_lst.push_back(nbin_ihist);
      }
      else if(i<n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs)*4) { // Bckg PC
        nbin_hist_PC += nbin_ihist;
        nbin_hist_PC_temp += nbin_ihist;
        nbin_hist_PC_lst.push_back(nbin_ihist);
      }
      else if(i<n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs)*5) { // EXT FC
        nbin_hist_FC += nbin_ihist;
        nbin_hist_FC_temp += nbin_ihist;
        nbin_hist_FC_lst.push_back(nbin_ihist);
      }
      else if(i<n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs)*6) { // EXT PC
        nbin_hist_PC += nbin_ihist;
        nbin_hist_PC_temp += nbin_ihist;
        nbin_hist_PC_lst.push_back(nbin_ihist);
      }
    }
    // Update the list of bin numbers
    nbin_FC_xs_lst.push_back(nbin_FC_temp);
    nbin_PC_xs_lst.push_back(nbin_PC_temp);
    nbin_hist_xs_lst.push_back(nbin_hist_FC_temp+nbin_hist_PC_temp);
    nbin_hist_FC_xs_lst.push_back(nbin_hist_FC_temp);
    nbin_hist_PC_xs_lst.push_back(nbin_hist_PC_temp);
    nbin_meas_xs_lst.push_back(nbin_meas_temp);
    nbin_true+=vec_signal_temp->GetNrows();
    nbin_true_lst.push_back(vec_signal_temp->GetNrows());
  
  }
 
  std::cout<<std::endl; 
  std::cout<<"nbin_FC "<<nbin_FC<<"  nbin_PC "<<nbin_PC<<"  nbin_hist "<<nbin_hist<<"  nbin_meas "<<nbin_meas<<"  nbin_true "<<nbin_true<<std::endl;
  std::cout<<std::endl;
  std::cout<<std::endl;

  //std::cout<<nbin_FC_lst.at(1000);

  // Measured channels and sig/bckg/ext
  // Ordering of tha channels is hardcoded, make sure it matches what is assumed here!
  TH1F *hdata_obsch_1 = new TH1F("hdata_obsch_1_merge","hdata_obsch_1_merge", nbin_FC, 0, nbin_FC); // Data FC
  TH1F *hdata_obsch_2 = new TH1F("hdata_obsch_2_merge","hdata_obsch_2_merge", nbin_PC, 0, nbin_PC); // Data PC
  TH1F *hmc_obsch_1   = new TH1F("hmc_obsch_1_merge",  "hmc_obsch_1_merge",   nbin_FC, 0, nbin_FC); // MC FC
  TH1F *hmc_obsch_2   = new TH1F("hmc_obsch_2_merge",  "hmc_obsch_2_merge",   nbin_PC, 0, nbin_PC); // MC PC
  TH1F *histo_1       = new TH1F("histo_1_merge",      "histo_1_merge",       nbin_FC, 0, nbin_FC); // Sig FC
  TH1F *histo_2       = new TH1F("histo_2_merge",      "histo_2_merge",       nbin_PC, 0, nbin_PC); // Sig PC
  TH1F *histo_3       = new TH1F("histo_3_merge",      "histo_3_merge",       nbin_FC, 0, nbin_FC); // Bckg FC
  TH1F *histo_4       = new TH1F("histo_4_merge",      "histo_4_merge",       nbin_PC, 0, nbin_PC); // Bckg PC
  TH1F *histo_5       = new TH1F("histo_5_merge",      "histo_5_merge",       nbin_FC, 0, nbin_FC); // Ext FC
  TH1F *histo_6       = new TH1F("histo_6_merge",      "histo_6_merge",       nbin_PC, 0, nbin_PC); // Ext PC

  // Measured and predicted vector
  TH1D *hmeas = new TH1D("full_hmeas","full_hmeas",nbin_meas,0.5, nbin_meas+0.5);
  TH1D *hpred = new TH1D("full_hpred","full_hpred",nbin_meas,0.5, nbin_meas+0.5);

  // True signal ...
  TH1D *htrue_signal = new TH1D("htrue_full_signal","htrue_full_signal",nbin_true, 0.5, nbin_true+0.5);
  TVectorD vec_signal;
  vec_signal.Zero();
  std::vector< TVectorD* > vec_signal_lst;

  // Collapsed response matrix
  TMatrixD mat_R_collapse(nbin_meas,nbin_true);
  mat_R_collapse.Zero();
  std::vector< TMatrixD* > mat_R_lst;

  // Collapsing matrix
  TMatrixD mat_collapse(nbin_hist,nbin_meas);
  mat_collapse.Zero();
  std::vector< TMatrixD* > mat_collapse_lst;

  // Dirt uncertainties
  TH2D *hcov_add = new TH2D("hcov_full_add","hcov_full_add",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);

  // Data stat uncertainties
  TH2D *hcov_stat = new TH2D("hcov_full_stat","hcov_full_stat",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);

  // Nominal vector for scaling systematics
  TVectorD vec_nominal(nbin_hist);

  int histo_index = 0;
  int histo_FC_index = 0;
  int histo_PC_index = nbin_hist_FC_xs_lst.at(0)/3;
  int meas_FC_index = 0;
  int meas_PC_index = nbin_FC_xs_lst.at(0);
  int meas_index = 0;
  int true_index = 0;
  int FC_index = 0;
  int PC_index = 0;
  int global_histo_name_index = 0;


  std::cout<<"Loading histograms and matricies from merge_xs.root files"<<std::endl;
  for(int xs=0;xs<n_xs;xs++){

    // Load files
    std::string filename1     =     "merge_xs" + std::to_string(xs+1) + ".root";
    std::cout<<"Getting meas and pred for XS "<<xs<<" from: "<<filename1<<std::endl;
    TFile *file1 = new TFile(filename1.c_str());
    TMatrixD* mat_collapse_temp =  (TMatrixD*)file1->Get("mat_collapse") ; // Collapse			
    mat_collapse_lst.push_back(mat_collapse_temp);
    TVectorD* vec_signal_temp =  (TVectorD*)file1->Get("vec_signal") ; // XS in pbar
    vec_signal_lst.push_back(vec_signal_temp);
    TMatrixD* mat_R_temp =  (TMatrixD*)file1->Get("mat_R") ; // Smearing matrix			
    mat_R_lst.push_back(mat_R_temp);
    TMatrixD *cov_mat_add_temp  = (TMatrixD*)file1->Get("cov_mat_add"); // Additional uncertainties 

    // Form collapse matrix here
    for (Int_t i=0;i!=nbin_hist_xs_lst.at(xs);i++){
      for (Int_t j=0;j!=nbin_meas_xs_lst.at(xs);j++){
        mat_collapse(i+histo_index,j+meas_index) = (*mat_collapse_temp)(i,j);
      }
    }

    // Form transpose of the collapsed matrix
    TMatrixD mat_collapse_temp_T(nbin_meas_xs_lst.at(xs), nbin_hist_xs_lst.at(xs));
    mat_collapse_temp_T.Transpose(*mat_collapse_temp);

    // Form the R matrix here
    TMatrixD mat_R_collapse_temp = (mat_collapse_temp_T)*(*mat_R_temp);
    for (Int_t i=0;i!=nbin_true_lst.at(xs);i++){
      for (Int_t j=0;j!=nbin_meas_xs_lst.at(xs);j++){
          mat_R_collapse(j+meas_index,i+true_index) = mat_R_collapse_temp(j,i);
      }
    }
 
    // Get the dirt uncertainties
    TMatrixD cov_mat_add = mat_collapse_temp_T * (*cov_mat_add_temp) * (*mat_collapse_temp);
    for (Int_t i=0;i!=nbin_meas_xs_lst.at(xs);i++){
      hcov_add->SetBinContent(meas_index+i+1,meas_index+i+1,cov_mat_add(i,i));
    }
 
    // Form truth vector here
    for(int i=0;i!=nbin_true_lst.at(xs);i++){
      htrue_signal->SetBinContent(true_index+i+1,(*(vec_signal_temp))(i));
    }
    vec_signal_lst.push_back(vec_signal_temp); 


    // All the measured and pre-collapsed histograms
    for (int k=0;k<n_diff_bins_3D.at(xs);k++) {
      for (int i=0;i<n_diff_bins_2D.at(xs);i++) {
   
        // Get the histograms from the file   
        int FC_histo_name_index = k*n_diff_bins_2D.at(xs) + i + 1;
        int PC_histo_name_index = n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs) + FC_histo_name_index;

        std::string FC_data_str = "hdata_obsch_" + std::to_string(FC_histo_name_index);                      // Data numuCC FC
        std::string PC_data_str = "hdata_obsch_" + std::to_string(PC_histo_name_index);                      // Data numuCC PC
        if (use_fakedata) {
          FC_data_str = "hmc_obsch_" + std::to_string(FC_histo_name_index);                                  // Fakedata numuCC FC
          PC_data_str = "hmc_obsch_" + std::to_string(PC_histo_name_index);                                  // Fakedata numuCC PC
        }
        std::string FC_mc_str   = "hmc_obsch_"   + std::to_string(FC_histo_name_index);                                                    // Pred numuCC FC
        std::string PC_mc_str   = "hmc_obsch_"   + std::to_string(PC_histo_name_index);                                                    // Pred numuCC PC
        std::string histo_str_1 = "histo_"       + std::to_string(FC_histo_name_index + 0*2*n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs));  // Sig numuCC FC
        std::string histo_str_2 = "histo_"       + std::to_string(PC_histo_name_index + 0*2*n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs));  // Sig numuCC PC
        std::string histo_str_3 = "histo_"       + std::to_string(FC_histo_name_index + 1*2*n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs));  // Bkg numuCC FC
        std::string histo_str_4 = "histo_"       + std::to_string(PC_histo_name_index + 1*2*n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs));  // Bkg numuCC PC
        std::string histo_str_5 = "histo_"       + std::to_string(FC_histo_name_index + 2*2*n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs));  // EXT FC
        std::string histo_str_6 = "histo_"       + std::to_string(PC_histo_name_index + 2*2*n_diff_bins_2D.at(xs)*n_diff_bins_3D.at(xs));  // EXT PC

        TH1F *hdata_obsch_1_tmp = (TH1F*)file1->Get(FC_data_str.c_str());                         // Data numuCC FC
        TH1F *hdata_obsch_2_tmp = (TH1F*)file1->Get(PC_data_str.c_str());                         // Data numuCC PC
        TH1F *hmc_obsch_1_tmp   = (TH1F*)file1->Get(FC_mc_str.c_str());                           // Pred numuCC FC
        TH1F *hmc_obsch_2_tmp   = (TH1F*)file1->Get(PC_mc_str.c_str());                           // Pred numuCC PC
        TH1F *histo_1_tmp       = (TH1F*)file1->Get(histo_str_1.c_str());                         // Sig numuCC FC
        TH1F *histo_2_tmp       = (TH1F*)file1->Get(histo_str_2.c_str());                         // Sig numuCC PC
        TH1F *histo_3_tmp       = (TH1F*)file1->Get(histo_str_3.c_str());                         // Bkg numuCC FC
        TH1F *histo_4_tmp       = (TH1F*)file1->Get(histo_str_4.c_str());                         // Bkg numuCC PC
        TH1F *histo_5_tmp       = (TH1F*)file1->Get(histo_str_5.c_str());                         // EXT FC
        TH1F *histo_6_tmp       = (TH1F*)file1->Get(histo_str_6.c_str());                         // EXT PC

        // Add to the measured and uncollapse histograms containing all measurments 
        for (int j=0;j<nbin_FC_lst.at( global_histo_name_index );j++) {

          histo_1->SetBinContent(      FC_index+j+1, histo_1_tmp->GetBinContent(j+1));
          histo_3->SetBinContent(      FC_index+j+1, histo_3_tmp->GetBinContent(j+1));
          histo_5->SetBinContent(      FC_index+j+1, histo_5_tmp->GetBinContent(j+1));
          hdata_obsch_1->SetBinContent(FC_index+j+1, hdata_obsch_1_tmp->GetBinContent(j+1));
//std::cout << "FC_index, j+1 , obs  " << FC_index<< ",  " << (j+1) <<", " <<hmc_obsch_1_tmp->GetBinContent(j+1)<<std::endl;
          hmc_obsch_1->SetBinContent(  FC_index+j+1, hmc_obsch_1_tmp->GetBinContent(j+1));

          // Calculate the statistical uncertianties
          double meas = hdata_obsch_1_tmp->GetBinContent(j+1);
          double pred = hmc_obsch_1_tmp->GetBinContent(j+1);
          double content;
          if (pred !=0){
            if (meas == 0){
              content = pred/2.;
            }else{
              content = 3./(1./meas+2./pred);
            }
          }else{
            content = 0;
          }
          hcov_stat->SetBinContent(meas_FC_index+j+1,meas_FC_index+j+1,content);
          //std::cout << "i,j,FC,FC_content = " << xs << ",  " << j << ",  " << FC_index << ",     " << content << std::endl;
     
          // Form the nominal vector for scaling systeamtics
          int index1 = histo_FC_index + j;
          int index3 = index1 + nbin_meas_xs_lst.at(xs);
          int index5 = index1 + nbin_meas_xs_lst.at(xs)*2;
          vec_nominal(index1) = histo_1_tmp->GetBinContent(j+1);
          vec_nominal(index3) = histo_3_tmp->GetBinContent(j+1);
          vec_nominal(index5) = histo_5_tmp->GetBinContent(j+1);
          //std::cout<<"xs, global, index1, index3, index5 "<<xs<<", "<<global_histo_name_index<<", "<<index1<<", "<<index3<<", "<<index5<<std::endl; 
        
          if(use_fakedata){
            hmeas->SetBinContent(j+meas_FC_index+1, hdata_obsch_1_tmp->GetBinContent(j+1) - histo_3_tmp->GetBinContent(j+1) );
          }
          else{
            hmeas->SetBinContent(j+meas_FC_index+1, hdata_obsch_1_tmp->GetBinContent(j+1) - histo_3_tmp->GetBinContent(j+1) - histo_5_tmp->GetBinContent(j+1) );
            //std::cout<<"hmeas->GetBinContent("<<j+meas_FC_index+1<<") = "<<hmeas->GetBinContent(j+meas_FC_index+1)<<std::endl;
          }
          hpred->SetBinContent(j+meas_FC_index+1, hmc_obsch_1_tmp->GetBinContent(j+1) );
        }

        for (int j=0;j<nbin_PC_lst.at( global_histo_name_index );j++) {
          
          histo_2->SetBinContent(      PC_index+j+1, histo_2_tmp->GetBinContent(j+1));
          histo_4->SetBinContent(      PC_index+j+1, histo_4_tmp->GetBinContent(j+1));
          histo_6->SetBinContent(      PC_index+j+1, histo_6_tmp->GetBinContent(j+1));
          hdata_obsch_2->SetBinContent(PC_index+j+1, hdata_obsch_2_tmp->GetBinContent(j+1));
          //std::cout << "PC_index, j+1 , obs  " << PC_index<< ",  " << (j+1) <<", " <<hmc_obsch_2_tmp->GetBinContent(j+1) <<std::endl;
          hmc_obsch_2->SetBinContent(  PC_index+j+1, hmc_obsch_2_tmp->GetBinContent(j+1));
          // Calculate the statistical uncertianties
          double meas = hdata_obsch_2_tmp->GetBinContent(j+1);
          double pred = hmc_obsch_2_tmp->GetBinContent(j+1);
          double content;
          if (pred !=0){
            if (meas == 0){
              content = pred/2.;
            }else{
              content = 3./(1./meas+2./pred);
            }
          }else{
            content = 0;
          }
          hcov_stat->SetBinContent(meas_PC_index+j+1,meas_PC_index+j+1,content);
          //std::cout << "i,j,PC,PC_content = " << xs << ",  " << j << ",  " << PC_index << ",     " << content << std::endl; 
        
          // Form the nominal vector for scaling systeamtics
          int index2 = histo_PC_index + j;
          int index4 = index2 + nbin_meas_xs_lst.at(xs);
          int index6 = index2 + nbin_meas_xs_lst.at(xs)*2;
          vec_nominal(index2) = histo_2_tmp->GetBinContent(j+1);
          vec_nominal(index4) = histo_4_tmp->GetBinContent(j+1);
          vec_nominal(index6) = histo_6_tmp->GetBinContent(j+1);
          //std::cout<<"xs, global, index2, index4, index6 "<<xs<<", "<<global_histo_name_index<<", "<<index2<<", "<<index4<<", "<<index6<<std::endl;

          if(use_fakedata){
            hmeas->SetBinContent(j+meas_PC_index+1, hdata_obsch_2_tmp->GetBinContent(j+1) - histo_4_tmp->GetBinContent(j+1) );
          }
          else{
            hmeas->SetBinContent(j+meas_PC_index+1, hdata_obsch_2_tmp->GetBinContent(j+1) - histo_4_tmp->GetBinContent(j+1) - histo_6_tmp->GetBinContent(j+1) );
            //std::cout<<"hmeas->GetBinContent("<<j+meas_PC_index+1<<") = "<<hmeas->GetBinContent(j+meas_PC_index+1)<<std::endl;
          }
          hpred->SetBinContent(j+meas_PC_index+1, hmc_obsch_2_tmp->GetBinContent(j+1) );
        }

        // Update the indicies
        FC_index+=nbin_FC_lst.at( global_histo_name_index );
        PC_index+=nbin_PC_lst.at( global_histo_name_index );
        meas_FC_index+=nbin_FC_lst.at( global_histo_name_index );
        meas_PC_index+=nbin_PC_lst.at( global_histo_name_index );
        histo_FC_index+=nbin_FC_lst.at( global_histo_name_index);
        histo_PC_index+=nbin_PC_lst.at( global_histo_name_index);
        global_histo_name_index++;


      }
    }

    // Update the index but don't do it if it is the last xs to prevent a seg fault
    if(xs==n_xs-1){break;}
    histo_index+=nbin_hist_xs_lst.at(xs);
    histo_FC_index=histo_index;
    histo_PC_index=histo_index+nbin_hist_FC_xs_lst.at(xs+1)/3;
    true_index+=nbin_true_lst.at(xs);
    meas_index+=nbin_meas_xs_lst.at(xs);
    meas_FC_index = meas_index;
    meas_PC_index = meas_index + nbin_FC_xs_lst.at(xs+1);
    //std::cout<<"xs, histo_index, histo_FC_index, histo_PC_index, meas_index, true_index"<<xs+1<<", "<<histo_index<<", "<<histo_FC_index<<", "<<histo_PC_index<<", "<<meas_index<<", "<<true_index<<std::endl;
  }

  //vec_nominal.Print();

  // Get transpose of total R matrix
  TMatrixD mat_R_collapse_T(nbin_true,nbin_meas);
  mat_R_collapse_T.Transpose(mat_R_collapse);

  // Get transpose of total collapse matrix
  TMatrixD mat_collapse_T(nbin_meas, nbin_hist);
  mat_collapse_T.Transpose(mat_collapse);
 
  // Convert response matrix to histogram
  TH2D* hR = new TH2D("hR_full","hR_full",nbin_meas,0.5,nbin_meas+0.5, nbin_true, 0.5, nbin_true+0.5);
  for (Int_t i=0;i!=nbin_true;i++){
    for (Int_t j=0;j!=nbin_meas;j++){
      hR->SetBinContent(j+1,i+1,mat_R_collapse(j,i));
    }
  }
 
  std::cout<<std::endl;
  std::cout<<std::endl; 
  std::cout<<"Loading MC Stat Uncertainties"<<std::endl;
  std::cout<<std::endl;
  // MC statistical uncertainties ...
  TH2D *hcov_mcstat = new TH2D("hcov_full_mcstat","hcov_full_mcstat",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
  meas_index = 0;
  for(int xs=0;xs<n_xs;xs++){
    std::string file_name =  "mcstat/xs_tot" + std::to_string(xs+1) + ".log";
    std::cout<<"Getting mcstat uncertainties for XS "<<xs<<" from: "<<file_name<<std::endl;
    ifstream infile(file_name);
    double temp, temp1,err2;
    infile >> temp >> temp;
    for (Int_t i=0;i!=nbin_meas_xs_lst.at(xs);i++){
      infile >> temp >> temp >> temp >> err2 >> temp;
      hcov_mcstat->SetBinContent(meas_index+i+1,meas_index+i+1,err2);
      //std::cout<<"xs, i, err2: "<<xs<<"  "<<i<<"  "<<err2<<std::endl;
    }
    meas_index+=nbin_meas_xs_lst.at(xs);
    std::cout<<"XS "<<xs<<" completed"<<std::endl;
  }

  // Origional mcstat cor treatment, this only uses the mc to do the correlations and allows "on digonal blocks" to have the small bit of correlation from the boostrapping
  if(0){
    TFile tmp_file("./mcstat/run_pred_stat.root");
    TMatrixD *tmp_matrix = (TMatrixD*)tmp_file.Get("frac_full_cov_mat_0");
    std::cout<<"MC Stat Cor"<<endl;
    for (Int_t i=0;i!=nbin_hist;i++){
      for (Int_t j=0;j!=nbin_hist;j++){
        (*tmp_matrix)(i,j) *= vec_nominal(i) * vec_nominal(j);
      }
    }
    TMatrixD mat_mcstatcor = mat_collapse_T * (*tmp_matrix) * mat_collapse;
    for (Int_t i=0;i!=nbin_meas;i++){
      for (Int_t j=0;j!=nbin_meas;j++){
        if(i==j) { continue; }
        else {
          double ii = TMath::Sqrt(mat_mcstatcor(i,i));
          double jj = TMath::Sqrt(mat_mcstatcor(j,j));
          double covij = 0;
          if(ii==0 || jj==0){ covij = 0;}
          else{ covij = mat_mcstatcor(i,j)/ii/jj; }
          double mcstat = TMath::Sqrt(hcov_mcstat->GetBinContent(i+1,i+1))*TMath::Sqrt(hcov_mcstat->GetBinContent(j+1,j+1));
          hcov_mcstat->SetBinContent(i+1,j+1, covij*mcstat );
          double stat = TMath::Sqrt(hcov_stat->GetBinContent(i+1,i+1))*TMath::Sqrt(hcov_stat->GetBinContent(j+1,j+1));
          hcov_stat->SetBinContent(i+1,j+1, covij*stat );
        }
      }
    }
  }
// New version, data and MC boostrapped seperatly and no modification to on diagonal blocks
int start_index = 0;
int end_index = 0;
  if(mcstat_cor){
    TFile tmp_file("./mcstat/run_pred_stat.root");
    TMatrixD *tmp_matrix = (TMatrixD*)tmp_file.Get("frac_full_cov_mat_0");
    std::cout<<"Stat Cor"<<endl;
    for (Int_t i=0;i!=nbin_hist;i++){
      for (Int_t j=0;j!=nbin_hist;j++){
        (*tmp_matrix)(i,j) *= vec_nominal(i) * vec_nominal(j);
      }
    }
    TFile tmp_file("./mcstat/run_data_stat.root");
    TMatrixD *tmp_matrix2 = (TMatrixD*)tmp_file.Get("cov_mat_0");
    TMatrixD mat_statcor = (*tmp_matrix2); // No need to collapse the data
    for (Int_t i=0;i!=nbin_meas;i++){
      for(int xs=0;xs<n_xs;xs++){
        end_index+=nbin_meas_xs_lst.at(xs);
        for (Int_t j=start_index;j!=end_index;j++){
          if(i>=start_index && i<end_index) { continue; }
          else {
            double ii = TMath::Sqrt(mat_mcstatcor(i,i));
            double jj = TMath::Sqrt(mat_mcstatcor(j,j));
            double covij = 0;
            if(ii==0 || jj==0){ covij = 0;}
            else{ covij = mat_mcstatcor(i,j)/ii/jj; }
            double mcstat = TMath::Sqrt(hcov_mcstat->GetBinContent(i+1,i+1))*TMath::Sqrt(hcov_mcstat->GetBinContent(j+1,j+1));
            hcov_mcstat->SetBinContent(i+1,j+1, covij*mcstat );
            ii = TMath::Sqrt(mat_statcor(i,i));
            jj = TMath::Sqrt(mat_statcor(j,j));
            covij = 0;
            if(ii==0 || jj==0){ covij = 0;}
            else{ covij = mat_statcor(i,j)/ii/jj; }
	    double stat = TMath::Sqrt(hcov_stat->GetBinContent(i+1,i+1))*TMath::Sqrt(hcov_stat->GetBinContent(j+1,j+1));
            hcov_stat->SetBinContent(i+1,j+1, covij*stat );
          }
        }
      start_index=end_index;
      }
      start_index = 0;
      end_index = 0;
    }
  }

  // Detector systematics
  std::cout<<std::endl;
  std::cout<<std::endl;
  std::cout<<"Loading Detector Systematics"<<std::endl;
  std::cout<<endl;
  TH2D *hcov_det = new TH2D("hcov_full_det","hcov_full_det",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
  std::map<int, TString> map_det_str;
  std::string detvar_prefix = "./DetVar/";
  map_det_str[1]  = detvar_prefix+"cov_LYDown.root";
  map_det_str[2]  = detvar_prefix+"cov_LYRayleigh.root";
  map_det_str[3]  = detvar_prefix+"cov_Recomb2.root";
  map_det_str[4]  = detvar_prefix+"cov_SCE.root";
  map_det_str[6]  = detvar_prefix+"cov_WMThetaXZ.root";
  map_det_str[7]  = detvar_prefix+"cov_WMThetaYZ.root";
  map_det_str[8]  = detvar_prefix+"cov_WMX.root";
  map_det_str[9]  = detvar_prefix+"cov_WMYZ.root";
  map_det_str[10] = detvar_prefix+"cov_LYatt.root";
  TMatrixD frac_det(nbin_hist,nbin_hist);
  for (auto it = map_det_str.begin(); it!=map_det_str.end(); it++){
    int idx = it->first;
    TFile tmp_file(it->second);
    TMatrixD *tmp_matrix = (TMatrixD*)tmp_file.Get(Form("frac_cov_det_mat_%d",idx));
    std::cout<<"Loaded "<<Form("frac_cov_det_mat_%d",idx)<<" from "<<it->second<<std::endl;
    frac_det += (*tmp_matrix);
  }

  for (Int_t i=0;i!=nbin_hist;i++){
    for (Int_t j=0;j!=nbin_hist;j++){
      frac_det(i,j) *= vec_nominal(i) * vec_nominal(j);
    }
  }
  TMatrixD mat_det = mat_collapse_T * frac_det * mat_collapse;
  for (Int_t i=0;i!=nbin_meas;i++){
    for (Int_t j=0;j!=nbin_meas;j++){
      hcov_det->SetBinContent(i+1,j+1,mat_det(i,j));      
    }
  }
 

  std::cout<<std::endl;
  std::cout<<std::endl;
  std::cout<<"Loading Flux and re-int Systematics"<<std::endl;
  std::cout<<endl; 
  // Flux systematics
  TH2D *hcov_flux = new TH2D("hcov_full_flux","hcov_full_flux",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
  TH2D *hcov_geant = new TH2D("hcov_full_geant","hcov_full_geant",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5); 
 
  TMatrixD frac_flux(nbin_hist,nbin_hist);
  TMatrixD frac_geant(nbin_hist,nbin_hist);

  int n_flux_files = 13;
  int n_geant_files = 3;
  if(!split_geant) {
    n_flux_files +=n_geant_files;
    n_geant_files=0;
  }

  for(Int_t i=1;i!=n_flux_files+1;i++){
    TFile tmp_file(Form("./XsFlux/cov_%d.root",i));
    TMatrixD *tmp_matrix = (TMatrixD*)tmp_file.Get(Form("frac_cov_xf_mat_%d",i));
    frac_flux += *tmp_matrix;
  }
  for (Int_t i=0;i!=nbin_hist;i++){
    for (Int_t j=0;j!=nbin_hist;j++){
      frac_flux(i,j) *= vec_nominal(i) * vec_nominal(j);
    }
  }
  TMatrixD mat_flux = mat_collapse_T * frac_flux * mat_collapse;
  for (Int_t i=0;i!=nbin_meas;i++){
    for (Int_t j=0;j!=nbin_meas;j++){
      hcov_flux->SetBinContent(i+1,j+1,mat_flux(i,j));
    }
  }
  for(Int_t i=n_flux_files+1;i!=n_flux_files+n_geant_files+1;i++){
    TFile tmp_file(Form("./XsFlux/cov_%d.root",i));
    TMatrixD *tmp_matrix = (TMatrixD*)tmp_file.Get(Form("frac_cov_xf_mat_%d",i));
    frac_geant += *tmp_matrix;
  }
  for (Int_t i=0;i!=nbin_hist;i++){
    for (Int_t j=0;j!=nbin_hist;j++){
      frac_geant(i,j) *= vec_nominal(i) * vec_nominal(j);
    }
  }
  TMatrixD mat_geant = mat_collapse_T * frac_geant * mat_collapse;
  for (Int_t i=0;i!=nbin_meas;i++){
    for (Int_t j=0;j!=nbin_meas;j++){
      hcov_geant->SetBinContent(i+1,j+1,mat_geant(i,j));
    }
  }


  std::cout<<std::endl;
  std::cout<<std::endl; 
  std::cout<<"Loading XS and RW Systematics"<<std::endl;
  std::cout<<endl;

  TH2D *hcov_xs      = new TH2D("hcov_full_xs","hcov_full_xs",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
  TH2D *hcov_pot = new TH2D("hcov_full_pot","hcov_full_pot",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
  TH2D *hcov_target = new TH2D("hcov_full_target","hcov_full_target",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
  {
    TString xs_file_name = "./XsFlux/cov_xs_blockwise.root";
    TString xs_mat_name = "frac_cov_blockwise";
    TFile tmp_file(xs_file_name);
    TMatrixD *frac_xs = (TMatrixD*)tmp_file.Get(xs_mat_name);
    std::cout<<"Loaded "<<xs_mat_name<<" from "<<xs_file_name<<std::endl;

    TMatrixD *frac_pot = new TMatrixD(frac_xs->GetNrows(),frac_xs->GetNcols());
    TMatrixD *frac_target = new TMatrixD(frac_xs->GetNrows(),frac_xs->GetNcols());
    frac_pot->Zero();
    frac_target->Zero();

    if(split_target_pot){
      std::cout<<"frac_xs->GetNrows"<<frac_xs->GetNrows()<<std::endl;
      double pot_unc = 0.02;
      double t_unc = 0.01;
      for (Int_t i=0;i!=frac_xs->GetNrows();i++){
        for (Int_t j=0;j!=frac_xs->GetNcols();j++){
          (*frac_xs)(i,j)     = (*frac_xs)(i,j) - pot_unc*pot_unc;
          (*frac_pot)(i,j)    = pot_unc*pot_unc;
        }
      }

      std::cout<<"nbin_meas"<<nbin_meas<<std::endl;
      int meas_index_i = 0;
      int meas_index_j = 0;
      for(int xs_i=0;xs_i<n_xs;xs_i++){
        for (Int_t i=0;i!=nbin_meas_xs_lst.at(xs_i);i++){
          for(int xs_j=0;xs_j<n_xs;xs_j++){
            for (Int_t j=0;j!=nbin_meas_xs_lst.at(xs_j);j++){
              (*frac_xs)(i+meas_index_i,j+meas_index_j)     = (*frac_xs)(i+meas_index_i,j+meas_index_j) - t_unc*t_unc;
              (*frac_target)(i+meas_index_i,j+meas_index_j) = t_unc*t_unc;
            }
            meas_index_j+=nbin_hist_xs_lst.at(xs_j);
          }
          meas_index_j=0;
        }
        meas_index_i+=nbin_hist_xs_lst.at(xs_i);
      }
    }


    for (Int_t i=0;i!=nbin_hist;i++){
      for (Int_t j=0;j!=nbin_hist;j++){
        (*frac_xs)(i,j)     *= vec_nominal(i) * vec_nominal(j);
        (*frac_pot)(i,j)    *= vec_nominal(i) * vec_nominal(j);
        (*frac_target)(i,j) *= vec_nominal(i) * vec_nominal(j);
       }
    }

    TMatrixD mat_xs     = mat_collapse_T * (*frac_xs)     * mat_collapse;
    TMatrixD mat_pot    = mat_collapse_T * (*frac_pot)    * mat_collapse;
    TMatrixD mat_target = mat_collapse_T * (*frac_target) * mat_collapse;

    for (Int_t i=0;i!=nbin_meas;i++){
      for (Int_t j=0;j!=nbin_meas;j++){
        hcov_xs->SetBinContent(i+1,j+1,mat_xs(i,j));
        hcov_pot->SetBinContent(i+1,j+1,mat_pot(i,j));
        hcov_target->SetBinContent(i+1,j+1,mat_target(i,j));
      }
    }
  }
 

  TH2D *hcov_rw      = new TH2D("hcov_full_rw","hcov_full_rw",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
  if(rw_sys){
    TString rw_file_name = "./XsFlux/cov_rw_blockwise.root";
    TString rw_mat_name = "frac_cov_blockwise";
    TFile tmp_file(rw_file_name);
    TMatrixD *frac_rw = (TMatrixD*)tmp_file.Get(rw_mat_name);
    std::cout<<"Loaded "<<rw_mat_name<<" from "<<rw_file_name<<std::endl;

    for (Int_t i=0;i!=nbin_hist;i++){
      for (Int_t j=0;j!=nbin_hist;j++){
        (*frac_rw)(i,j) *= vec_nominal(i) * vec_nominal(j);
      }
    }
    TMatrixD mat_rw = mat_collapse_T * (*frac_rw) * mat_collapse;

    for (Int_t i=0;i!=nbin_meas;i++){
      for (Int_t j=0;j!=nbin_meas;j++){
        hcov_rw->SetBinContent(i+1,j+1,mat_rw(i,j));
      }
    }
  } 

  TH2D *hcov_rwcor      = new TH2D("hcov_full_rwcor","hcov_full_rwcor",nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
  if(rw_sys){
    TString rwcor_file_name = "./XsFlux/cov_rwcor_blockwise.root";
    TString rwcor_mat_name = "frac_cov_blockwise";
    TFile tmp_file(rwcor_file_name);
    TMatrixD *frac_rwcor = (TMatrixD*)tmp_file.Get(rwcor_mat_name);
    std::cout<<"Loaded "<<rwcor_mat_name<<" from "<<rwcor_file_name<<std::endl;

    for (Int_t i=0;i!=nbin_hist;i++){
      for (Int_t j=0;j!=nbin_hist;j++){
        (*frac_rwcor)(i,j) *= vec_nominal(i) * vec_nominal(j);
      }
    }
    TMatrixD mat_rwcor = mat_collapse_T * (*frac_rwcor) * mat_collapse;

    for (Int_t i=0;i!=nbin_meas;i++){
      for (Int_t j=0;j!=nbin_meas;j++){
        hcov_rwcor->SetBinContent(i+1,j+1,mat_rwcor(i,j));
      }
    }
  }


  std::cout<<std::endl;
  std::cout<<std::endl;
  std::cout<<"Combining into total covarience matrix"<<std::endl;
  std::cout<<"rw_sys = "<<rw_sys<<std::endl;
  std::cout<<"just_stat_uncertainty = "<<just_stat_uncertainty<<std::endl;
  std::cout<<"mcstat_cor = "<<mcstat_cor<<std::endl;
  std::cout<<endl;
  // Form the total covariance matrix
  TH2D *hcov_tot    = new TH2D("hcov_full_tot",   "hcov_full_tot",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
  hcov_tot->Add(hcov_stat);
  if (!just_stat_uncertainty) {
    hcov_tot->Add(hcov_mcstat);
    hcov_tot->Add(hcov_add);
    hcov_tot->Add(hcov_flux);
    hcov_tot->Add(hcov_geant);
    hcov_tot->Add(hcov_det);
    hcov_tot->Add(hcov_xs);
    hcov_tot->Add(hcov_pot);
    hcov_tot->Add(hcov_target);
    if(rw_sys){
      hcov_tot->Add(hcov_rw);
      hcov_tot->Add(hcov_rwcor);
    }
  }



std::cout     << "hpred"
              << "\thmeas"
              << "\thcov_tot"
              << "\thcov_stat"
              << "\thcov_mcstat"
              << "\thcov_add"
              << "\thcov_flux"
              << "\thcov_geant"
              << "\thcov_det"
              << "\thcov_xs"
              << "\thcov_pot"
              << "\thcov_target"
              << "\thcov_rw"
              << "\thcov_rwcor"
              << std::endl;

  for (Int_t i=0;i!=nbin_meas;i++){
    std::cout << i << " "
              << hpred->GetBinContent(i+1) << " " 
              << hmeas->GetBinContent(i+1) << " "
              << sqrt(hcov_tot->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
	      << sqrt(hcov_stat->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
	      << sqrt(hcov_mcstat->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
	      << sqrt(hcov_add->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
	      << sqrt(hcov_flux->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
              << sqrt(hcov_geant->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
	      << sqrt(hcov_det->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
	      << sqrt(hcov_xs->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
              << sqrt(hcov_pot->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
              << sqrt(hcov_target->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
              << sqrt(hcov_rw->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " "
              << sqrt(hcov_rwcor->GetBinContent(i+1,i+1))/hpred->GetBinContent(i+1) << " ";
	      std::cout << std::endl;
  }

  std::cout<<std::endl;
  std::cout<<std::endl;
  std::cout<<"Saving the output"<<std::endl; 
  std::cout<<"one_wiener = "<<one_wiener<<std::endl;
  std::cout<<endl;

  TFile *file;
  else file = new TFile("wiener.root","RECREATE");
  true_index = 0;
  meas_index = 0;
  if(!one_wiener){
    for(int xs=0; xs<n_xs; xs++){
      std::string ifilename     =     "wiener" + std::to_string(xs+1) + ".root";
      TFile *ifile = new TFile(ifilename.c_str(),"RECREATE");

      TH2D *ihcov_tot    = new TH2D("hcov_tot",   "hcov_tot",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_stat    = new TH2D("hcov_stat",   "hcov_stat",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_mcstat    = new TH2D("hcov_mcstat",   "hcov_mcstat",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_add    = new TH2D("hcov_add",   "hcov_add",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_det    = new TH2D("hcov_det",   "hcov_det",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_flux    = new TH2D("hcov_flux",   "hcov_flux",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_geant    = new TH2D("hcov_geant",   "hcov_geant",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_xs    = new TH2D("hcov_xs",   "hcov_xs",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_pot    = new TH2D("hcov_pot",   "hcov_pot",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_target    = new TH2D("hcov_target",   "hcov_target",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_rw    = new TH2D("hcov_rw",   "hcov_rw",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);
      TH2D *ihcov_rwcor    = new TH2D("hcov_rwcor",   "hcov_rwcor",   nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5,nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5);

      TH1D *ihmeas = new TH1D("hmeas","hmeas",nbin_meas_xs_lst.at(xs),0.5, nbin_meas_xs_lst.at(xs)+0.5);
      TH1D *ihpred = new TH1D("hpred","hpred",nbin_meas_xs_lst.at(xs),0.5, nbin_meas_xs_lst.at(xs)+0.5);

      TH1D *ihtrue_signal = new TH1D("htrue_signal","htrue_signal",nbin_true_lst.at(xs), 0.5, nbin_true_lst.at(xs)+0.5);

      TH2D* ihR = new TH2D("hR","hR",nbin_meas_xs_lst.at(xs),0.5,nbin_meas_xs_lst.at(xs)+0.5, nbin_true_lst.at(xs), 0.5, nbin_true_lst.at(xs)+0.5);

      for(int i=0; i<nbin_true_lst.at(xs); i++){
        ihtrue_signal->SetBinContent( i+1, htrue_signal->GetBinContent(true_index+i+1) );
      }

      for(int i=0; i<nbin_meas_xs_lst.at(xs); i++){
        ihmeas->SetBinContent( i+1, hmeas->GetBinContent(meas_index+i+1) );
        ihpred->SetBinContent( i+1, hpred->GetBinContent(meas_index+i+1) );
        for(int j=0; j<nbin_meas_xs_lst.at(xs); j++){
          ihcov_tot->SetBinContent(i+1,j+1, hcov_tot->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_stat->SetBinContent(i+1,j+1, hcov_stat->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_mcstat->SetBinContent(i+1,j+1, hcov_mcstat->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_add->SetBinContent(i+1,j+1, hcov_add->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_det->SetBinContent(i+1,j+1, hcov_det->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_flux->SetBinContent(i+1,j+1, hcov_flux->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_geant->SetBinContent(i+1,j+1, hcov_geant->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_xs->SetBinContent(i+1,j+1, hcov_xs->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_pot->SetBinContent(i+1,j+1, hcov_pot->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_target->SetBinContent(i+1,j+1, hcov_target->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_rw->SetBinContent(i+1,j+1, hcov_rw->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_rwcor->SetBinContent(i+1,j+1, hcov_rwcor->GetBinContent(meas_index+i+1,meas_index+j+1) );
        }
      }

      TMatrixD mat_collapse_T_temp(mat_collapse_lst.at(xs)->GetNcols(), mat_collapse_lst.at(xs)->GetNrows());
      mat_collapse_T_temp.Transpose(*mat_collapse_lst.at(xs));
      TMatrixD mat_R_collapse_temp = (mat_collapse_T_temp)*(*mat_R_lst.at(xs));

      for (Int_t i=0;i!=nbin_true_lst.at(xs);i++){
        for (Int_t j=0;j!=nbin_meas_xs_lst.at(xs);j++){
          ihR->SetBinContent(j+1,i+1,mat_R_collapse_temp(j,i));
        }
      }

      ihtrue_signal->SetDirectory(ifile);
      ihmeas->SetDirectory(ifile);
      ihpred->SetDirectory(ifile);
      ihR->SetDirectory(ifile);
      ihcov_stat->SetDirectory(ifile);
      ihcov_mcstat->SetDirectory(ifile);
      ihcov_add->SetDirectory(ifile);
      ihcov_det->SetDirectory(ifile);
      ihcov_flux->SetDirectory(ifile);
      ihcov_geant->SetDirectory(ifile);
      ihcov_xs->SetDirectory(ifile);
      ihcov_pot->SetDirectory(ifile);
      ihcov_target->SetDirectory(ifile);
      ihcov_rw->SetDirectory(ifile);
      ihcov_rwcor->SetDirectory(ifile);
      ihcov_tot->SetDirectory(ifile);

      true_index+=nbin_true_lst.at(xs);
      meas_index+=nbin_meas_xs_lst.at(xs);

      ifile->Write();
      ifile->Close();
      std::cout<<"saved "<<ifilename<<std::endl;
    }

  }
  
    TH2D *ihcov_tot    = new TH2D("hcov_tot",   "hcov_tot",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_stat    = new TH2D("hcov_stat",   "hcov_stat",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_mcstat    = new TH2D("hcov_mcstat",   "hcov_mcstat",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_add    = new TH2D("hcov_add",   "hcov_add",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_det    = new TH2D("hcov_det",   "hcov_det",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_flux    = new TH2D("hcov_flux",   "hcov_flux",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_geant    = new TH2D("hcov_geant",   "hcov_geant",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_xs    = new TH2D("hcov_xs",   "hcov_xs",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_pot    = new TH2D("hcov_pot",   "hcov_pot",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_target    = new TH2D("hcov_target",   "hcov_target",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_rw    = new TH2D("hcov_rw",   "hcov_rw",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);
    TH2D *ihcov_rwcor    = new TH2D("hcov_rwcor",   "hcov_rwcor",   nbin_meas,0.5,nbin_meas+0.5,nbin_meas,0.5,nbin_meas+0.5);

    TH1D *ihmeas = new TH1D("hmeas","hmeas",nbin_meas,0.5, nbin_meas+0.5);
    TH1D *ihpred = new TH1D("hpred","hpred",nbin_meas,0.5, nbin_meas+0.5);

    TH1D *ihtrue_signal = new TH1D("htrue_signal","htrue_signal",nbin_true, 0.5, nbin_true+0.5);

    TH2D* ihR = new TH2D("hR","hR",nbin_meas,0.5,nbin_meas+0.5, nbin_true, 0.5, nbin_true+0.5);

    for(int xs=0; xs<n_xs; xs++){
      for(int i=0; i<nbin_true_lst.at(xs); i++){
        ihtrue_signal->SetBinContent( true_index+i+1, htrue_signal->GetBinContent(true_index+i+1) );
      }

      for(int i=0; i<nbin_meas_xs_lst.at(xs); i++){
        ihmeas->SetBinContent( meas_index+i+1, hmeas->GetBinContent(meas_index+i+1) );
        ihpred->SetBinContent( meas_index+i+1, hpred->GetBinContent(meas_index+i+1) );
        for(int j=0; j<nbin_meas_xs_lst.at(xs); j++){
          ihcov_tot->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_tot->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_stat->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_stat->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_mcstat->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_mcstat->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_add->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_add->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_det->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_det->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_flux->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_flux->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_geant->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_geant->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_xs->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_xs->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_pot->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_pot->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_target->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_target->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_rw->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_rw->GetBinContent(meas_index+i+1,meas_index+j+1) );
          ihcov_rwcor->SetBinContent(meas_index+i+1,meas_index+j+1, hcov_rwcor->GetBinContent(meas_index+i+1,meas_index+j+1) );
        }
      }

      true_index+=nbin_true_lst.at(xs);
      meas_index+=nbin_meas_xs_lst.at(xs);
    }
    for (Int_t i=0;i!=nbin_true;i++){
      for (Int_t j=0;j!=nbin_meas;j++){
        ihR->SetBinContent(j+1,i+1,mat_R_collapse(j,i));
      }
    }

    ihtrue_signal->SetDirectory(file);
    ihmeas->SetDirectory(file);
    ihpred->SetDirectory(file);
    ihR->SetDirectory(file);
    ihcov_stat->SetDirectory(file);
    ihcov_mcstat->SetDirectory(file);
    ihcov_add->SetDirectory(file);
    ihcov_det->SetDirectory(file);
    ihcov_flux->SetDirectory(file);
    ihcov_geant->SetDirectory(file);
    ihcov_xs->SetDirectory(file);
    ihcov_pot->SetDirectory(file);
    ihcov_target->SetDirectory(file);
    ihcov_rw->SetDirectory(file);
    ihcov_rwcor->SetDirectory(file);
    ihcov_tot->SetDirectory(file);
    file->Write();
  

  hdata_obsch_1->SetDirectory(file);
  hdata_obsch_2->SetDirectory(file);
  hmc_obsch_1->SetDirectory(file);
  hmc_obsch_2->SetDirectory(file);
  htrue_signal->SetDirectory(file);
  hmeas->SetDirectory(file);
  hpred->SetDirectory(file);
  hR->SetDirectory(file);
  hcov_stat->SetDirectory(file);
  hcov_mcstat->SetDirectory(file);
  hcov_add->SetDirectory(file);
  hcov_det->SetDirectory(file);
  hcov_flux->SetDirectory(file);
  hcov_geant->SetDirectory(file);
  hcov_rw->SetDirectory(file);
  hcov_rwcor->SetDirectory(file);
  hcov_xs->SetDirectory(file);
  hcov_pot->SetDirectory(file);
  hcov_target->SetDirectory(file);
  hcov_tot->SetDirectory(file);

  file->Write();
  file->Close();
  else std::cout<<"saved wiener.root"<<std::endl;

}



void make_cov_xs_block_matrix(){

///////////////////////////////////////////////
  int n_xs = 5; //
  int sys_type = 17; // 17 for cov_xs, 18 and 19 for cov_rw and cov_rwcor
////////////////////////////////////////////////

  std::string filename1;
  if(sys_type==17) filename1 = "XsVars1.root";
  else if(sys_type==18) filename1 = "rwVars1.root";
  else if(sys_type==19) filename1 = "rwcorVars1.root";
  else{
    std::cout<<"ERROR: Unknows sys_type, setting sys_type==17"<<std::endl;
    sys_type=17;
    std::string filename = "XsVars1.root";
  }

  TFile *file1 = new TFile(filename1.c_str());
  TTree *t_knobs = (TTree*)file1->Get("knobs");
  Int_t universes;
  Int_t sup_universes;
  t_knobs->SetBranchAddress("universes", &universes);
  t_knobs->SetBranchAddress("sup_universes", &sup_universes);
  int nknobs = t_knobs->GetEntries(); 
  std::cout<<"knobs: "<<nknobs<<std::endl;

  int reco_bins = 0;
  std::vector< int > reco_bins_lst;

  std::vector< TVectorD* > vec_mean_lst;

  // Load all the cv from the diffrent files
  for(int xs=0;xs<n_xs;xs++){
    std::string filename;
    if(sys_type==17) filename = "XsVars" + std::to_string(xs+1) + ".root";
    else if(sys_type==18) filename = "rwVars" + std::to_string(xs+1) + ".root";
    else if(sys_type==19) filename = "rwcorVars" + std::to_string(xs+1) + ".root";
    
    TFile *file = new TFile(filename.c_str());
    
    TVectorD *vec_mean = (TVectorD*)file->Get("vec_mean");

    vec_mean_lst.push_back( vec_mean );

    reco_bins_lst.push_back(vec_mean->GetNrows());
    reco_bins += vec_mean->GetNrows();
    file->Close();
  }

//std::cout<<"N Reco Bins: "<<reco_bins<<std::endl;

  TMatrixD* cov = new TMatrixD(reco_bins,reco_bins);
  TMatrixD* cov_temp = new TMatrixD(reco_bins,reco_bins);

  // Merge the cvs together from the diffrent files into a single one
  TVectorD vec_mean(reco_bins);
  int reco_index = 0;
  for(int xs=0;xs<n_xs;xs++){ 
    for(int i=0; i<reco_bins_lst[xs]; i++){
      vec_mean(reco_index) = (*vec_mean_lst[xs])[i];
      reco_index++;
    }
  }

  // Loop over all the knobs to get the contribution to the covarience matrix for each
  for(int k=0; k<nknobs; k++){
    t_knobs->GetEntry(k);
    cov_temp->Zero();
    // Loop over all universes for this knob
    for(int n=0; n<universes; n++){

      // Holds the diffrence vectors for each xs for this universe
      std::vector< TVectorD* > x_lst;

      // Get the diffrence vectors from all of the files
      for(int xs=0;xs<n_xs;xs++){

        // Load the files that contain all of the 600 universes
        std::string filename;
        if(sys_type==17)filename = "XsVars" + std::to_string(xs+1) + ".root";
        else if(sys_type==18) filename = "rwVars" + std::to_string(xs+1) + ".root";
        else if(sys_type==19) filename = "rwcorVars" + std::to_string(xs+1) + ".root";

        TFile *file = new TFile(filename.c_str());
      
        std::string x_name = "knob" + std::to_string(k) + "_x" + std::to_string(n);
        TVectorD* x = (TVectorD*)file->Get(x_name.c_str());

        x_lst.push_back( x );
        file->Close();
      }

      // Merge the cvs together from the diffrent files into a single one
      TVectorD x(reco_bins);
      reco_index = 0;
      for(int xs=0;xs<n_xs;xs++){
        for(int i=0; i<reco_bins_lst[xs]; i++){
          x(reco_index) = (*x_lst[xs])(i);
          reco_index++;
        }
      }

      // Add the contribution from this universe to the covarience matrix
      for(int i=0; i<reco_bins; i++){
        for(int j=0; j<reco_bins; j++){
          (*cov_temp)(i,j) += (x(j)) * (x(i));
        }
      }

    }//n

    // Normalize to the number of universes
    if(universes==2) (*cov_temp) *= 1./sup_universes;
    else (*cov_temp) *= 1./universes;
    (*cov) += (*cov_temp);
  }//k

  //cov->Print();

  // If this is the xs systematic, add in the pot and target error
  if(sys_type==17){
    std::vector< TVectorD* >  x_pot_lst;
    std::vector< TVectorD* >  x_ntarget_lst;

    for(int xs=0;xs<n_xs;xs++){
      std::string filename = "XsVars" + std::to_string(xs+1) + ".root";
      if(sys_type==17) filename = "XsVars" + std::to_string(xs+1) + ".root";
      else if(sys_type==18) filename = "rwVars" + std::to_string(xs+1) + ".root"; 
      else if(sys_type==19) filename = "rwcorVars" + std::to_string(xs+1) + ".root";
      TFile *file = new TFile(filename.c_str());

      TVectorD *x_pot = (TVectorD*)file->Get("x_pot_err");
      TVectorD *x_ntarget = (TVectorD*)file->Get("x_ntarget_err");
      x_ntarget->Print();
      x_pot_lst.push_back( x_pot );
      x_ntarget_lst.push_back( x_ntarget );
      file->Close();
    }
    TVectorD *x_pot = new TVectorD(reco_bins);
    TVectorD *x_ntarget = new TVectorD(reco_bins);
    reco_index = 0;
    for(int xs=0;xs<n_xs;xs++){
      for(int i=0; i<reco_bins_lst[xs]; i++){
        (*x_pot)(reco_index) = (*(x_pot_lst[xs]))(i);
        (*x_ntarget)(reco_index) = (*(x_ntarget_lst[xs]))(i);   
        reco_index++;
      }
    }

    for(int i=0; i<reco_bins; i++){
      for(int j=0; j<reco_bins; j++){
        (*cov)(i,j) += (*x_pot)(i) * (*x_pot)(j);
        (*cov)(i,j) += (*x_ntarget)(i) * (*x_ntarget)(j);
      }
    }
  }
//cov->Print();


  // Build the fractionsl covarience matrix
  TMatrixD* frac_cov_xs_mat = new TMatrixD(cov->GetNrows(), cov->GetNcols());

  for (size_t i=0; i!= frac_cov_xs_mat->GetNrows(); i++){
    double val_1 = vec_mean(i);
    for (size_t j=0; j!=frac_cov_xs_mat->GetNrows();j++){
      double val_2 = vec_mean(j);
      double val = (*cov)(i,j);
      if (val_1 ==0 && val_2 == 0){
        (*frac_cov_xs_mat)(i,j) = 0;
      }else if (val_1 ==0 || val_2 ==0){
        if (val !=0){
          if (i==j){
            (*frac_cov_xs_mat)(i,j) = 0.;
          }else{
            (*frac_cov_xs_mat)(i,j) = 0;
          }
        }else{
          (*frac_cov_xs_mat)(i,j) = 0;
        }
      }else{
        (*frac_cov_xs_mat)(i,j) = val/val_1/val_2;
      }
    }
  }

  std::string ofilename;
  if(sys_type==17) ofilename = "cov_xs_blockwise.root";
  else if(sys_type==18) ofilename = "cov_rw_blockwise.root";
  else if(sys_type==19) ofilename = "cov_rwcor_blockwise.root";

  TFile *outfile = new TFile(ofilename.c_str(),"RECREATE"); 
  cov->Write("cov_blockwise");
  frac_cov_xs_mat->Write("frac_cov_blockwise");
  outfile->Close();

}

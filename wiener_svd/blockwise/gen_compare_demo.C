// Standard library includes
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ROOT includes
#include "TMatrixD.h"

// Main function is gen_compare_demo(pred_is_total=false).
// The first bool tells the function if the prediction is in terms of total cross section (true) or differential cross section (false, the default) for each bin.
// The second bool dictates if SVD (recommended) or LU decomposition is used to invert the covariance matrix.
// By default, the cross section results should be in xs.txt, the covariance matrix in cov.txt, and the additional smearing matrix in Ac.txt.
// The phase space limits are in real_bins.txt.
// The external prediction is contained in pred.txt.
// See gen_compare_demo() or the readme.txt for more information.





//Checks the file to see how many bins there are.
int get_n_bins( std::string input_file_name) {

  int n_bins = 0;

  // Get the table of matrix element values.
  std::ifstream matrix_table_file( input_file_name );

  // Skip the header line which contains the data column names.
  std::string dummy;
  std::getline( matrix_table_file, dummy );

  // Parse its contents from the remaining lines.
  std::string line;
  while ( std::getline(matrix_table_file, line) ) {
    if(line == "end") break;
    n_bins++;
  }

  return n_bins;

}



// Loads the file into a matirx.
TMatrixD load_matrix( std::string input_file_name, int lo=0, int hi=84) {

  // Get the table of matrix element values.
  std::ifstream matrix_table_file( input_file_name );

 // Declare a matrix of the appropriate size for our bins. 
  int num_bins = hi-lo+1;
  TMatrixD matrix( num_bins, num_bins );

  if(hi<=lo){std::cout<<"ERROR in load_matrix(): hi<=lo"<<std::endl; return matrix;}

  // Skip the header line which contains the data column names.
  std::string dummy;
  std::getline( matrix_table_file, dummy );

  // Parse its contents from the remaining lines.
  std::string line;
  while ( std::getline(matrix_table_file, line) ) {
    if(line == "end") break;

    // Read the given line.
    int global_binX, global_binY;
    double element;
    std::stringstream temp_ss( line );
    temp_ss >> global_binX >> global_binY >> element;

    // Check to see if we want to fill bins in this range and fill the matrix if we do.
    if( (global_binX>=lo && global_binX<=hi) && (global_binY>=lo && global_binY<=hi) ){
      matrix( global_binX-lo, global_binY-lo ) = element;
    }

    // We have passed the high index, end filling the matrix.
    if(global_binX>hi && global_binY>hi){break;}
  }

  return matrix;

}



// Loads the file into a vector.
TMatrixD load_vector( std::string input_file_name, int lo=0, int hi=84) {

  // Get the table of matrix element values.
  std::ifstream vector_table_file( input_file_name );

  // Declare a matrix of the appropriate size for our bins.
  int num_bins = hi-lo+1;
  TMatrixD vector( num_bins, 1 );

  if(hi<=lo){std::cout<<"ERROR in load_vector(): hi<=lo"<<std::endl; return vector;}

  // Skip the header line which contains the data column names.
  std::string dummy;
  std::getline( vector_table_file, dummy );

  // Parse its contents from the remaining lines.
  std::string line;
  while ( std::getline(vector_table_file, line) ) {
    if(line == "end") break;
    
    // Read the given line.
    int global_binX;
    double element;
    std::stringstream temp_ss( line );
    temp_ss >> global_binX >> element;

    // Check to see if we want to fill bins in this range and fill the vector if we do.   
    if( global_binX>=lo && global_binX<=hi ){
      vector( global_binX-lo, 0 ) = element;
    } 
    // We have passed the high index, end filling the vector.
    if( global_binX>hi ){ break; }
  }

  return vector;

}



// Loads the file phase space limits for the bins and reweight the prediction accordingly.
TMatrixD divide_by_binwidth( TMatrixD pred, std::string input_file_name) {

  // Get the file with the phase space limits the bins.
  std::ifstream vector_table_file( input_file_name );

  // Skip the header line which contains the data column names.
  std::string dummy;
  std::getline( vector_table_file, dummy );

  // Parse its contents from the remaining lines.
  std::string line;
  while ( std::getline(vector_table_file, line) ) {
    if(line == "end") break;

    // Read the given line.
    int global_binX, binX;
    double hi_row,lo_row, hi_subrow,lo_subrow, hi_element,lo_element;
    std::stringstream temp_ss( line );
    temp_ss >> global_binX >> binX >> lo_row >> hi_row >> lo_subrow >> hi_subrow >> lo_element >> hi_element;

    // Get the width of the bin using all three possible dimensions. Make sure the width is one if the dimension is not being used (ie hi_row=lo_row).
    double dx1 = hi_row-lo_row;
    if(dx1==0) {dx1=1;}
    double dx2 = hi_subrow-lo_subrow;
    if(dx2==0) {dx2=1;}
    double dx3 = hi_element-lo_element;
    if(dx3==0) {dx3=1;}
    double diff = dx1*dx2*dx3;

    // Divide the prediction of the total cross section by the bin width to obtain the differential one.
    double val = pred( global_binX, 0 );
    pred( global_binX, 0 ) = val/diff;
  }

  return pred;

}


// Calculate chi2 for a sub-set of bins in a given measurment.
// Note: The prediction should still contain an estimation for the cross section in bins not being used in the calculation but still corresponding to the same measuremnt.
//       This is required for Ac to properly smear the prediction. This smearing takes place in the gen_compare_demo(). 
double calc_chi2(TMatrixD data_full, TMatrixD pred_full, TMatrixD cov_full, std::vector<int> bins = {}, bool use_svd=true) {

  // Form smaller vectors and matrices containing only the bins we are interested in.
  // This is done by looping over all bins and adding the given one to the smaller matrix if it is in std::vector<int> bins.
  int num_bins = bins.size();
  int num_bins_full = cov_full.GetNrows();
  TMatrixD pred(num_bins,1);
  TMatrixD data(num_bins,1);
  TMatrixD cov(num_bins,num_bins);

  // Indicies for the small matrix.
  int i_index = 0;
  int j_index = 0;
  for(int i=0; i<num_bins_full; i++){
    bool flag_add_i = false;

    // Check if this i is in vector<int> bins, add it to pred and data if it is.
    for(int k=0; k<num_bins; k++){
      if(bins.at(k)==i){
        pred(i_index,0) = pred_full(i,0); 
        data(i_index,0) = data_full(i,0);
        // Set flag indicating this i is in vector<int> bins and move onto incrementing over j.
        flag_add_i = true; 
        break;
      }
    }

    // The given i bin is not in vector<int> bins, go onto the next one.
    if(!flag_add_i){continue;}

    // Now do the same, but for j.
    for(int j=0; j<num_bins_full; j++){
      for(int k=0; k<num_bins; k++){
        // Check if this i is in vector<int> bins, add the i,j bin to cov and Ac if it is.
        if(bins.at(k)==j){
          flag_add_i = true; 
          cov(i_index,j_index) = cov_full(i,j); 
          // Increment the index and go onto the next bin.
          j_index++;
          break;
        }
      }
    }
 
   // Reset the j index and go onto the next i bin. 
    j_index = 0;
    i_index++;
  }

  // Invert the covariance matrix. We recommend using SVD as it provides better numerical stability.
  TMatrixD inv_cov = cov;
  if(!use_svd){inv_cov.Invert();}
  else{
    TDecompSVD svd(cov);
    inv_cov = TMatrixDSym(cov.GetNrows(), svd.Invert().GetMatrixArray());
  }

  // Compute a vector of differences between the data and the smeared prediction.
  TMatrixD diff_vec = data;
  diff_vec -= pred;
  // Compute the transpose of this vector.
  TMatrixD diff_vec_tr = diff_vec;
  diff_vec_tr.T();

  // Multiply diff^{T} * cov^{-1} * diff to get the chi-squared metric value the goodness-of-fit.
  TMatrixD temp1( inv_cov, TMatrixD::kMult, diff_vec );
  TMatrixD temp2( diff_vec_tr, TMatrixD::kMult, temp1 );

  // We now have a 1x1 matrix containing the chi-squared value.
  double chi2 = temp2( 0, 0 );
  return chi2;

}



double calc_chi2(TMatrixD data_full, TMatrixD pred_full, TMatrixD cov_full, int lo_index=0, int hi_index=84, bool use_svd=true) {

  if(hi_index<=lo_index){std::cout<<"ERROR calc_chi2(): hi_index<=lo_index"<<std::endl; return 0;}

  // Create a vector of bins to be given to calc_chi2().
  std::vector<int> all_bins = {};
  for(int i=lo_index; i<=hi_index; i++){ all_bins.push_back(i); }

  // Return the chi2 value using calc_chi2().
  return calc_chi2(data_full, pred_full, cov_full, all_bins, use_svd);
}



void gen_compare_demo(bool pred_is_total=false, bool use_svd=true) {
  
  // Name of files containing the prediction, corss section measurments, covariance matrix and additional smearing matrix.
  const std::string data_file_name = "xs.txt";
  const std::string cov_file_name = "cov.txt";
  const std::string Ac_file_name = "Ac.txt";
  // Replace this one with an independent prediction. It need to contain all measuremtns, but it should contain an esitmate for the cross section in all bins in which it is comparing to this data. 
  // If bins are ommited, the smearing will not be properly accounting for the bias induced by unfolding.
  const std::string pred_file_name = "pred.txt"; 

  // Bin index:
  //0-84:    All bins
  //0-11:    0pNp Ppi
  //12-20:   Xp   Ppi
  //21-43:   0pNp CosThetaPi
  //44-60:   Xp   CosThetaPi
  //61-84:   Xp   [CosThetaPi, Ppi]

  // Check the cross section data file to see how many bins it has
  int num_bins = get_n_bins(data_file_name);

  // Load the column vector containing the prediction and data for all bins as a TMatrixD object.
  TMatrixD pred = load_vector( pred_file_name, 0, num_bins-1);
  TMatrixD data = load_vector( data_file_name, 0, num_bins-1);
  // Load the matrices needed for the comparison of the prediction to data.
  TMatrixD cov = load_matrix( cov_file_name, 0, num_bins-1);
  TMatrixD Ac = load_matrix( Ac_file_name, 0, num_bins-1);

  // Divide by the bin width if this the prediction has not been, in other words, it is the total cross section for the given bin, not the differential cross section.
  const std::string real_bins_file_name = "real_bins.txt";
  if(pred_is_total){ pred = divide_by_binwidth( pred, real_bins_file_name ); }
  // Multiply the prediction by the additional smearing matrix.
  TMatrixD Ac_times_pred( Ac, TMatrixD::kMult, pred );

  // Calculate chi2 over all bins.
  double chi2_all = calc_chi2(data, Ac_times_pred, cov, 0, num_bins-1, use_svd);
  std::cout<<"chi2 = " << chi2_all << " for all bins"<<std::endl;

  // Calculate chi2 between data and pred for 0pNp Ppi.
  double chi2_Ppi = calc_chi2(data, Ac_times_pred, cov, 0, 11, use_svd);
  std::cout<<"chi2 = " << chi2_Ppi << " for 0pNp Ppi"<<std::endl;

  // Calculate chi2 between data and pred for 0p Ppi.
  std::vector<int> Ppi_0p_bins = {};
  for(int i=0; i<=5; i++){ Ppi_0p_bins.push_back(i); }
  double chi2_Ppi_0p = calc_chi2(data, Ac_times_pred, cov, Ppi_0p_bins, use_svd);
  std::cout<<"chi2 = " << chi2_Ppi_0p << " for 0p Ppi"<<std::endl;

  // Calculate chi2 between data and pred for Np Ppi.
  std::vector<int> Ppi_Np_bins = {};
  for(int i=6; i<=11; i++){ Ppi_Np_bins.push_back(i); }
  double chi2_Ppi_Np = calc_chi2(data, Ac_times_pred, cov, Ppi_Np_bins, use_svd);
  std::cout<<"chi2 = " << chi2_Ppi_Np << " for Np Ppi"<<std::endl;

  // Calculate chi2 between data and pred for Xp Ppi.
  double chi2_Ppi_Xp = calc_chi2(data, Ac_times_pred, cov, 12, 20, use_svd);
  std::cout<<"chi2 = " << chi2_Ppi_Xp << " for Xp Ppi"<<std::endl;

  // Calculate chi2 between data and pred for 0pNp 2D [CosThetaPi, Ppi].
  double chi2_CosThetaPiPpi2d = calc_chi2(data, Ac_times_pred, cov, 61, 84, use_svd);
  std::cout<<"chi2 = " << chi2_CosThetaPiPpi2d << " for 0pNp [CosThetaPi, Ppi]"<<std::endl;

  // Calculate chi2 between data and pred for 0pNp blockwise Ppi and CosThetaPi.
  std::vector<int> PpiCosThetaPi_bins = {};
  for(int i=0; i<=11; i++){ PpiCosThetaPi_bins.push_back(i); }// Add the Ppi bins.
  for(int i=21; i<=43; i++){ PpiCosThetaPi_bins.push_back(i); }// Add the CosThetaPi bins.
  double chi2_PpiCosThetaPi = calc_chi2(data, Ac_times_pred, cov, PpiCosThetaPi_bins, use_svd);
  std::cout<<"chi2 = " << chi2_PpiCosThetaPi << " for 0pNp blockwse Ppi and CosThetaPi"<<std::endl;
}



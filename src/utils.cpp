#include "rsparse.h"

// [[Rcpp::export]]
Rcpp::NumericVector cpp_make_sparse_approximation(const Rcpp::S4 &mat_template,
                                            const arma::mat& X,
                                            const arma::mat& Y,
                                            int sparse_matrix_type,
                                            unsigned n_threads) {
  Rcpp::IntegerVector rp = mat_template.slot("p");
  int* p = rp.begin();
  Rcpp::IntegerVector rj;
  if(sparse_matrix_type == CSR) {
    rj = mat_template.slot("j");
  } else if(sparse_matrix_type == CSC) {
    rj = mat_template.slot("i");
  } else
    Rcpp::stop("make_sparse_approximation_csr doesn't know sparse matrix type. Should be CSC=1 or CSR=2");

  uint32_t* j = (uint32_t *)rj.begin();
  Rcpp::IntegerVector dim = mat_template.slot("Dim");

  size_t nr = dim[0];
  size_t nc = dim[1];
  uint32_t N;
  if(sparse_matrix_type == CSR)
    N = nr;
  else
    N = nc;

  Rcpp::NumericVector approximated_values(rj.length());

  double *ptr_approximated_values = approximated_values.begin();
  #ifdef _OPENMP
  #pragma omp parallel for num_threads(n_threads) schedule(dynamic, GRAIN_SIZE)
  #endif
  for(uint32_t i = 0; i < N; i++) {
    int p1 = p[i];
    int p2 = p[i + 1];
    arma::rowvec xc;
    if(sparse_matrix_type == CSR)
      xc = X.col(i).t();
    else
      xc = Y.col(i).t();

    for(int pp = p1; pp < p2; pp++) {
      uint64_t ind = (size_t)j[pp];
      if(sparse_matrix_type == CSR)
        ptr_approximated_values[pp] = as_scalar(xc * Y.col(ind));
      else
        ptr_approximated_values[pp] = as_scalar(xc * X.col(ind));
    }

  }
  return(approximated_values);
}

dMappedCSR extract_mapped_csr(Rcpp::S4 input) {
  Rcpp::IntegerVector dim = input.slot("Dim");
  Rcpp::NumericVector value = input.slot("x");
  uint32_t nrows = dim[0];
  uint32_t ncols = dim[1];
  Rcpp::IntegerVector rj = input.slot("j");
  Rcpp::IntegerVector rp = input.slot("p");
  return dMappedCSR(nrows, ncols, value.length(), (uint32_t *)rj.begin(), (uint32_t *)rp.begin(), value.begin());
}

dMappedCSC extract_mapped_csc(Rcpp::S4 input) {
  Rcpp::IntegerVector dim = input.slot("Dim");
  Rcpp::NumericVector values = input.slot("x");
  uint32_t nrows = dim[0];
  uint32_t ncols = dim[1];
  Rcpp::IntegerVector row_indices = input.slot("i");
  Rcpp::IntegerVector col_ptrs = input.slot("p");
  return dMappedCSC(nrows, ncols, values.length(), (uint32_t *)row_indices.begin(), (uint32_t *)col_ptrs.begin(), values.begin());
}

// returns number of available threads
// omp_get_num_threads() for some reason doesn't work on all systems
// check following link
// http://stackoverflow.com/questions/11071116/i-got-omp-get-num-threads-always-return-1-in-gcc-works-in-icc
int omp_thread_count() {
  int n = 0;
  #ifdef _OPENMP
  #pragma omp parallel reduction(+:n)
  #endif
  n += 1;
  return n;
}

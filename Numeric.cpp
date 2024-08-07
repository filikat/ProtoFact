#include "Numeric.h"

void Numeric::Lsolve(std::vector<double>& x) const {
  // Forward solve.
  // Blas calls: dtrsv_, dgemv_

  // variables for BLAS calls
  const char LL = 'L';
  const char NN = 'N';
  const char TT = 'T';
  const char UU = 'U';
  const int i_one = 1;
  const double d_one = 1.0;
  const double d_zero = 0.0;

  // unit diagonal for augmented system only
  const char DD = S->Type() == FactType::NormEq ? 'N' : 'U';

  if (S->Packed() == PackType::Hybrid || S->Packed() == PackType::Hybrid2) {
    // supernode columns in hybrid-blocked format

    const int nb = S->BlockSize();

    for (int sn = 0; sn < S->Sn(); ++sn) {
      // leading size of supernode
      const int ldSn = S->Ptr(sn + 1) - S->Ptr(sn);

      // number of columns in the supernode
      const int sn_size = S->SnStart(sn + 1) - S->SnStart(sn);

      // first colums of the supernode
      const int sn_start = S->SnStart(sn);

      // index to access S->rows for this supernode
      const int start_row = S->Ptr(sn);

      // number of blocks of columns
      const int n_blocks = (sn_size - 1) / nb + 1;

      // index to access SnColumns[sn]
      int SnCol_ind{};

      // go through blocks of columns for this supernode
      for (int j = 0; j < n_blocks; ++j) {
        // number of columns in the block
        const int jb = std::min(nb, sn_size - nb * j);

        // number of entries in diagonal part
        const int diag_entries = jb * (jb + 1) / 2;

        // index to access vector x
        const int x_start = sn_start + nb * j;

        dtpsv_(&UU, &TT, &DD, &jb, &SnColumns[sn][SnCol_ind], &x[x_start],
               &i_one);
        SnCol_ind += diag_entries;

        // temporary space for gemv
        const int gemv_space = ldSn - nb * j - jb;
        std::vector<double> y(gemv_space);

        dgemv_(&TT, &jb, &gemv_space, &d_one, &SnColumns[sn][SnCol_ind], &jb,
               &x[x_start], &i_one, &d_zero, y.data(), &i_one);
        SnCol_ind += jb * gemv_space;

        // scatter solution of gemv
        for (int i = 0; i < gemv_space; ++i) {
          const int row = S->Rows(start_row + nb * j + jb + i);
          x[row] -= y[i];
        }
      }
    }

  } else {
    // supernode columns in full format

    for (int sn = 0; sn < S->Sn(); ++sn) {
      // leading size of supernode
      const int ldSn = S->Ptr(sn + 1) - S->Ptr(sn);

      // number of columns in the supernode
      const int sn_size = S->SnStart(sn + 1) - S->SnStart(sn);

      // first colums of the supernode
      const int sn_start = S->SnStart(sn);

      // size of clique of supernode
      const int clique_size = ldSn - sn_size;

      // index to access S->rows for this supernode
      const int start_row = S->Ptr(sn);

      dtrsv_(&LL, &NN, &DD, &sn_size, SnColumns[sn].data(), &ldSn, &x[sn_start],
             &i_one);

      // temporary space for gemv
      std::vector<double> y(clique_size);

      dgemv_(&NN, &clique_size, &sn_size, &d_one, &SnColumns[sn][sn_size],
             &ldSn, &x[sn_start], &i_one, &d_zero, y.data(), &i_one);

      // scatter solution of gemv
      for (int i = 0; i < clique_size; ++i) {
        const int row = S->Rows(start_row + sn_size + i);
        x[row] -= y[i];
      }
    }
  }
}

void Numeric::Ltsolve(std::vector<double>& x) const {
  // Backward solve.
  // Blas calls: dgemv_, dtrsv_

  // variables for BLAS calls
  const char LL = 'L';
  const char NN = 'N';
  const char TT = 'T';
  const char UU = 'U';
  const int i_one = 1;
  const double d_m_one = -1.0;
  const double d_one = 1.0;
  const double d_zero = 0.0;

  // unit diagonal for augmented system only
  const char DD = S->Type() == FactType::NormEq ? 'N' : 'U';

  if (S->Packed() == PackType::Hybrid || S->Packed() == PackType::Hybrid2) {
    // supernode columns in hybrid-blocked format

    const int nb = S->BlockSize();

    // go through the sn in reverse order
    for (int sn = S->Sn() - 1; sn >= 0; --sn) {
      // leading size of supernode
      const int ldSn = S->Ptr(sn + 1) - S->Ptr(sn);

      // number of columns in the supernode
      const int sn_size = S->SnStart(sn + 1) - S->SnStart(sn);

      // first colums of the supernode
      const int sn_start = S->SnStart(sn);

      // index to access S->rows for this supernode
      const int start_row = S->Ptr(sn);

      // number of blocks of columns
      const int n_blocks = (sn_size - 1) / nb + 1;

      // index to access SnColumns[sn]
      // initialized with the total number of entries of SnColumns[sn]
      int SnCol_ind = ldSn * sn_size - sn_size * (sn_size - 1) / 2;

      // go through blocks of columns for this supernode in reverse order
      for (int j = n_blocks - 1; j >= 0; --j) {
        // number of columns in the block
        const int jb = std::min(nb, sn_size - nb * j);

        // number of entries in diagonal part
        const int diag_entries = jb * (jb + 1) / 2;

        // index to access vector x
        const int x_start = sn_start + nb * j;

        // temporary space for gemv
        const int gemv_space = ldSn - nb * j - jb;
        std::vector<double> y(gemv_space);

        // scatter entries into y
        for (int i = 0; i < gemv_space; ++i) {
          const int row = S->Rows(start_row + nb * j + jb + i);
          y[i] = x[row];
        }

        SnCol_ind -= jb * gemv_space;
        dgemv_(&NN, &jb, &gemv_space, &d_m_one, &SnColumns[sn][SnCol_ind], &jb,
               y.data(), &i_one, &d_one, &x[x_start], &i_one);

        SnCol_ind -= diag_entries;
        dtpsv_(&UU, &NN, &DD, &jb, &SnColumns[sn][SnCol_ind], &x[x_start],
               &i_one);
      }
    }
  } else {
    // supernode columns in full format

    // go through the sn in reverse order
    for (int sn = S->Sn() - 1; sn >= 0; --sn) {
      // leading size of supernode
      const int ldSn = S->Ptr(sn + 1) - S->Ptr(sn);

      // number of columns in the supernode
      const int sn_size = S->SnStart(sn + 1) - S->SnStart(sn);

      // first colums of the supernode
      const int sn_start = S->SnStart(sn);

      // size of clique of supernode
      const int clique_size = ldSn - sn_size;

      // index to access S->rows for this supernode
      const int start_row = S->Ptr(sn);

      // temporary space for gemv
      std::vector<double> y(clique_size);

      // scatter entries into y
      for (int i = 0; i < clique_size; ++i) {
        const int row = S->Rows(start_row + sn_size + i);
        y[i] = x[row];
      }

      dgemv_(&TT, &clique_size, &sn_size, &d_m_one, &SnColumns[sn][sn_size],
             &ldSn, y.data(), &i_one, &d_one, &x[sn_start], &i_one);

      dtrsv_(&LL, &TT, &DD, &sn_size, SnColumns[sn].data(), &ldSn, &x[sn_start],
             &i_one);
    }
  }
}

void Numeric::Dsolve(std::vector<double>& x) const {
  // Diagonal solve

  // Dsolve performed only for augmented system
  if (S->Type() == FactType::NormEq) return;

  if (S->Packed() == PackType::Hybrid || S->Packed() == PackType::Hybrid2) {
    // supernode columns in hybrid-blocked format

    const int nb = S->BlockSize();

    for (int sn = 0; sn < S->Sn(); ++sn) {
      // leading size of supernode
      const int ldSn = S->Ptr(sn + 1) - S->Ptr(sn);

      // number of columns in the supernode
      const int sn_size = S->SnStart(sn + 1) - S->SnStart(sn);

      // first colums of the supernode
      const int sn_start = S->SnStart(sn);

      // index to access S->rows for this supernode
      const int start_row = S->Ptr(sn);

      // number of blocks of columns
      const int n_blocks = (sn_size - 1) / nb + 1;

      // index to access diagonal part of block
      int diag_start{};

      // go through blocks of columns for this supernode
      for (int j = 0; j < n_blocks; ++j) {
        // number of columns in the block
        const int jb = std::min(nb, sn_size - nb * j);

        // go through columns of block
        for (int col = 0; col < jb; ++col) {
          const double d =
              SnColumns[sn][diag_start + (col + 1) * (col + 2) / 2 - 1];
          x[sn_start + nb * j + col] /= d;
        }

        // move diag_start forward by number of diagonal entries in block
        diag_start += jb * (jb + 1) / 2;

        // move diag_start forward by number of sub-diagonal entries in block
        diag_start += (ldSn - nb * j - jb) * jb;
      }
    }
  } else {
    // supernode columns in full format

    for (int sn = 0; sn < S->Sn(); ++sn) {
      // leading size of supernode
      const int ldSn = S->Ptr(sn + 1) - S->Ptr(sn);

      for (int col = S->SnStart(sn); col < S->SnStart(sn + 1); ++col) {
        // relative index of column within supernode
        const int j = col - S->SnStart(sn);

        // diagonal entry of column j
        const double d = SnColumns[sn][j + j * ldSn];

        x[col] /= d;
      }
    }
  }
}

void Numeric::Solve(std::vector<double>& x) const {
  PermuteVector(x, S->Perm());
  Lsolve(x);
  Dsolve(x);
  Ltsolve(x);
  PermuteVector(x, S->Iperm());
}

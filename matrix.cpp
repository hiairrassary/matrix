#include "matrix.hpp"

#include <cmath>
#include <iomanip>

/* #include <cblas.h> */ /* LINUX */
/* #include <lapacke.h> */ /* LINUX */
#include <Accelerate/Accelerate.h> /* MACOS */


matrix::matrix()
{
    m_columns = 1;
    m_rows = 1;

    m_data.resize(1, 0.0);
}


matrix::matrix(size_t n)
{
    if(n == 0)
    {
       throw std::runtime_error("matrix: constructor: dimension error");
    }

    m_columns = n;
    m_rows = n;

    m_data.resize(columns()*rows(), 0.0);
}


matrix::matrix(size_t n, size_t m)
{
    if(n == 0 || m == 0)
    {
       throw std::runtime_error("matrix: constructor: dimension error");
    }

    m_columns = m;
    m_rows = n;

    m_data.resize(columns()*rows(), 0.0);
}


matrix::matrix(const matrix & X)
{
    m_columns = X.m_columns;
    m_rows = X.m_rows;

    m_data = X.m_data;
}


matrix::matrix(const std::initializer_list<std::initializer_list<double>> & list)
{
    /* TODO : Tester que toutes les listes font la bonne taille ! */

    m_columns = (*list.begin()).size();
    m_rows = list.size();

    m_data.reserve(columns()*rows());

    /* Column-major order */
    for(size_t i(0); i < columns(); ++i)
    {
        for(const std::initializer_list<double> & l : list)
        {
            m_data.push_back(*(l.begin() + i));
        }
    }

    /* Row-major order */
    /*
    for(const std::initializer_list<double> & l : list)
    {
        m_data.insert(m_data.end(), l.begin(), l.end());
    }
    */
}


matrix & matrix::operator=(const matrix & X)
{
    m_columns = X.m_columns;
    m_rows = X.m_rows;

    m_data = X.m_data;

    return *this;
}


double & matrix::at(size_t row_idx, size_t column_idx)
{
    if(row_idx >= rows() || column_idx >= columns())
    {
        throw std::runtime_error("matrix: at: out of bounds");
    }

    return m_data.at(row_idx + column_idx*rows());
}


const double & matrix::at(size_t row_idx, size_t column_idx) const
{
    if(row_idx >= rows() || column_idx >= columns())
    {
        throw std::runtime_error("matrix: at: out of bounds");
    }

    return m_data.at(row_idx + column_idx*rows());
}


double & matrix::operator()(size_t row_idx, size_t column_idx)
{
    return at(row_idx, column_idx);
}


const double & matrix::operator()(size_t row_idx, size_t column_idx) const
{
    return at(row_idx, column_idx);
}


size_t matrix::columns() const
{
    return m_columns;
}


size_t matrix::rows() const
{
    return m_rows;
}


void matrix::print(std::ostream & os) const
{
    os << std::fixed << std::setprecision(3) << std::right << std::showpos;

    for(size_t r(0); r < rows(); ++r)
    {
        for(size_t c(0); c < columns(); ++c)
        {
            os << std::setw(9) << at(r, c) << ' ';
        }
        
        os << '\n';
    }

    os << "(" << rows() << "x" << columns() << ")";
}


/*
    MATRIX::PLUS
    C = A + B adds arrays A and B and returns the result in C.
*/
matrix matrix::plus(const matrix & A, const matrix & B)
{
    if(A.rows() != B.rows() || A.columns() != B.columns())
    {
        throw std::runtime_error("matrix: plus: size error");
    }

    matrix C(A.rows(), A.columns());

    for(size_t i(0); i < C.m_data.size(); ++i)
    {
        C.m_data[i] = A.m_data[i] + B.m_data[i];
    }

    return C;
}


matrix matrix::plus(double n, const matrix & X)
{
    matrix Y(X);

    for(size_t i(0); i < Y.m_data.size(); ++i)
    {
        Y.m_data[i] = n + Y.m_data[i];
    }

    return Y;
}


/*
    MATRIX::MINUS
    C = A - B subtracts array B from array A and returns the result in C.
*/
matrix matrix::minus(const matrix & A, const matrix & B)
{
    if(A.rows() != B.rows() || A.columns() != B.columns())
    {
        throw std::runtime_error("matrix: minus: size error");
    }

    matrix C(A.rows(), A.columns());

    for(size_t i(0); i < C.m_data.size(); ++i)
    {
        C.m_data[i] = A.m_data[i] - B.m_data[i];
    }

    return C;
}


matrix matrix::minus(double n, const matrix & X)
{
    matrix Y(X);

    for(size_t i(0); i < Y.m_data.size(); ++i)
    {
        Y.m_data[i] =  n - Y.m_data[i];
    }

    return Y;
}


/*
    MATRIX::MLDIVIDE
    Solve systems of linear equations Ax = B for x.
*/
matrix matrix::mldivide(const matrix & A, const matrix & B)
{
    if(A.rows() != B.rows())
    {
        throw std::runtime_error("matrix: mldivide: size error");
    }

    matrix X = B;
    matrix Y = A;

    if(A.rows() == A.columns())
    {
        int n = (int)A.columns();
        int nrhs = (int)B.columns();
        std::vector<int> ipiv(A.columns());
        int info = 0;

        /* DGESV computes the solution to a real system of linear equations
               A * X = B,
           where A is an N-by-N matrix and X and B are N-by-NRHS matrices.
        */
        dgesv_(&n, &nrhs,
               &Y.m_data[0], &n,
               &ipiv[0],
               &X.m_data[0], &n,
               &info);

        /* Y: Details of LU factorization */
        /* X: Solution */

        if(info != 0)
        {
            throw std::runtime_error("matrix: mldivide: dgesv error");
        }
    }
    else
    {
        char trans = 'N';
        int m = (int)A.rows();
        int n = (int)A.columns();
        int nrhs = (int)B.columns();
        int lda = (int)A.rows();
        int ldb = (int)std::max(A.rows(), A.columns());
        double work_query = 0;
        int lwork = -1; /* Workspace query */
        int info = 0;

        /* Resize the matrix to get the solution matrix */
        X.m_data.resize(std::max(B.rows()*B.columns() ,A.columns()*B.columns()));

        /* DGELS solves overdetermined or underdetermined real linear systems
           involving an M-by-N matrix A, or its transpose, using a QR or LQ
           factorization of A.
        */
        /* Query and allocate the optimal workspace */
        dgels_(&trans,
               &m, &n, &nrhs,
               &Y.m_data[0], &lda,
               &X.m_data[0], &ldb,
               &work_query, &lwork,
               &info);

        if(info != 0)
        {
            throw std::runtime_error("matrix: mldivide: dgels error");
        }

        lwork = (int)work_query;
        std::vector<double> work((size_t)lwork);

        /* Solve the equations A*X = B */
        dgels_(&trans,
               &m, &n, &nrhs,
               &Y.m_data[0], &lda,
               &X.m_data[0], &ldb,
               &work[0], &lwork,
               &info);

        /* Y: Details of factorization */
        /* X: Solution */

        if(info != 0)
        {
            throw std::runtime_error("matrix: mldivide: dgels error");
        }

        /* Update solution matrix dimension */
        X.m_columns = B.columns();
        X.m_rows = A.columns();

        /* Reconstruct the solution matrix when M > N */
        if(m > n)
        {
            size_t i = 0;
            size_t j = 0;
            size_t k = 0;

            while(i < X.columns()*X.rows())
            {
                for(k = 0; k < X.rows(); ++k)
                {
                    X.m_data[i + k] = X.m_data[j + k];
                }

                j += B.rows();
                i += X.rows();
            }

            X.m_data.resize(X.columns()*X.rows());
        }
    }

    return X;
}


/*
    MATRIX::MTIMES
    C = A*B is the matrix product of A and B.
*/
matrix matrix::mtimes(const matrix & A, const matrix & B)
{
    if(A.columns() != B.rows())
    {
        throw std::runtime_error("matrix: mtimes: size error");
    }

    matrix C(A.rows(), B.columns());

    /* DGEMM performs one of the matrix-matrix operations
           C := alpha*op( A )*op( B ) + beta*C
    */
    cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
                (int)A.rows(), (int)B.columns(), (int)A.columns(),
                1.0, /* alpha */
                &A.m_data[0], (int)A.rows(),
                &B.m_data[0], (int)B.rows(),
                0.0, /* beta */
                &C.m_data[0], (int)C.rows());

    return C;
}


/*
    MATRIX::TIMES
    C = times(A,B) multiplies arrays A and B element by element and returns the result in C.
*/
matrix matrix::times(const matrix & A, const matrix & B)
{
    if(A.rows() != B.rows() || A.columns() != B.columns())
    {
        throw std::runtime_error("matrix: times: size error");
    }

    matrix C(A.rows(), A.columns());

    for(size_t i(0); i < C.m_data.size(); ++i)
    {
        C.m_data[i] = A.m_data[i] * B.m_data[i];
    }

    return C;
}


matrix matrix::times(double n, const matrix & X)
{
    matrix Y(X);

    for(size_t i(0); i < Y.m_data.size(); ++i)
    {
        Y.m_data[i] =  n * Y.m_data[i];
    }

    return Y;
}


/*
    MATRIX::INV
    Y = inv(X) computes the inverse of square matrix X.
*/
matrix matrix::inv(const matrix & X)
{
    if(X.rows() != X.columns())
    {
        throw std::runtime_error("matrix: inv: size error");
    }

    matrix Y = X;

    int m = (int)Y.rows();
    int n = (int)Y.columns();
    std::vector<int> ipiv(std::min(Y.rows(), Y.columns()));
    int info = 0;

    /* DGETRF computes an LU factorization of a general M-by-N matrix A
       using partial pivoting with row interchanges.
    */
    dgetrf_(&m, &n,
            &Y.m_data[0], &m,
            &ipiv[0],
            &info);

    if(info != 0)
    {
        throw std::runtime_error("matrix: inv: dgetrf error");
    }


    int lwork = n;
    std::vector<double> work((size_t)lwork);

    /* DGETRI computes the inverse of a matrix using the LU factorization
       computed by DGETRF.
    */
    dgetri_(&n,
            &Y.m_data[0], &n,
            &ipiv[0],
            &work[0], &lwork,
            &info);

    if(info != 0)
    {
        throw std::runtime_error("matrix: inv: dgetri error");
    }

    return Y;
}


/*
    MATRIX::PINV
    B = pinv(A) returns the Moore-Penrose pseudoinverse of A.
*/
matrix matrix::pinv(const matrix & X)
{
    size_t dim = std::max(X.columns(), X.rows());

    matrix Y = X;
    matrix W(dim);

    /* Create an identity matrix */
    for(size_t i(0); i < dim; ++i)
    {
        W.at(i, i) = 1.0;
    }

    /* Resize the matrix to get the solution matrix */
    W.m_data.resize(std::max(Y.rows()*Y.rows(), Y.rows()*Y.columns()));

    int m = (int)Y.rows();
    int n = (int)Y.columns();
    int nrhs = (int)W.columns();
    int lda = (int)Y.rows();
    int ldb = (int)std::max(Y.rows(), Y.columns());
    std::vector<double> s(std::min(Y.rows(), Y.columns()));
    double rcond = -1.0;
    int rank = -1;
    double work_query = 0;
    int lwork = -1; /* Workspace query */
    int info = 0;

    /* DGELSS computes the minimum norm solution to a real linear least
       squares problem:
           Minimize 2-norm(| b - A*x |).

       using the singular value decomposition (SVD) of A. A is an M-by-N
       matrix which may be rank-deficient.
    */
    /* Query and allocate the optimal workspace */
    dgelss_(&m, &n, &nrhs,
            &Y.m_data[0], &lda,
            &W.m_data[0], &ldb,
            &s[0],
            &rcond, &rank,
            &work_query, &lwork,
            &info);

    if(info != 0)
    {
        throw std::runtime_error("matrix: pinv: dgelss error");
    }

    lwork = (int)work_query;
    std::vector<double> work((size_t)lwork);

    /* Solve the solution */
    dgelss_(&m, &n, &nrhs,
        &Y.m_data[0], &lda,
        &W.m_data[0], &ldb,
        &s[0],
        &rcond, &rank,
        &work[0], &lwork,
        &info);

    if(info != 0)
    {
        throw std::runtime_error("matrix: pinv: dgelss error");
    }

    /* Update solution matrix dimension */
    W.m_columns = Y.rows();
    W.m_rows = Y.columns();

    /* Reconstruct the solution matrix when M > N */
    if(m > n)
    {
        size_t i = 0;
        size_t j = 0;
        size_t k = 0;

        while(i < W.columns()*W.rows())
        {
            for(k = 0; k < W.rows(); ++k)
            {
                W.m_data[i + k] = W.m_data[j + k];
            }

            j += Y.rows();
            i += W.rows();
        }

        W.m_data.resize(W.columns()*W.rows());
    }

    return W;
}


matrix matrix::transpose(const matrix & X)
{
    matrix Y(X.columns(), X.rows());

    size_t m = X.rows();
    size_t n = X.columns();


    for(size_t i(0); i < m; ++i)
    {
        for(size_t j(0); j < n; ++j)
        {
            Y.m_data[i*n + j] = X.m_data[j*m + i];
        }
    }

    return Y;
}


bool matrix::equal(const matrix & A, const matrix & B, const double epsilon)
{
    bool result = (A.rows() == B.rows() && A.columns() == B.columns());


    for(size_t i(0); i < A.m_data.size() && result; ++i)
    {
        if(!(std::abs(A.m_data[i] - B.m_data[i]) < epsilon))
        {
            result = false;
        }
    }

    return result;
}




matrix operator+(const matrix & A, const matrix & B)
{
    return matrix::plus(A, B);
}


matrix operator+(double n, const matrix & X)
{
    return matrix::plus(n, X);
}


matrix operator-(const matrix & A, const matrix & B)
{
    return matrix::minus(A, B);
}


matrix operator-(double n, const matrix & X)
{
    return matrix::minus(n, X);
}


matrix operator*(const matrix & A, const matrix & B)
{
    return matrix::mtimes(A, B);
}


matrix operator*(double n, const matrix & X)
{
    return matrix::times(n, X);
}


bool operator==(const matrix & A, const matrix & B)
{
    return matrix::equal(A, B, 1.0e-12);
}


bool operator!=(const matrix & A, const matrix & B)
{
    return !matrix::equal(A, B, 1.0e-12);
}


std::ostream & operator<<(std::ostream & os, const matrix & X)
{
    X.print(os);

    return os;
}


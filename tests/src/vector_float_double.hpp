/* =========================================================================
   Copyright (c) 2010-2013, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.
   Portions of this software are copyright by UChicago Argonne, LLC.

                            -----------------
                  ViennaCL - The Vienna Computing Library
                            -----------------

   Project Head:    Karl Rupp                   rupp@iue.tuwien.ac.at

   (A list of authors and contributors can be found in the PDF manual)

   License:         MIT (X11), see file LICENSE in the base directory
============================================================================= */

//
// *** System
//
#include <iostream>
#include <iomanip>
#include <cmath>

//
// *** Boost
//
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>

//
// *** ViennaCL
//
//#define VIENNACL_DEBUG_ALL
#define VIENNACL_WITH_UBLAS 1
#include "viennacl/vector.hpp"
#include "viennacl/vector_proxy.hpp"
#include "viennacl/linalg/inner_prod.hpp"
#include "viennacl/linalg/norm_1.hpp"
#include "viennacl/linalg/norm_2.hpp"
#include "viennacl/linalg/norm_inf.hpp"

#include "Random.hpp"

using namespace boost::numeric;


//
// -------------------------------------------------------------
//
template <typename ScalarType>
ScalarType diff(ScalarType const & s1, ScalarType const & s2)
{
   viennacl::backend::finish();
   if (s1 != s2)
      return (s1 - s2) / std::max(std::fabs(s1), std::fabs(s2));
   return 0;
}
//
// -------------------------------------------------------------
//
template <typename ScalarType>
ScalarType diff(ScalarType const & s1, viennacl::scalar<ScalarType> const & s2)
{
   viennacl::backend::finish();
   if (s1 != s2)
      return (s1 - s2) / std::max(std::fabs(s1), std::fabs(s2));
   return 0;
}
//
// -------------------------------------------------------------
//
template <typename ScalarType>
ScalarType diff(ScalarType const & s1, viennacl::entry_proxy<ScalarType> const & s2)
{
   viennacl::backend::finish();
   if (s1 != s2)
      return (s1 - s2) / std::max(std::fabs(s1), std::fabs(s2));
   return 0;
}
//
// -------------------------------------------------------------
//
template <typename ScalarType, typename ViennaCLVectorType>
ScalarType diff(ublas::vector<ScalarType> const & v1, ViennaCLVectorType const & vcl_vec)
{
   ublas::vector<ScalarType> v2_cpu(vcl_vec.size());
   viennacl::backend::finish();
   viennacl::copy(vcl_vec, v2_cpu);

   for (unsigned int i=0;i<v1.size(); ++i)
   {
      if ( std::max( std::fabs(v2_cpu[i]), std::fabs(v1[i]) ) > 0 )
         v2_cpu[i] = std::fabs(v2_cpu[i] - v1[i]) / std::max( std::fabs(v2_cpu[i]), std::fabs(v1[i]) );
      else
         v2_cpu[i] = 0.0;
   }

   return ublas::norm_inf(v2_cpu);
}


template <typename T1, typename T2>
int check(T1 const & t1, T2 const & t2, double epsilon)
{
  int retval = EXIT_SUCCESS;

  double temp = std::fabs(diff(t1, t2));
  if (temp > epsilon)
  {
    std::cout << "# Error! Relative difference: " << temp << std::endl;
    retval = EXIT_FAILURE;
  }
  return retval;
}


//
// -------------------------------------------------------------
//
template< typename NumericT, typename Epsilon, typename UblasVectorType, typename ViennaCLVectorType1, typename ViennaCLVectorType2 >
int test(Epsilon const& epsilon,
         UblasVectorType     & ublas_v1, UblasVectorType     & ublas_v2,
         ViennaCLVectorType1 &   vcl_v1, ViennaCLVectorType2 &   vcl_v2)
{
  int retval = EXIT_SUCCESS;

  NumericT                    cpu_result = 42.0;
  viennacl::scalar<NumericT>  gpu_result = 43.0;

  //
  // Initializer:
  //
  std::cout << "Checking for zero_vector initializer..." << std::endl;
  ublas_v1 = ublas::zero_vector<NumericT>(ublas_v1.size());
  vcl_v1 = viennacl::zero_vector<NumericT>(vcl_v1.size());
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Checking for scalar_vector initializer..." << std::endl;
  ublas_v1 = ublas::scalar_vector<NumericT>(ublas_v1.size(), cpu_result);
  vcl_v1 = viennacl::scalar_vector<NumericT>(vcl_v1.size(), cpu_result);
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1 = ublas::scalar_vector<NumericT>(ublas_v1.size(), gpu_result);
  vcl_v1 = viennacl::scalar_vector<NumericT>(vcl_v1.size(), gpu_result);
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Checking for unit_vector initializer..." << std::endl;
  ublas_v1 = ublas::unit_vector<NumericT>(ublas_v1.size(), 5);
  vcl_v1 = viennacl::unit_vector<NumericT>(vcl_v1.size(), 5);
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  for (std::size_t i=0; i<ublas_v1.size(); ++i)
  {
    ublas_v1[i] = NumericT(1.0) + random<NumericT>();
    ublas_v2[i] = NumericT(1.0) + random<NumericT>();
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());  //resync
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  std::cout << "Checking for successful copy..." << std::endl;
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;
  if (check(ublas_v2, vcl_v2, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  //
  // Part 1: Norms and inner product
  //

  // --------------------------------------------------------------------------
  std::cout << "Testing inner_prod..." << std::endl;
  cpu_result = viennacl::linalg::inner_prod(ublas_v1, ublas_v2);
  NumericT cpu_result2 = viennacl::linalg::inner_prod(vcl_v1, vcl_v2);
  gpu_result = viennacl::linalg::inner_prod(vcl_v1, vcl_v2);

  if (check(cpu_result, cpu_result2, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;
  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  cpu_result = inner_prod(ublas_v1 + ublas_v2, ublas_v2 - ublas_v1);
  NumericT cpu_result3 = viennacl::linalg::inner_prod(vcl_v1 + vcl_v2, vcl_v2 - vcl_v1);
  gpu_result = viennacl::linalg::inner_prod(vcl_v1 + vcl_v2, vcl_v2 - vcl_v1);

  if (check(cpu_result, cpu_result3, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;
  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  // --------------------------------------------------------------------------
  std::cout << "Testing norm_1..." << std::endl;
  cpu_result = ublas::norm_1(ublas_v1);
  gpu_result = viennacl::linalg::norm_1(vcl_v1);

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  gpu_result = 2 * cpu_result; //reset
  gpu_result = ublas::norm_1(ublas_v1);
  cpu_result = viennacl::linalg::norm_1(vcl_v1);

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  gpu_result = ublas::norm_1(ublas_v1 + ublas_v2);
  cpu_result = viennacl::linalg::norm_1(vcl_v1 + vcl_v2);

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  // --------------------------------------------------------------------------
  std::cout << "Testing norm_2..." << std::endl;
  cpu_result = ublas::norm_2(ublas_v1);
  gpu_result = viennacl::linalg::norm_2(vcl_v1);

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  gpu_result = 2 * cpu_result; //reset
  gpu_result = ublas::norm_2(ublas_v1);
  cpu_result = viennacl::linalg::norm_2(vcl_v1);

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  gpu_result = ublas::norm_2(ublas_v1 + ublas_v2);
  cpu_result = viennacl::linalg::norm_2(vcl_v1 + vcl_v2);

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  // --------------------------------------------------------------------------
  std::cout << "Testing norm_inf..." << std::endl;
  cpu_result = ublas::norm_inf(ublas_v1);
  gpu_result = viennacl::linalg::norm_inf(vcl_v1);

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  gpu_result = 2 * cpu_result; //reset
  gpu_result = ublas::norm_inf(ublas_v1);
  cpu_result = viennacl::linalg::norm_inf(vcl_v1);

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  gpu_result = ublas::norm_inf(ublas_v1 + ublas_v2);
  cpu_result = viennacl::linalg::norm_inf(vcl_v1 + vcl_v2);

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  // --------------------------------------------------------------------------
  std::cout << "Testing index_norm_inf..." << std::endl;
  std::size_t cpu_index = ublas::index_norm_inf(ublas_v1);
  std::size_t gpu_index = viennacl::linalg::index_norm_inf(vcl_v1);

  if (check(static_cast<NumericT>(cpu_index), static_cast<NumericT>(gpu_index), epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;
  // --------------------------------------------------------------------------
  cpu_result = ublas_v1[index_norm_inf(ublas_v1)];
  gpu_result = vcl_v1[viennacl::linalg::index_norm_inf(vcl_v1)];

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  cpu_result = ublas_v1[index_norm_inf(ublas_v1 + ublas_v2)];
  gpu_result = vcl_v1[viennacl::linalg::index_norm_inf(vcl_v1 + vcl_v2)];

  if (check(cpu_result, gpu_result, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  //
  // Plane rotation and assignments
  //

  // --------------------------------------------------------------------------

  ublas::vector<NumericT> x = ublas_v1;
  ublas::vector<NumericT> y = ublas_v2;
  ublas::vector<NumericT> t = ublas_v1;
  t.assign (NumericT(1.1) * x + NumericT(2.3) * y),
  y.assign (- NumericT(2.3) * x + NumericT(1.1) * y),
  x.assign (t);

  viennacl::linalg::plane_rotation(vcl_v1, vcl_v2, NumericT(1.1), NumericT(2.3));

  if (check(x, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;
  if (check(y, vcl_v2, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  // --------------------------------------------------------------------------

  std::cout << "Testing assignments..." << std::endl;
  NumericT val = static_cast<NumericT>(1e-1);
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = val;

  for (size_t i=0; i < vcl_v1.size(); ++i)
    vcl_v1(i) = val;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  //
  // multiplication and division of vectors by scalars
  //
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = NumericT(1.0) + random<NumericT>();
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());  //resync
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  std::cout << "Testing scaling with CPU scalar..." << std::endl;
  NumericT alpha = static_cast<NumericT>(1.7182);
  viennacl::scalar<NumericT> gpu_alpha = alpha;

  ublas_v1  *= alpha;
  vcl_v1    *= alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing scaling with GPU scalar..." << std::endl;
  ublas_v1  *= alpha;
  vcl_v1    *= gpu_alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing scaling with scalar expression..." << std::endl;
  ublas_v1  *= inner_prod(ublas_v1, ublas_v2);
  vcl_v1    *= viennacl::linalg::inner_prod(vcl_v1, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  NumericT beta  = static_cast<NumericT>(1.4153);
  viennacl::scalar<NumericT> gpu_beta = beta;

  std::cout << "Testing shrinking with CPU scalar..." << std::endl;
  ublas_v1 /= beta;
  vcl_v1   /= beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing shrinking with GPU scalar..." << std::endl;
  ublas_v1 /= beta;
  vcl_v1   /= gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;



  //
  // add and inplace_add of vectors
  //
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = NumericT(1.0) + random<NumericT>();
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());  //resync
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  std::cout << "Testing add on vector..." << std::endl;

  std::cout << "Checking for successful copy..." << std::endl;
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;
  if (check(ublas_v2, vcl_v2, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1     = ublas_v1 + ublas_v2;
  vcl_v1       =   vcl_v1 +   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing add on vector with flipsign..." << std::endl;
  ublas_v1     = - ublas_v1 + ublas_v2;
  vcl_v1       = -   vcl_v1 +   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace-add on vector..." << std::endl;
  ublas_v1 += ublas_v2;
  vcl_v1   +=   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing assignment to vector with vector multiplied by scalar expression..." << std::endl;
  ublas_v1  = inner_prod(ublas_v1, ublas_v2) * ublas_v2;
  vcl_v1    = viennacl::linalg::inner_prod(vcl_v1, vcl_v2) * vcl_v2;

  //
  // subtract and inplace_subtract of vectors
  //
  std::cout << "Testing sub on vector..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1     = ublas_v1 - ublas_v2;
  vcl_v1       =   vcl_v1 -   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace-sub on vector..." << std::endl;
  ublas_v1 -= ublas_v2;
  vcl_v1   -= vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;



  //
  // multiply-add
  //
  std::cout << "Testing multiply-add on vector with CPU scalar (right)..." << std::endl;
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = NumericT(1.0) + random<NumericT>();
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 + alpha * ublas_v2;
  vcl_v1   = vcl_v1   + alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing multiply-add on vector with CPU scalar (left)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = alpha * ublas_v1 + ublas_v2;
  vcl_v1   = alpha *   vcl_v1 +   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing multiply-add on vector with CPU scalar (both)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = alpha * ublas_v1 + beta * ublas_v2;
  vcl_v1   = alpha *   vcl_v1 + beta *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing inplace multiply-add on vector with CPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 += alpha * ublas_v2;
  vcl_v1   += alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing multiply-add on vector with GPU scalar (right)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 +     alpha * ublas_v2;
  vcl_v1   = vcl_v1   + gpu_alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing multiply-add on vector with GPU scalar (left)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 +     alpha * ublas_v2;
  vcl_v1   = vcl_v1   + gpu_alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing multiply-add on vector with GPU scalar (both)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 =     alpha * ublas_v1 +     beta * ublas_v2;
  vcl_v1   = gpu_alpha *   vcl_v1 + gpu_beta *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing inplace multiply-add on vector with GPU scalar (both, adding)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 +=     alpha * ublas_v1 +     beta * ublas_v2;
  vcl_v1   += gpu_alpha *   vcl_v1 + gpu_beta *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace multiply-add on vector with GPU scalar (both, subtracting)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 +=     alpha * ublas_v1 -     beta * ublas_v2;
  vcl_v1   += gpu_alpha *   vcl_v1 - gpu_beta *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;



  std::cout << "Testing inplace multiply-add on vector with GPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 +=     alpha * ublas_v2;
  vcl_v1   += gpu_alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  //
  // division-add
  //
  std::cout << "Testing division-add on vector with CPU scalar (right)..." << std::endl;
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = NumericT(1.0) + random<NumericT>();
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 + ublas_v2 / alpha;
  vcl_v1   = vcl_v1   + vcl_v2 / alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing division-add on vector with CPU scalar (left)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 / alpha + ublas_v2;
  vcl_v1   =   vcl_v1 / alpha +   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing division-add on vector with CPU scalar (both)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 / alpha + ublas_v2 / beta;
  vcl_v1   =   vcl_v1 / alpha +   vcl_v2 / beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing division-multiply-add on vector with CPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 / alpha + ublas_v2 * beta;
  vcl_v1   =   vcl_v1 / alpha +   vcl_v2 * beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing multiply-division-add on vector with CPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 * alpha + ublas_v2 / beta;
  vcl_v1   =   vcl_v1 * alpha +   vcl_v2 / beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;



  std::cout << "Testing inplace division-add on vector with CPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 += ublas_v2 / alpha;
  vcl_v1   += vcl_v2 / alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing division-add on vector with GPU scalar (right)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 + ublas_v2 / alpha;
  vcl_v1   = vcl_v1   +   vcl_v2 / gpu_alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing division-add on vector with GPU scalar (left)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 + ublas_v2 / alpha;
  vcl_v1   = vcl_v1   +   vcl_v2 / gpu_alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing division-add on vector with GPU scalar (both)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 / alpha     + ublas_v2 / beta;
  vcl_v1   =   vcl_v1 / gpu_alpha +   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing inplace division-add on vector with GPU scalar (both, adding)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 += ublas_v1 / alpha     + ublas_v2 / beta;
  vcl_v1   +=   vcl_v1 / gpu_alpha +   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace division-add on vector with GPU scalar (both, subtracting)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 += ublas_v1 / alpha     - ublas_v2 / beta;
  vcl_v1   +=   vcl_v1 / gpu_alpha -   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace division-multiply-add on vector with GPU scalar (adding)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 += ublas_v1 / alpha     + ublas_v2 * beta;
  vcl_v1   +=   vcl_v1 / gpu_alpha +   vcl_v2 * gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace multiply-division-add on vector with GPU scalar (subtracting)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 += ublas_v1 * alpha     - ublas_v2 / beta;
  vcl_v1   +=   vcl_v1 * gpu_alpha -   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;



  std::cout << "Testing inplace division-add on vector with GPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 += ublas_v2 * alpha;
  vcl_v1   +=   vcl_v2 * gpu_alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;



  //
  // multiply-subtract
  //
  std::cout << "Testing multiply-subtract on vector with CPU scalar (right)..." << std::endl;
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = NumericT(1.0) + random<NumericT>();
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 - alpha * ublas_v2;
  vcl_v1   = vcl_v1   - alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing multiply-subtract on vector with CPU scalar (left)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = alpha * ublas_v1 - ublas_v2;
  vcl_v1   = alpha * vcl_v1   -   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing multiply-subtract on vector with CPU scalar (both)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = alpha * ublas_v1 - beta * ublas_v2;
  vcl_v1   = alpha * vcl_v1   - beta *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing inplace multiply-subtract on vector with CPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -= alpha * ublas_v2;
  vcl_v1   -= alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing multiply-subtract on vector with GPU scalar (right)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 -     alpha * ublas_v2;
  vcl_v1   = vcl_v1   - gpu_alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing multiply-subtract on vector with GPU scalar (left)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 -     alpha * ublas_v2;
  vcl_v1   = vcl_v1   - gpu_alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing multiply-subtract on vector with GPU scalar (both)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 =     alpha * ublas_v1 -     beta * ublas_v2;
  vcl_v1   = gpu_alpha * vcl_v1   - gpu_beta *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace multiply-subtract on vector with GPU scalar (both, adding)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -=     alpha * ublas_v1 +     beta * ublas_v2;
  vcl_v1   -= gpu_alpha * vcl_v1   + gpu_beta *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace multiply-subtract on vector with GPU scalar (both, subtracting)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -=     alpha * ublas_v1 -     beta * ublas_v2;
  vcl_v1   -= gpu_alpha * vcl_v1   - gpu_beta *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing inplace multiply-subtract on vector with GPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -=     alpha * ublas_v2;
  vcl_v1   -= gpu_alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;



  //
  // division-subtract
  //
  std::cout << "Testing division-subtract on vector with CPU scalar (right)..." << std::endl;
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = NumericT(1.0) + random<NumericT>();
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 - ublas_v2 / alpha;
  vcl_v1   = vcl_v1   -   vcl_v2 / alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing division-subtract on vector with CPU scalar (left)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 / alpha - ublas_v2;
  vcl_v1   =   vcl_v1 / alpha -   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing division-subtract on vector with CPU scalar (both)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 / alpha - ublas_v2 / alpha;
  vcl_v1   =   vcl_v1 / alpha -   vcl_v2 / alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing inplace division-subtract on vector with CPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -= ublas_v2 / alpha;
  vcl_v1   -=   vcl_v2 / alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace division-subtract on vector with GPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -= ublas_v2 / alpha;
  vcl_v1   -=   vcl_v2 / gpu_alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing division-subtract on vector with GPU scalar (right)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 - ublas_v2 / alpha;
  vcl_v1   = vcl_v1   -   vcl_v2 / gpu_alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing division-subtract on vector with GPU scalar (left)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 - ublas_v2 / alpha;
  vcl_v1   = vcl_v1   -   vcl_v2 / gpu_alpha;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing division-subtract on vector with GPU scalar (both)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 / alpha     - ublas_v2 / beta;
  vcl_v1   =   vcl_v1 / gpu_alpha -   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace division-subtract on vector with GPU scalar (both, adding)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -= ublas_v1 / alpha     + ublas_v2 / beta;
  vcl_v1   -=   vcl_v1 / gpu_alpha +   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace division-subtract on vector with GPU scalar (both, subtracting)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -= ublas_v1 / alpha     - ublas_v2 / beta;
  vcl_v1   -=   vcl_v1 / gpu_alpha -   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing multiply-division-subtract on vector with GPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 * alpha     - ublas_v2 / beta;
  vcl_v1   =   vcl_v1 * gpu_alpha -   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing division-multiply-subtract on vector with GPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v1 / alpha     - ublas_v2 * beta;
  vcl_v1   =   vcl_v1 / gpu_alpha -   vcl_v2 * gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace multiply-division-subtract on vector with GPU scalar (adding)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -= ublas_v1 * alpha     + ublas_v2 / beta;
  vcl_v1   -=   vcl_v1 * gpu_alpha +   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace division-multiply-subtract on vector with GPU scalar (adding)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -= ublas_v1 / alpha     + ublas_v2 * beta;
  vcl_v1   -=   vcl_v1 / gpu_alpha +   vcl_v2 * gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace multiply-division-subtract on vector with GPU scalar (subtracting)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -= ublas_v1 * alpha     - ublas_v2 / beta;
  vcl_v1   -=   vcl_v1 * gpu_alpha -   vcl_v2 / gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing inplace division-multiply-subtract on vector with GPU scalar (subtracting)..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -= ublas_v1 / alpha     - ublas_v2 * beta;
  vcl_v1   -=   vcl_v1 / gpu_alpha -   vcl_v2 * gpu_beta;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing inplace division-subtract on vector with GPU scalar..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 -=     alpha * ublas_v2;
  vcl_v1   -= gpu_alpha *   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;



  //
  // More complicated expressions (for ensuring the operator overloads work correctly)
  //
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = NumericT(1.0) + random<NumericT>();
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  std::cout << "Testing three vector additions..." << std::endl;
  ublas_v1 = ublas_v2 + ublas_v1 + ublas_v2;
  vcl_v1   =   vcl_v2 +   vcl_v1 +   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  std::cout << "Testing complicated vector expression with CPU scalar..." << std::endl;
  ublas_v1 = beta * (ublas_v1 - alpha * ublas_v2);
  vcl_v1   = beta * (vcl_v1   - alpha * vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing complicated vector expression with GPU scalar..." << std::endl;
  ublas_v1 =     beta * (ublas_v1 -     alpha * ublas_v2);
  vcl_v1   = gpu_beta * (vcl_v1   - gpu_alpha * vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  // --------------------------------------------------------------------------
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  std::cout << "Testing swap..." << std::endl;
  swap(ublas_v1, ublas_v2);
  swap(vcl_v1, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  // --------------------------------------------------------------------------
  for (std::size_t i=0; i<ublas_v1.size(); ++i)
  {
    ublas_v1[i] = NumericT(1.0) + random<NumericT>();
    ublas_v2[i] = NumericT(5.0) + random<NumericT>();
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  std::cout << "Testing unary operator-..." << std::endl;
  ublas_v1 = - ublas_v2;
  vcl_v1   = -   vcl_v2;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing elementwise multiplication..." << std::endl;
  std::cout << " v1 = element_prod(v1, v2);" << std::endl;
  ublas_v1 = ublas::element_prod(ublas_v1, ublas_v2);
  vcl_v1 = viennacl::linalg::element_prod(vcl_v1, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " v1 += element_prod(v1, v2);" << std::endl;
  ublas_v1 += ublas::element_prod(ublas_v1, ublas_v2);
  vcl_v1 += viennacl::linalg::element_prod(vcl_v1, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " v1 -= element_prod(v1, v2);" << std::endl;
  ublas_v1 -= ublas::element_prod(ublas_v1, ublas_v2);
  vcl_v1 -= viennacl::linalg::element_prod(vcl_v1, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ///////
  std::cout << " v1 = element_prod(v1 + v2, v2);" << std::endl;
  ublas_v1 = ublas::element_prod(ublas_v1 + ublas_v2, ublas_v2);
  vcl_v1 = viennacl::linalg::element_prod(vcl_v1 + vcl_v2, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " v1 += element_prod(v1 + v2, v2);" << std::endl;
  ublas_v1 += ublas::element_prod(ublas_v1 + ublas_v2, ublas_v2);
  vcl_v1 += viennacl::linalg::element_prod(vcl_v1 + vcl_v2, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " v1 -= element_prod(v1 + v2, v2);" << std::endl;
  ublas_v1 -= ublas::element_prod(ublas_v1 + ublas_v2, ublas_v2);
  vcl_v1 -= viennacl::linalg::element_prod(vcl_v1 + vcl_v2, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ///////
  std::cout << " v1 = element_prod(v1, v2 + v1);" << std::endl;
  ublas_v1 = ublas::element_prod(ublas_v1, ublas_v2 + ublas_v1);
  vcl_v1 = viennacl::linalg::element_prod(vcl_v1, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " v1 += element_prod(v1, v2 + v1);" << std::endl;
  ublas_v1 += ublas::element_prod(ublas_v1, ublas_v2 + ublas_v1);
  vcl_v1 += viennacl::linalg::element_prod(vcl_v1, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " v1 -= element_prod(v1, v2 + v1);" << std::endl;
  ublas_v1 -= ublas::element_prod(ublas_v1, ublas_v2 + ublas_v1);
  vcl_v1 -= viennacl::linalg::element_prod(vcl_v1, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ///////
  std::cout << " v1 = element_prod(v1 + v2, v2 + v1);" << std::endl;
  ublas_v1 = ublas::element_prod(ublas_v1 + ublas_v2, ublas_v2 + ublas_v1);
  vcl_v1 = viennacl::linalg::element_prod(vcl_v1 + vcl_v2, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " v1 += element_prod(v1 + v2, v2 + v1);" << std::endl;
  ublas_v1 += ublas::element_prod(ublas_v1 + ublas_v2, ublas_v2 + ublas_v1);
  vcl_v1 += viennacl::linalg::element_prod(vcl_v1 + vcl_v2, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " v1 -= element_prod(v1 + v2, v2 + v1);" << std::endl;
  ublas_v1 -= ublas::element_prod(ublas_v1 + ublas_v2, ublas_v2 + ublas_v1);
  vcl_v1 -= viennacl::linalg::element_prod(vcl_v1 + vcl_v2, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing elementwise division..." << std::endl;
  for (std::size_t i=0; i<ublas_v1.size(); ++i)
  {
    ublas_v1[i] = NumericT(1.0) + random<NumericT>();
    ublas_v2[i] = NumericT(5.0) + random<NumericT>();
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas::element_div(ublas_v1, ublas_v2);
  vcl_v1 = viennacl::linalg::element_div(vcl_v1, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1 += ublas::element_div(ublas_v1, ublas_v2);
  vcl_v1 += viennacl::linalg::element_div(vcl_v1, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1 -= ublas::element_div(ublas_v1, ublas_v2);
  vcl_v1 -= viennacl::linalg::element_div(vcl_v1, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ///////
  ublas_v1 = ublas::element_div(ublas_v1 + ublas_v2, ublas_v2);
  vcl_v1 = viennacl::linalg::element_div(vcl_v1 + vcl_v2, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1 += ublas::element_div(ublas_v1 + ublas_v2, ublas_v2);
  vcl_v1 += viennacl::linalg::element_div(vcl_v1 + vcl_v2, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1 -= ublas::element_div(ublas_v1 + ublas_v2, ublas_v2);
  vcl_v1 -= viennacl::linalg::element_div(vcl_v1 + vcl_v2, vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ///////
  ublas_v1 = ublas::element_div(ublas_v1, ublas_v2 + ublas_v1);
  vcl_v1 = viennacl::linalg::element_div(vcl_v1, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1 += ublas::element_div(ublas_v1, ublas_v2 + ublas_v1);
  vcl_v1 += viennacl::linalg::element_div(vcl_v1, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1 -= ublas::element_div(ublas_v1, ublas_v2 + ublas_v1);
  vcl_v1 -= viennacl::linalg::element_div(vcl_v1, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ///////
  ublas_v1 = ublas::element_div(ublas_v1 + ublas_v2, ublas_v2 + ublas_v1);
  vcl_v1 = viennacl::linalg::element_div(vcl_v1 + vcl_v2, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1 += ublas::element_div(ublas_v1 + ublas_v2, ublas_v2 + ublas_v1);
  vcl_v1 += viennacl::linalg::element_div(vcl_v1 + vcl_v2, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ublas_v1 -= ublas::element_div(ublas_v1 + ublas_v2, ublas_v2 + ublas_v1);
  vcl_v1 -= viennacl::linalg::element_div(vcl_v1 + vcl_v2, vcl_v2 + vcl_v1);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing elementwise power function..." << std::endl;
  for (std::size_t i=0; i<ublas_v1.size(); ++i)
  {
    ublas_v1[i] = NumericT(1.0) + random<NumericT>();
    ublas_v2[i] = NumericT(5.0) + random<NumericT>();
  }
  UblasVectorType ublas_v3 = ublas_v1;

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] = std::pow(ublas_v1[i], ublas_v2[i]);
  vcl_v1 = viennacl::linalg::element_pow(vcl_v1, vcl_v2);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 = pow(v1, v2);" << std::endl;
    return EXIT_FAILURE;
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] += std::pow(ublas_v1[i], ublas_v2[i]);
  vcl_v1 += viennacl::linalg::element_pow(vcl_v1, vcl_v2);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 += pow(v1, v2);" << std::endl;
    return EXIT_FAILURE;
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] -= std::pow(ublas_v1[i], ublas_v2[i]);
  vcl_v1 -= viennacl::linalg::element_pow(vcl_v1, vcl_v2);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 -= pow(v1, v2);" << std::endl;
    return EXIT_FAILURE;
  }

  ///////
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] = std::pow(ublas_v1[i] + ublas_v2[i], ublas_v2[i]);
  vcl_v1 = viennacl::linalg::element_pow(vcl_v1 + vcl_v2, vcl_v2);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 = pow(v1 + v2, v2);" << std::endl;
    return EXIT_FAILURE;
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] += std::pow(ublas_v1[i] + ublas_v2[i], ublas_v2[i]);
  vcl_v1 += viennacl::linalg::element_pow(vcl_v1 + vcl_v2, vcl_v2);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 += pow(v1 + v2, v2);" << std::endl;
    return EXIT_FAILURE;
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] -= std::pow(ublas_v1[i] + ublas_v2[i], ublas_v2[i]);
  vcl_v1 -= viennacl::linalg::element_pow(vcl_v1 + vcl_v2, vcl_v2);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 -= pow(v1 + v2, v2);" << std::endl;
    return EXIT_FAILURE;
  }

  ///////
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] = std::pow(ublas_v1[i], ublas_v2[i] + ublas_v1[i]);
  vcl_v1 = viennacl::linalg::element_pow(vcl_v1, vcl_v2 + vcl_v1);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 = pow(v1, v2 + v1);" << std::endl;
    return EXIT_FAILURE;
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] += std::pow(ublas_v1[i], ublas_v2[i] + ublas_v1[i]);
  vcl_v1 += viennacl::linalg::element_pow(vcl_v1, vcl_v2 + vcl_v1);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 += pow(v1, v2 + v1);" << std::endl;
    return EXIT_FAILURE;
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] -= std::pow(ublas_v1[i], ublas_v2[i] + ublas_v1[i]);
  vcl_v1 -= viennacl::linalg::element_pow(vcl_v1, vcl_v2 + vcl_v1);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 -= pow(v1, v2 + v1);" << std::endl;
    return EXIT_FAILURE;
  }

  ///////
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] = std::pow(ublas_v1[i] + ublas_v2[i], ublas_v2[i] + ublas_v1[i]);
  vcl_v1 = viennacl::linalg::element_pow(vcl_v1 + vcl_v2, vcl_v2 + vcl_v1);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 = pow(v1 + v2, v2 + v1);" << std::endl;
    return EXIT_FAILURE;
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] += std::pow(ublas_v1[i] + ublas_v2[i], ublas_v2[i] + ublas_v1[i]);
  vcl_v1 += viennacl::linalg::element_pow(vcl_v1 + vcl_v2, vcl_v2 + vcl_v1);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 += pow(v1 + v2, v2 + v1);" << std::endl;
    return EXIT_FAILURE;
  }

  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  ublas_v3 = ublas_v1;
  for (std::size_t i=0; i<ublas_v3.size(); ++i)
    ublas_v3[i] -= std::pow(ublas_v1[i] + ublas_v2[i], ublas_v2[i] + ublas_v1[i]);
  vcl_v1 -= viennacl::linalg::element_pow(vcl_v1 + vcl_v2, vcl_v2 + vcl_v1);

  if (check(ublas_v3, vcl_v1, epsilon) != EXIT_SUCCESS)
  {
    std::cerr << "** Failure in v1 -= pow(v1 + v2, v2 + v1);" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Testing unary elementwise operations..." << std::endl;
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = random<NumericT>() / NumericT(4);

#define GENERATE_UNARY_OP_TEST(FUNCNAME) \
  ublas_v2 = 3.1415 * ublas_v1; \
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin()); \
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin()); \
  \
  for (std::size_t i=0; i<ublas_v1.size(); ++i) \
    ublas_v1[i] = std::FUNCNAME(ublas_v2[i]); \
  vcl_v1 = viennacl::linalg::element_##FUNCNAME(vcl_v2); \
 \
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS) \
  { \
    std::cout << "Failure at v1 = " << #FUNCNAME << "(v2)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<ublas_v1.size(); ++i) \
    ublas_v1[i] = std::FUNCNAME(ublas_v1[i] + ublas_v2[i]); \
  vcl_v1 = viennacl::linalg::element_##FUNCNAME(vcl_v1 + vcl_v2); \
 \
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS) \
  { \
    std::cout << "Failure at v1 = " << #FUNCNAME << "(v1 + v2)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<ublas_v1.size(); ++i) \
    ublas_v1[i] += std::FUNCNAME(ublas_v1[i]); \
  vcl_v1 += viennacl::linalg::element_##FUNCNAME(vcl_v1); \
 \
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS) \
  { \
    std::cout << "Failure at v1 += " << #FUNCNAME << "(v2)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<ublas_v1.size(); ++i) \
    ublas_v1[i] += std::FUNCNAME(ublas_v1[i] + ublas_v2[i]); \
  vcl_v1 += viennacl::linalg::element_##FUNCNAME(vcl_v1 + vcl_v2); \
 \
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS) \
  { \
    std::cout << "Failure at v1 += " << #FUNCNAME << "(v1 + v2)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<ublas_v1.size(); ++i) \
    ublas_v1[i] -= std::FUNCNAME(ublas_v2[i]); \
  vcl_v1 -= viennacl::linalg::element_##FUNCNAME(vcl_v2); \
 \
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS) \
  { \
    std::cout << "Failure at v1 -= " << #FUNCNAME << "(v2)" << std::endl; \
    return EXIT_FAILURE; \
  } \
 \
  for (std::size_t i=0; i<ublas_v1.size(); ++i) \
    ublas_v1[i] -= std::FUNCNAME(ublas_v1[i] + ublas_v2[i]); \
  vcl_v1 -= viennacl::linalg::element_##FUNCNAME(vcl_v1 + vcl_v2); \
 \
  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS) \
  { \
    std::cout << "Failure at v1 -= " << #FUNCNAME << "(v1 + v2)" << std::endl; \
    return EXIT_FAILURE; \
  } \

  GENERATE_UNARY_OP_TEST(cos);
  GENERATE_UNARY_OP_TEST(cosh);
  for (size_t i=0; i < ublas_v1.size(); ++i)
    ublas_v1(i) = random<NumericT>() / NumericT(4);
  GENERATE_UNARY_OP_TEST(exp);
  GENERATE_UNARY_OP_TEST(floor);
  GENERATE_UNARY_OP_TEST(fabs);
  GENERATE_UNARY_OP_TEST(log);
  GENERATE_UNARY_OP_TEST(log10);
  GENERATE_UNARY_OP_TEST(sin);
  GENERATE_UNARY_OP_TEST(sinh);
  GENERATE_UNARY_OP_TEST(fabs);
  //GENERATE_UNARY_OP_TEST(abs); //OpenCL allows abs on integers only
  GENERATE_UNARY_OP_TEST(sqrt);
  GENERATE_UNARY_OP_TEST(tan);
  GENERATE_UNARY_OP_TEST(tanh);

  // --------------------------------------------------------------------------
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  std::cout << "Testing another complicated vector expression with CPU scalars..." << std::endl;
  ublas_v1 = ublas_v2 / alpha + beta * (ublas_v1 - alpha*ublas_v2);
  vcl_v1   = vcl_v2 / alpha   + beta * (vcl_v1   - alpha*vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << "Testing another complicated vector expression with GPU scalars..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v2 / alpha   +     beta * (ublas_v1 - alpha*ublas_v2);
  vcl_v1   = vcl_v2 / gpu_alpha + gpu_beta * (vcl_v1   - gpu_alpha*vcl_v2);

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  std::cout << "Testing lenghty sum of scaled vectors..." << std::endl;
  ublas_v2 = 3.1415 * ublas_v1;
  viennacl::copy(ublas_v1.begin(), ublas_v1.end(), vcl_v1.begin());
  viennacl::copy(ublas_v2.begin(), ublas_v2.end(), vcl_v2.begin());

  ublas_v1 = ublas_v2 / alpha   +     beta * ublas_v1 - alpha * ublas_v2 + beta * ublas_v1 - alpha * ublas_v1;
  vcl_v1   = vcl_v2 / gpu_alpha + gpu_beta *   vcl_v1 - alpha *   vcl_v2 + beta *   vcl_v1 - alpha *   vcl_v1;

  if (check(ublas_v1, vcl_v1, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  // --------------------------------------------------------------------------
  return retval;
}


template< typename NumericT, typename Epsilon >
int test(Epsilon const& epsilon)
{
  int retval = EXIT_SUCCESS;
  std::size_t size = 24656;

  std::cout << "Running tests for vector of size " << size << std::endl;

  //
  // Set up UBLAS objects
  //
  ublas::vector<NumericT> ublas_full_vec(size);
  ublas::vector<NumericT> ublas_full_vec2(ublas_full_vec.size());

  for (std::size_t i=0; i<ublas_full_vec.size(); ++i)
  {
    ublas_full_vec[i]  = NumericT(1.0) + random<NumericT>();
    ublas_full_vec2[i] = NumericT(1.0) + random<NumericT>();
  }

  ublas::range r1(    ublas_full_vec.size() / 4, 2 * ublas_full_vec.size() / 4);
  ublas::range r2(2 * ublas_full_vec2.size() / 4, 3 * ublas_full_vec2.size() / 4);
  ublas::vector_range< ublas::vector<NumericT> > ublas_range_vec(ublas_full_vec, r1);
  ublas::vector_range< ublas::vector<NumericT> > ublas_range_vec2(ublas_full_vec2, r2);

  ublas::slice s1(    ublas_full_vec.size() / 4, 3, ublas_full_vec.size() / 4);
  ublas::slice s2(2 * ublas_full_vec2.size() / 4, 2, ublas_full_vec2.size() / 4);
  ublas::vector_slice< ublas::vector<NumericT> > ublas_slice_vec(ublas_full_vec, s1);
  ublas::vector_slice< ublas::vector<NumericT> > ublas_slice_vec2(ublas_full_vec2, s2);

  //
  // Set up ViennaCL objects
  //
  viennacl::vector<NumericT> vcl_full_vec(ublas_full_vec.size());
  viennacl::vector<NumericT> vcl_full_vec2(ublas_full_vec2.size());

  viennacl::fast_copy(ublas_full_vec.begin(), ublas_full_vec.end(), vcl_full_vec.begin());
  viennacl::copy(ublas_full_vec2.begin(), ublas_full_vec2.end(), vcl_full_vec2.begin());

  viennacl::range vcl_r1(    vcl_full_vec.size() / 4, 2 * vcl_full_vec.size() / 4);
  viennacl::range vcl_r2(2 * vcl_full_vec2.size() / 4, 3 * vcl_full_vec2.size() / 4);
  viennacl::vector_range< viennacl::vector<NumericT> > vcl_range_vec(vcl_full_vec, vcl_r1);
  viennacl::vector_range< viennacl::vector<NumericT> > vcl_range_vec2(vcl_full_vec2, vcl_r2);

  {
    viennacl::vector<NumericT> vcl_short_vec(vcl_range_vec);
    viennacl::vector<NumericT> vcl_short_vec2 = vcl_range_vec2;

    ublas::vector<NumericT> ublas_short_vec(ublas_range_vec);
    ublas::vector<NumericT> ublas_short_vec2(ublas_range_vec2);

    std::cout << "Testing creation of vectors from range..." << std::endl;
    if (check(ublas_short_vec, vcl_short_vec, epsilon) != EXIT_SUCCESS)
      return EXIT_FAILURE;
    if (check(ublas_short_vec2, vcl_short_vec2, epsilon) != EXIT_SUCCESS)
      return EXIT_FAILURE;
  }

  viennacl::slice vcl_s1(    vcl_full_vec.size() / 4, 3, vcl_full_vec.size() / 4);
  viennacl::slice vcl_s2(2 * vcl_full_vec2.size() / 4, 2, vcl_full_vec2.size() / 4);
  viennacl::vector_slice< viennacl::vector<NumericT> > vcl_slice_vec(vcl_full_vec, vcl_s1);
  viennacl::vector_slice< viennacl::vector<NumericT> > vcl_slice_vec2(vcl_full_vec2, vcl_s2);

  viennacl::vector<NumericT> vcl_short_vec(vcl_slice_vec);
  viennacl::vector<NumericT> vcl_short_vec2 = vcl_slice_vec2;

  ublas::vector<NumericT> ublas_short_vec(ublas_slice_vec);
  ublas::vector<NumericT> ublas_short_vec2(ublas_slice_vec2);

  std::cout << "Testing creation of vectors from slice..." << std::endl;
  if (check(ublas_short_vec, vcl_short_vec, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;
  if (check(ublas_short_vec2, vcl_short_vec2, epsilon) != EXIT_SUCCESS)
    return EXIT_FAILURE;


  //
  // Now start running tests for vectors, ranges and slices:
  //

  std::cout << " ** vcl_v1 = vector, vcl_v2 = vector **" << std::endl;
  retval = test<NumericT>(epsilon,
                          ublas_short_vec, ublas_short_vec2,
                          vcl_short_vec, vcl_short_vec2);
  if (retval != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " ** vcl_v1 = vector, vcl_v2 = range **" << std::endl;
  retval = test<NumericT>(epsilon,
                          ublas_short_vec, ublas_short_vec2,
                          vcl_short_vec, vcl_range_vec2);
  if (retval != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " ** vcl_v1 = vector, vcl_v2 = slice **" << std::endl;
  retval = test<NumericT>(epsilon,
                          ublas_short_vec, ublas_short_vec2,
                          vcl_short_vec, vcl_slice_vec2);
  if (retval != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ///////

  std::cout << " ** vcl_v1 = range, vcl_v2 = vector **" << std::endl;
  retval = test<NumericT>(epsilon,
                          ublas_short_vec, ublas_short_vec2,
                          vcl_range_vec, vcl_short_vec2);
  if (retval != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " ** vcl_v1 = range, vcl_v2 = range **" << std::endl;
  retval = test<NumericT>(epsilon,
                          ublas_short_vec, ublas_short_vec2,
                          vcl_range_vec, vcl_range_vec2);
  if (retval != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " ** vcl_v1 = range, vcl_v2 = slice **" << std::endl;
  retval = test<NumericT>(epsilon,
                          ublas_short_vec, ublas_short_vec2,
                          vcl_range_vec, vcl_slice_vec2);
  if (retval != EXIT_SUCCESS)
    return EXIT_FAILURE;

  ///////

  std::cout << " ** vcl_v1 = slice, vcl_v2 = vector **" << std::endl;
  retval = test<NumericT>(epsilon,
                          ublas_short_vec, ublas_short_vec2,
                          vcl_slice_vec, vcl_short_vec2);
  if (retval != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " ** vcl_v1 = slice, vcl_v2 = range **" << std::endl;
  retval = test<NumericT>(epsilon,
                          ublas_short_vec, ublas_short_vec2,
                          vcl_slice_vec, vcl_range_vec2);
  if (retval != EXIT_SUCCESS)
    return EXIT_FAILURE;

  std::cout << " ** vcl_v1 = slice, vcl_v2 = slice **" << std::endl;
  retval = test<NumericT>(epsilon,
                          ublas_short_vec, ublas_short_vec2,
                          vcl_slice_vec, vcl_slice_vec2);
  if (retval != EXIT_SUCCESS)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}



#ifndef VIENNACL_LINALG_HOST_BASED_COMMON_HPP_
#define VIENNACL_LINALG_HOST_BASED_COMMON_HPP_

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

/** @file viennacl/linalg/host_based/common.hpp
    @brief Common routines for single-threaded or OpenMP-enabled execution on CPU
*/

#include "viennacl/traits/handle.hpp"

namespace viennacl
{
  namespace linalg
  {
    namespace host_based
    {
      namespace detail
      {
        template <typename T, typename VectorType>
        T * extract_raw_pointer(VectorType & vec)
        {
          return reinterpret_cast<T *>(viennacl::traits::ram_handle(vec).get());
        }

        template <typename T, typename VectorType>
        T const * extract_raw_pointer(VectorType const & vec)
        {
          return reinterpret_cast<T const *>(viennacl::traits::ram_handle(vec).get());
        }

        template <typename NumericT>
        class vector_array_wrapper
        {
          public:
            typedef NumericT   value_type;

            vector_array_wrapper(value_type * A,
                                 std::size_t start,
                                 std::size_t inc)
             : A_(A),
               start_(start),
               inc_(inc) {}

            value_type & operator()(std::size_t i)
            {
              return A_[i * inc_ + start_];
            }

          private:
            value_type * A_;
            std::size_t start_;
            std::size_t inc_;
        };


        inline bool is_row_major(viennacl::row_major_tag) { return true; }
        inline bool is_row_major(viennacl::column_major_tag) { return false; }

        template <typename T>
        struct majority_struct_for_orientation
        {
          typedef typename T::ERROR_UNRECOGNIZED_MAJORITY_CATEGORTY_TAG   type;
        };

        template <>
        struct majority_struct_for_orientation<viennacl::row_major_tag>
        {
          typedef viennacl::row_major   type;
        };

        template <>
        struct majority_struct_for_orientation<viennacl::column_major_tag>
        {
          typedef viennacl::column_major type;
        };


        template <typename NumericT, typename MajorityCategory, bool is_transposed>
        class matrix_array_wrapper
        {
            typedef typename majority_struct_for_orientation<MajorityCategory>::type   F;

          public:
            typedef NumericT   value_type;

            matrix_array_wrapper(value_type * A,
                                 std::size_t start1, std::size_t start2,
                                 std::size_t inc1,   std::size_t inc2,
                                 std::size_t internal_size1, std::size_t internal_size2)
             : A_(A),
               start1_(start1), start2_(start2),
               inc1_(inc1), inc2_(inc2),
               internal_size1_(internal_size1), internal_size2_(internal_size2) {}

            value_type & operator()(std::size_t i, std::size_t j)
            {
              return A_[F::mem_index(i * inc1_ + start1_, j * inc2_ + start2_, internal_size1_, internal_size2_)];
            }

          private:
            value_type * A_;
            std::size_t start1_, start2_;
            std::size_t inc1_, inc2_;
            std::size_t internal_size1_, internal_size2_;
        };

        template <typename NumericT, typename MajorityCategory>
        class matrix_array_wrapper<NumericT, MajorityCategory, true>
        {
            typedef typename majority_struct_for_orientation<MajorityCategory>::type   F;

          public:
            typedef NumericT   value_type;

            matrix_array_wrapper(value_type * A,
                                 std::size_t start1, std::size_t start2,
                                 std::size_t inc1,   std::size_t inc2,
                                 std::size_t internal_size1, std::size_t internal_size2)
             : A_(A),
               start1_(start1), start2_(start2),
               inc1_(inc1), inc2_(inc2),
               internal_size1_(internal_size1), internal_size2_(internal_size2) {}

            value_type & operator()(std::size_t i, std::size_t j)
            {
              return A_[F::mem_index(j * inc1_ + start1_, i * inc2_ + start2_, internal_size1_, internal_size2_)];  //swapping row and column indices here
            }

          private:
            value_type * A_;
            std::size_t start1_, start2_;
            std::size_t inc1_, inc2_;
            std::size_t internal_size1_, internal_size2_;
        };

      }

    } //namespace host_based
  } //namespace linalg
} //namespace viennacl


#endif

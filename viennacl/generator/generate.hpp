#ifndef VIENNACL_GENERATOR_GENERATE_HPP
#define VIENNACL_GENERATOR_GENERATE_HPP

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


/** @file viennacl/generator/generate.hpp
    @brief User interface
*/

#include <cstring>
#include <vector>
#include <typeinfo>

#include "viennacl/scheduler/forwards.h"

#include "viennacl/generator/enqueue_statement.hpp"
#include "viennacl/generator/statement_representation.hpp"

#include "viennacl/generator/map_generate_prototype.hpp"

#include "viennacl/generator/generate_saxpy.hpp"
#include "viennacl/generator/generate_scalar_reduction.hpp"
#include "viennacl/generator/generate_vector_reduction.hpp"
#include "viennacl/generator/generate_matrix_product.hpp"

namespace viennacl{

  namespace generator{

    using namespace scheduler;

    enum expression_type_family{
      SCALAR_SAXPY,
      VECTOR_SAXPY,
      MATRIX_SAXPY,
      SCALAR_REDUCE,
      VECTOR_REDUCE,
      MATRIX_PRODUCT
    };

    unsigned int count(typename statement::container_type const & expr, operation_node_type op_type){
      unsigned int res = 0;
      for(typename statement::container_type::const_iterator it = expr.begin() ; it != expr.end() ; ++it)
        res += static_cast<unsigned int>(it->op_type_==op_type);
      return res;
    }

    expression_type_family type_family_of(typename statement::container_type const & expr){
      switch(expr[0].lhs_type_family_){
        case VECTOR_TYPE_FAMILY :
          if(count(expr, OPERATION_BINARY_MAT_VEC_PROD_TYPE))
            return VECTOR_REDUCE;
          else
            return VECTOR_SAXPY;
        case MATRIX_ROW_TYPE_FAMILY :
          if(count(expr, OPERATION_BINARY_MAT_MAT_PROD_TYPE))
            return MATRIX_PRODUCT;
          else
            return MATRIX_SAXPY;
        case MATRIX_COL_TYPE_FAMILY :
          if(count(expr, OPERATION_BINARY_MAT_MAT_PROD_TYPE))
            return MATRIX_PRODUCT;
          else
            return MATRIX_SAXPY;
        case SCALAR_TYPE_FAMILY :
          if(count(expr, OPERATION_BINARY_INNER_PROD_TYPE))
            return SCALAR_REDUCE;
          else
            return SCALAR_SAXPY;
        default:
          throw "not implemented";
      }
    }

    class code_generator{
      private:
        typedef std::pair<expression_type_family, generator::template_base::statements_type> representation_node_type;
        typedef std::vector<representation_node_type> statements_type;

        template<class T>
        static void merge(T & first, T const & second){
          first.insert(first.end(), second.begin(), second.end());
        }

        static void add_kernel_name(int id, std::vector<const char *> & vec_ref){

        }

        template<class T, class StatementsType>
        void enqueue_expression(T const & profile, StatementsType const & statements, unsigned int & kernel_id, viennacl::ocl::program & p, std::list<viennacl::ocl::kernel *> & kernels) const {
          for(std::size_t i = 0 ; i < profile.num_kernels() ; ++i){
            //add kernel name
            char str[10];
            std::sprintf(str,"kernel_%d",kernel_id);
            viennacl::ocl::kernel & k = p.get_kernel(str);
            kernels.push_back(&k);

            unsigned int current_arg = 0;

            //Configure ND Range and enqueue arguments
            profile.configure_range_enqueue_arguments(i, statements, k, current_arg);

            std::set<void *> memory;
            for(typename StatementsType::const_iterator it = statements.begin() ; it != statements.end() ; ++it){
              detail::enqueue_statement(*it, memory, current_arg, k);
            }


            ++kernel_id;
          }
        }

      public:
        code_generator() : vector_saxpy_profile_(1,128,128,true)
                          , matrix_saxpy_profile_(1,16,16,16,16,true)
                          , scalar_reduction_profile_(1, 128, 128, true)
                          , vector_reduction_profile_(1, 1, 256, 32)
                          , matrix_product_profile_(4,16,32,64,4,2,8,true,false,1)
                           {
          statements_.reserve(16);
        }

        bool add_statement(scheduler::statement const & s) {
          expression_type_family expr_type = type_family_of(s.array());
          if(statements_.empty())
            statements_.push_back(std::make_pair(expr_type,template_base::statements_type(1,s)));
          else
            if(statements_.back().first == expr_type)
              statements_.back().second.push_back(s);
            else
              statements_.push_back(std::make_pair(expr_type,template_base::statements_type(1,s)));
          return true;
        }

        void configure_program(viennacl::ocl::program & p, std::list<viennacl::ocl::kernel *> & kernels) const {
          unsigned int kernel_id = 0;
          for(statements_type::const_iterator it = statements_.begin() ; it != statements_.end() ; ++it){
            switch(it->first){
              case VECTOR_SAXPY:
                enqueue_expression(vector_saxpy_profile_, it->second, kernel_id, p, kernels);
                break;
              case MATRIX_SAXPY:
                enqueue_expression(matrix_saxpy_profile_, it->second, kernel_id, p, kernels);
                break;
              case SCALAR_REDUCE:
                enqueue_expression(scalar_reduction_profile_, it->second, kernel_id, p, kernels);
                break;
              case VECTOR_REDUCE:
                enqueue_expression(vector_reduction_profile_, it->second, kernel_id, p, kernels);
                break;
              case MATRIX_PRODUCT:
                enqueue_expression(matrix_product_profile_, it->second, kernel_id, p, kernels);
                break;
              default:
                break;
            }

          }
        }

        void make_program_name(char * program_name) const {
          unsigned int current_arg = 0;
          void* memory[64] = {NULL};
          for(statements_type::const_iterator it = statements_.begin() ; it != statements_.end() ; ++it){
            for(std::vector<scheduler::statement>::const_iterator iit = it->second.begin() ; iit != it->second.end() ; ++iit){
              detail::statement_representation(*iit, memory, current_arg, program_name);
            }
          }
          *program_name='\0';
        }



        std::string make_program_string() const {
          utils::kernel_generation_stream stream;

          //Headers generation
          stream << "#if defined(cl_khr_fp64)\n";
          stream <<  "#  pragma OPENCL EXTENSION cl_khr_fp64: enable\n";
          stream <<  "#elif defined(cl_amd_fp64)\n";
          stream <<  "#  pragma OPENCL EXTENSION cl_amd_fp64: enable\n";
          stream <<  "#endif\n";
          stream << std::endl;

          for(statements_type::const_iterator it = statements_.begin() ; it != statements_.end() ; ++it){
            switch(it->first){
              case VECTOR_SAXPY:
                vector_saxpy(it->second, vector_saxpy_profile_)(stream);
                break;
              case MATRIX_SAXPY:
                matrix_saxpy(it->second, matrix_saxpy_profile_)(stream);
                break;
              case SCALAR_REDUCE:
                scalar_reduction(it->second, scalar_reduction_profile_)(stream);
                break;
              case VECTOR_REDUCE:
                vector_reduction(it->second, vector_reduction_profile_)(stream);
                break;
              case MATRIX_PRODUCT:
                matrix_product(it->second,matrix_product_profile_)(stream);
                break;
              default:
                break;
            }
          }
          return stream.str();
        }

      private:
        statements_type statements_;

        vector_saxpy::profile vector_saxpy_profile_;
        matrix_saxpy::profile matrix_saxpy_profile_;
        scalar_reduction::profile scalar_reduction_profile_;
        vector_reduction::profile vector_reduction_profile_;
        matrix_product::profile matrix_product_profile_;

    };

    void enqueue(viennacl::generator::code_generator const & generator){
      char* program_name = (char*)malloc(256*sizeof(char));
      generator.make_program_name(program_name);
      if(!viennacl::ocl::current_context().has_program(program_name)){
        std::string source_code = generator.make_program_string();
    #ifdef VIENNACL_DEBUG_BUILD
        std::cout << "Building " << program_name << "..." << std::endl;
        std::cout << source_code << std::endl;
    #endif
        viennacl::ocl::current_context().add_program(source_code, program_name);
      }
      viennacl::ocl::program & p = viennacl::ocl::current_context().get_program(program_name);
      std::list<viennacl::ocl::kernel*> kernels;
      generator.configure_program(p, kernels);
      for(std::list<viennacl::ocl::kernel*>::iterator it = kernels.begin() ; it != kernels.end() ; ++it){
        viennacl::ocl::enqueue(**it, (*it)->context().get_queue());
      }
    }

  }
}
#endif
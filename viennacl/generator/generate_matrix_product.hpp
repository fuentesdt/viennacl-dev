#ifndef VIENNACL_GENERATOR_GENERATE_MATRIX_PRODUCT_HPP
#define VIENNACL_GENERATOR_GENERATE_MATRIX_PRODUCT_HPP

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


/** @file viennacl/generator/templates/matrix_product.hpp
 *
 * Kernel template for the vector reduction operation
*/

#include <vector>

#include "viennacl/scheduler/forwards.h"

#include "viennacl/generator/generate_template_base.hpp"
#include "viennacl/generator/mapped_types.hpp"
#include "viennacl/generator/utils.hpp"


#include "viennacl/tools/tools.hpp"

namespace viennacl{

  namespace generator{

    class matrix_product : public template_base{
        typedef template_base base_type;
      public:
        typedef base_type::statements_type statements_type;

        class profile : public template_base::profile{
            friend class matrix_product;
            std::size_t lmem_used(std::size_t scalartype_size) const {
              std::size_t lmem_used = 0;
              if(use_LHS_shared_) lmem_used += (ml_ + 1) * (kl_ + 1) * scalartype_size;
              if(use_RHS_shared_) lmem_used += (kl_ + 1) * (nl_ + 1) * scalartype_size;
              return lmem_used;
            }

          public:
            /** @brief The user constructor */
            profile(unsigned int vectorization, unsigned int ml, unsigned int kl, unsigned int nl
                    , unsigned int ms, unsigned int ks, unsigned int ns
                    , bool use_LHS_shared, bool use_RHS_shared
                    , unsigned int unroll) : template_base::profile(vectorization,1){

              ml_= ml; kl_=kl ; nl_=nl;
              ms_ = ms; ks_=ks; ns_=ns;
              use_LHS_shared_ = use_LHS_shared ; use_RHS_shared_ = use_RHS_shared;
              vectorization_ = vectorization;
              unroll_ = unroll;
            }



            void set_local_sizes(std::size_t & size1, std::size_t & size2, std::size_t kernel_id) const{
              size1 = ml_/ms_;
              size2 = nl_/ns_;
            }

            void configure_range_enqueue_arguments(std::size_t kernel_id, statements_type  const & statements, viennacl::ocl::kernel & k, unsigned int & n_arg)  const {
              //set M, N
              scheduler::statement_node first_node = statements.front().array()[0];
              unsigned int M = utils::call_on_matrix(first_node.lhs_type_, first_node.lhs_, utils::size1_fun());
              unsigned int N = utils::call_on_matrix(first_node.lhs_type_, first_node.lhs_, utils::size2_fun());

              //set ND range
              configure_local_sizes(k, kernel_id);
              k.global_work_size(0, M/ms_);
              k.global_work_size(1, N/ns_);

              //set arguments
              //M,N
              k.arg(n_arg++, cl_uint(M));
              k.arg(n_arg++, cl_uint(N));

              //K
              for(statements_type::const_iterator it = statements.begin() ; it != statements.end() ; ++it){
                scheduler::statement::container_type exprs = it->array();
                for(scheduler::statement::container_type::iterator iit = exprs.begin() ; iit != exprs.end() ; ++iit){
                  if(iit->op_type_==scheduler::OPERATION_BINARY_MAT_MAT_PROD_TYPE){
                    scheduler::statement_node const * current_node = &(*iit);
                    //The LHS of the prod is a matrix
                    if(current_node->lhs_type_family_==scheduler::MATRIX_ROW_TYPE_FAMILY
                       ||current_node->lhs_type_family_==scheduler::MATRIX_COL_TYPE_FAMILY)
                    {
                      k.arg(n_arg++, cl_uint(utils::call_on_matrix(current_node->lhs_type_, current_node->lhs_, utils::size2_fun())));
                    }
                    else{
                      //The LHS of the prod is a matrix expression
                      current_node = &exprs[current_node->lhs_.node_index_];
                      if(current_node->lhs_type_family_==scheduler::MATRIX_ROW_TYPE_FAMILY
                         ||current_node->lhs_type_family_==scheduler::MATRIX_COL_TYPE_FAMILY)
                      {
                        k.arg(n_arg++, cl_uint(utils::call_on_matrix(current_node->lhs_type_, current_node->lhs_, utils::size2_fun())));
                      }
                      else if(current_node->rhs_type_family_==scheduler::MATRIX_ROW_TYPE_FAMILY
                              ||current_node->rhs_type_family_==scheduler::MATRIX_COL_TYPE_FAMILY)
                      {
                        k.arg(n_arg++, cl_uint(utils::call_on_matrix(current_node->lhs_type_, current_node->lhs_, utils::size2_fun())));
                      }
                      else{
                        assert(false && bool("unexpected expression tree"));
                      }
                    }
                    return;
                  }
                }
              }
            }
            static std::string size1() { return "M";  }
            static std::string size2() { return "K"; }
            static std::string size3() { return "N"; }

            void kernel_arguments(statements_type  const & statements, std::string & arguments_string) const{
              arguments_string += detail::generate_value_kernel_argument("unsigned int", "M");
              arguments_string += detail::generate_value_kernel_argument("unsigned int", "N");
              arguments_string += detail::generate_value_kernel_argument("unsigned int", "K");
            }

          private:
            unsigned int ml_;
            unsigned int kl_;
            unsigned int nl_;

            unsigned int ms_;
            unsigned int ks_;
            unsigned int ns_;

            bool use_LHS_shared_;
            bool use_RHS_shared_;

            unsigned int unroll_;
        };

        void transform_block(detail::mapped_matrix const & mat, bool store_shared, bool is_transposed
                                    , unsigned int & large_block_1, unsigned int & large_block_2
                                    , unsigned int & small_block_1, unsigned int & small_block_2) const {
          if(mat.is_row_major()){
            if(is_transposed)
              large_block_1 /= profile_.vectorization_;
            else
              large_block_2/=profile_.vectorization_;
            if(!store_shared){
              if(is_transposed)
                small_block_1/=profile_.vectorization_;
              else
                small_block_2/=profile_.vectorization_;
            }
          }
          else{
            if(is_transposed)
              large_block_2 /= profile_.vectorization_;
            else
              large_block_1/=profile_.vectorization_;
            if(!store_shared){
              if(is_transposed)
                small_block_2/=profile_.vectorization_;
              else
                small_block_1/=profile_.vectorization_;
            }
          }
        }

        std::string helper_variable(utils::kernel_generation_stream & stream
                                    , bool store_in_register
                                    , std::string const & type
                                    , std::string const & name
                                    , std::string const & expr) const {
          if(!store_in_register)
            return expr;
          stream << type << " " << name << " = " << expr << ";" << std::endl;
          return name;
        }

        void init_rhs_global_ptr(utils::kernel_generation_stream &  stream,  detail::mapped_matrix const & mat, std::string const & aligned_scalartype, std::string const & offset, unsigned int ks_rhs,unsigned int ns_rhs, unsigned int nl_rhs) const{
          if(mat.is_row_major())
            for(unsigned int k = 0 ; k < ks_rhs ; ++k){
              stream << "__global " << aligned_scalartype << " * ptr_rhs_" << k << " = " << mat.name() << " + " ;
              if(is_rhs_transposed_)
                stream<< mat.offset(utils::to_string(k) + " + " + offset + " +  get_group_id(1)*" + utils::to_string(nl_rhs),"0");
              else
                stream << mat.offset(utils::to_string(k), offset + " +  get_group_id(1)*" + utils::to_string(nl_rhs));
              stream << ";" << std::endl;
            }
          else
            for(unsigned int n = 0 ; n < ns_rhs ; ++n){
              stream << "__global " << aligned_scalartype << " * ptr_rhs_" << n << " = " << mat.name() << " +  " ;
              if(is_rhs_transposed_)
                stream << mat.offset(offset + " +  get_group_id(1)*" + utils::to_string(nl_rhs), utils::to_string(n));
              else
                stream << mat.offset("0",offset + " +  get_group_id(1)*" + utils::to_string(nl_rhs) + " + " + utils::to_string(n));
              stream << ";" << std::endl;
            }
        }


        void update_rhs_global_ptr(utils::kernel_generation_stream & stream,  detail::mapped_matrix const & mat, std::string const & aligned_scalartype,  unsigned int ks, unsigned int ns_rhs, unsigned int ks_rhs) const {
          if(mat.is_row_major() && !is_rhs_transposed_)
            for(unsigned int k=0 ; k<ks ; ++k)
              stream << "ptr_rhs_" << k << " += " << ks_rhs << "*" << profile::size3() << " - " << ns_rhs << ";" << std::endl;
          else if(is_rhs_transposed_ && !mat.is_row_major())
            for(unsigned int n=0 ; n<ns_rhs ; ++n)
              stream << "ptr_rhs_" << n << " += " << ns_rhs << "*" << profile::size2() << " - " << ks_rhs << ";" << std::endl;
        }


        void init_lhs_global_ptr(utils::kernel_generation_stream & stream,  detail::mapped_matrix const & mat, std::string const & aligned_scalartype,  std::string const & offset, unsigned int ms_lhs, unsigned int ks_lhs, unsigned int ml_lhs) const {
          if(mat.is_row_major()){
            for(unsigned int m=0; m<ms_lhs; ++m){
              std::string ptr_name = "ptr_lhs_" + utils::to_string(m);
              stream << "__global " << aligned_scalartype << " * " << ptr_name << " = " << mat.name() << " + ";
              if(is_lhs_transposed_){
                std::string i = utils::to_string(m);
                std::string j = "get_group_id(0)*" + utils::to_string(ml_lhs) + "+" + offset;
                stream << mat.offset(i,j);
              }
              else{
                std::string i = "get_group_id(0)*" + utils::to_string(ml_lhs) + "+" + offset + "+" + utils::to_string(m);
                stream << mat.offset(i,"0");
              }
              stream << ";" << std::endl;
            }
          }
          else{
            for(unsigned int k=0; k<ks_lhs; ++k){
              std::string ptr_name = "ptr_lhs_" + utils::to_string(k);
              stream << "__global " << aligned_scalartype << " * " << ptr_name << " = " << mat.name() << " + " ;
              if(is_lhs_transposed_){
                std::string j = utils::to_string(k) + "+" + "get_group_id(0)*" + utils::to_string(ml_lhs) + "+" + offset ;
                stream << mat.offset("0",j);
              }
              else{
                std::string i = "get_group_id(0)*" + utils::to_string(ml_lhs) + "+" + offset;
                std::string j = utils::to_string(k);
                stream << mat.offset(i,j);
              }
              stream << ";" << std::endl;
            }
          }
        }

        void update_lhs_global_ptr(utils::kernel_generation_stream & stream,  detail::mapped_matrix const & mat, std::string const & aligned_scalartype,  unsigned int ks, unsigned int ms_lhs, unsigned int ks_lhs) const {
          if(is_lhs_transposed_ && mat.is_row_major())
            for(unsigned int m=0 ; m<ms_lhs ; ++m)
              stream << "ptr_lhs_" << m << " += " << ks << "*" << profile::size2() << " - " <<  ks_lhs << ";" << std::endl;
          else if(!is_lhs_transposed_ && !mat.is_row_major())
            for(unsigned int k=0 ; k<ks_lhs ; ++k)
              stream << "ptr_lhs_" << k << " += " << ks_lhs << "*" << profile::size1() << " - " << ms_lhs << ";" << std::endl;
        }



        void fetch_to_local_mem(utils::kernel_generation_stream & stream,
                                std::string const & lmem_name,
                                std::size_t lmem_size2,
                                std::string const & offset,
                                unsigned int bound1,
                                unsigned int bound2,
                                detail::mapped_matrix const & mat,
                                std::string const & aligned_scalartype) const {
          stream << "barrier(CLK_LOCAL_MEM_FENCE);" << std::endl;
          stream << "for(unsigned int i = get_local_id(0)" << " ; i < " << bound1 << "; i+= get_local_size(0)){" << std::endl;
          stream.inc_tab();
          stream << "for(unsigned int j = get_local_id(1)" << " ; j < " << bound2 << "; j+= get_local_size(1)){" << std::endl;
          stream.inc_tab();
          if(mat.is_row_major()){
            std::string i = offset + " + j  + " + mat.size2() + "*i";
            stream << aligned_scalartype << " val = " << mat.generate(i) << ";" << std::endl;
            stream << "__local " << mat.scalartype() << "* ptr = " << lmem_name << " + i*" << lmem_size2 << "+j*" << profile_.vectorization_<<";" <<std::endl;
            for(unsigned int a = 0 ; a < profile_.vectorization_ ; ++a){
              if(profile_.vectorization_>1)
                stream << "*ptr++ =  val.s" << a << ";" << std::endl;
              else
                stream << "*ptr++ =  val;" << std::endl;
            }
          }
          else{
            std::string i = offset + "+ j*" + mat.size1() + " + i";
            stream << aligned_scalartype << " val = " << mat.generate(i) << ";" << std::endl;
            stream << "__local " << mat.scalartype() << "* ptr = " << lmem_name << " + i*" << profile_.vectorization_ * lmem_size2 << "+ j;" <<std::endl;
            for(unsigned int a = 0 ; a < profile_.vectorization_ ; ++a){
              if(profile_.vectorization_>1)
                stream << "*ptr =  val.s" << a << ";" << std::endl;
              else
                stream << "*ptr =  val;" << std::endl;
              stream << "ptr += " << lmem_size2 << ";" << std::endl;
            }
          }

          stream.dec_tab();
          stream << "}" << std::endl;
          stream.dec_tab();
          stream << "}" << std::endl;
          stream << "barrier(CLK_LOCAL_MEM_FENCE);" << std::endl;
        }



      public:
        matrix_product(template_base::statements_type const & s, profile const & p) : template_base(s, profile_), profile_(p){
          is_lhs_transposed_ = false;
          is_rhs_transposed_ = false;
        }

        void core(std::size_t idx, utils::kernel_generation_stream& stream) const{

          bool use_LHS_shared = profile_.use_LHS_shared_;
          bool use_RHS_shared = profile_.use_RHS_shared_;
          unsigned int vectorization = profile_.vectorization_;
          unsigned int kl = profile_.kl_;
          unsigned int ks = profile_.ks_;
          unsigned int ml = profile_.ml_;
          unsigned int ms = profile_.ms_;
          unsigned int nl = profile_.nl_;
          unsigned int ns = profile_.ns_;
          unsigned int unroll = profile_.unroll_;

          detail::mapped_matrix const * assigned = static_cast<detail::mapped_matrix const *>(mapping_[0].at(std::make_pair(0,detail::LHS_NODE_TYPE)).get());
          std::vector<detail::mapped_matrix_product*> exprs;
          for(std::vector<detail::mapping_type>::iterator it = mapping_.begin() ; it != mapping_.end() ; ++it)
            for(detail::mapping_type::iterator iit = it->begin() ; iit != it->end() ; ++iit)
              if(detail::mapped_matrix_product * p = dynamic_cast<detail::mapped_matrix_product*>(iit->second.get()))
                exprs.push_back(p);
          detail::mapped_matrix const * lhs = static_cast<detail::mapped_matrix const *>(exprs.front()->lhs().mapping_->at(exprs.front()->lhs().index_).get());
          detail::mapped_matrix const * rhs = static_cast<detail::mapped_matrix const *>(exprs.front()->rhs().mapping_->at(exprs.front()->rhs().index_).get());

          assigned->bind_sizes("M", "N");
          lhs->bind_sizes("M", "K");
          rhs->bind_sizes("K", "N");

          unsigned int ml_res = ml, nl_res = nl, ms_res = ms, ns_res = ns;
          unsigned int ml_lhs = ml, kl_lhs = kl, ms_lhs = ms, ks_lhs = ks;
          unsigned int kl_rhs = kl, nl_rhs = nl, ks_rhs = ks, ns_rhs = ns;

          transform_block(*assigned,false,false,ml_res,nl_res,ms_res,ns_res);
          transform_block(*lhs,is_lhs_transposed_,use_LHS_shared,ml_lhs,kl_lhs,ms_lhs,ks_lhs);
          transform_block(*rhs,is_rhs_transposed_,use_RHS_shared,kl_rhs,nl_rhs,ks_rhs,ns_rhs);

          unsigned int lhs_local_size1 = ml, lhs_local_size2 = kl+1;
          unsigned int rhs_local_size1 = kl, rhs_local_size2 = nl+1;
          if(is_lhs_transposed_)
            std::swap(lhs_local_size1, lhs_local_size2);
          if(is_rhs_transposed_)
            std::swap(rhs_local_size1, rhs_local_size2);

          std::string aligned_scalartype = assigned->scalartype();
          if(profile_.vectorization_ > 1) aligned_scalartype+=utils::to_string(profile_.vectorization_);

          std::string lhs_value_scalartype;
          if(use_LHS_shared)
            lhs_value_scalartype = lhs->scalartype();
          else
            lhs_value_scalartype = aligned_scalartype;

          std::string rhs_value_scalartype;
          if(use_RHS_shared)
            rhs_value_scalartype = rhs->scalartype();
          else
            rhs_value_scalartype = aligned_scalartype;

          //Declaration of results registers
          //std::string res_table_name(first_prod->repr() + "_res");
          for(unsigned int m=0; m< ms_res; ++m)
            for(unsigned int n=0; n < ns_res ; ++n)
              stream << aligned_scalartype << " " << "val_res" << m << n << " = (" << aligned_scalartype << ")(0) ;" << std::endl;

          //Declaration of local memories
          if(use_LHS_shared)
            stream << "__local " << lhs->scalartype() <<" local_lhs" << "[" << lhs_local_size1*(lhs_local_size2) << "];" << std::endl;

          if(use_RHS_shared)
            stream << "__local " << lhs->scalartype() <<" local_rhs" << "[" << rhs_local_size1*(rhs_local_size2) << "];" << std::endl;

          //Declaration of helpers
          std::string offset_m = helper_variable(stream,false,"unsigned int", "offset_m", "get_local_id(0)*" + utils::to_string(ms_lhs));
          std::string offset_n = helper_variable(stream,false,"unsigned int", "offset_n", "get_local_id(1)*" + utils::to_string(ns_rhs));
          std::string n_blocks;
          if(is_lhs_transposed_)
            n_blocks = "M/" + utils::to_string(kl_lhs);
          else
            n_blocks = "K/" + utils::to_string(kl_lhs);

          std::string block_num = helper_variable(stream,true,"unsigned int", "block_num",n_blocks);

          //Declaration of pointers and/or offsets to result, rhs, lhs.
          stream << "__global " << aligned_scalartype << "* res_ptr = ";
          stream <<  assigned->name() << " + " << assigned->offset("get_global_id(0)*" + utils::to_string(ms_res), "get_global_id(1)*" + utils::to_string(ns_res)) << ";" << std::endl;

          if(use_RHS_shared){
            if(is_rhs_transposed_)
              stream << "unsigned int offsetRHS = " << rhs->offset(" get_group_id(1)*" + utils::to_string(nl_rhs),"0") << ";" << std::endl;
            else
              stream << "unsigned int offsetRHS = " << rhs->offset("0", " get_group_id(1)*" + utils::to_string(nl_rhs)) << ";" << std::endl;
          }
          else{
            if(is_rhs_transposed_)
              init_rhs_global_ptr(stream,*rhs,aligned_scalartype,offset_n, ns_rhs, ks_rhs, nl_rhs);
            else
              init_rhs_global_ptr(stream,*rhs,aligned_scalartype,offset_n, ks_rhs, ns_rhs, nl_rhs);
          }

          if(use_LHS_shared){
            if(is_lhs_transposed_)
              stream << "unsigned int offsetLHS = " << lhs->offset("0", "get_group_id(0)*" + utils::to_string(ml_lhs)) << ";" << std::endl;
            else
              stream << "unsigned int offsetLHS = " << lhs->offset("get_group_id(0)*" + utils::to_string(ml_lhs), "0") << ";" << std::endl;
          }
          else{
            if(is_lhs_transposed_)
              init_lhs_global_ptr(stream, *lhs, aligned_scalartype, offset_m, ks_lhs, ms_lhs, ml_lhs);
            else
              init_lhs_global_ptr(stream, *lhs, aligned_scalartype, offset_m, ms_lhs, ks_lhs, ml_lhs);
          }


          //Main loop
          stream << "for(unsigned int bl=0 ; bl<" << block_num << " ; ++bl){" << std::endl;
          stream.inc_tab();

          //Fetches to local memory if necessary and declares pointers to local memory
          if(use_LHS_shared){
            if(is_lhs_transposed_)
              fetch_to_local_mem(stream,"local_lhs", lhs_local_size2, "offsetLHS",kl_lhs,ml_lhs,*lhs,aligned_scalartype);
            else
              fetch_to_local_mem(stream,"local_lhs", lhs_local_size2, "offsetLHS",ml_lhs,kl_lhs,*lhs,aligned_scalartype);
            unsigned int upper_bound = is_lhs_transposed_?ks_lhs:ms_lhs;
            for(unsigned int m=0; m<upper_bound; ++m){
              stream << "__local " << lhs_value_scalartype << "* ptr_lhs_" << m << " = local_lhs + " ;
              if(is_lhs_transposed_)
                stream << m*lhs_local_size2 << " + " << offset_m ;
              else
                stream << "(" << offset_m << "+" << m << ")" << "*" << lhs_local_size2;
              stream << ";" << std::endl;
            }
          }

          if(use_RHS_shared){
            if(is_rhs_transposed_)
              fetch_to_local_mem(stream,"local_rhs", rhs_local_size2, "offsetRHS",nl_rhs,kl_rhs,*rhs,aligned_scalartype);
            else
              fetch_to_local_mem(stream,"local_rhs", rhs_local_size2, "offsetRHS",kl_rhs,nl_rhs,*rhs,aligned_scalartype);
            unsigned int upper_bound = is_rhs_transposed_?ns_rhs:ks_rhs;
            for(unsigned int k=0; k<upper_bound; ++k){
              stream << "__local " << rhs_value_scalartype << "* ptr_rhs_" << k << " = local_rhs + " ;
              if(is_rhs_transposed_)
                stream << "(" << offset_n << "+" << k << ")*" << rhs_local_size2;
              else
                stream << k*rhs_local_size2 << " + " << offset_n;
              stream << ";" << std::endl;
            }
          }

          if(unroll > 1)
            stream << "#pragma unroll " << unroll << std::endl;
          stream << " for(unsigned int bs=0 ; bs < " << kl/ks  << " ; ++bs){" << std::endl;
          stream.inc_tab();


          unsigned int upperbound_1_rhs = is_rhs_transposed_?ns_rhs:ks_rhs;
          unsigned int upperbound_2_rhs = is_rhs_transposed_?ks_rhs:ns_rhs;
          for(unsigned int k = 0 ; k < upperbound_1_rhs ; ++k){
            for(unsigned int n=0 ; n < upperbound_2_rhs ; ++n){
              stream << rhs_value_scalartype << " val_rhs_" << k << "_" << n << " = " ;
              if(use_RHS_shared )
                 stream << "* ptr_rhs_" << k << "++";
              else{
                if(rhs->is_row_major())
                  stream << "* ptr_rhs_" << k << "++";
                else
                  stream  << "* ptr_rhs_" << n << "++";
              }
              stream << ";" << std::endl;
            }
          }



          unsigned int upperbound_1_lhs = is_lhs_transposed_?ms_lhs:ks_lhs;
          unsigned int upperbound_2_lhs = is_lhs_transposed_?ks_lhs:ms_lhs;
          for(unsigned int k = 0 ; k < upperbound_1_lhs ; ++k){
            for(unsigned int m=0 ; m < upperbound_2_lhs ; ++m){
              stream << lhs_value_scalartype << " " << "val_lhs_" << m << "_" << k << " = ";
              if(use_LHS_shared)
                stream <<  "* ptr_lhs_" << m << "++" ;
              else if(lhs->is_row_major())
                stream << "* ptr_lhs_" << m << "++";
              else
                stream << "* ptr_lhs_" << k << "++";
              stream << ";" << std::endl;
            }
          }



          for(unsigned int k = 0 ; k < ks ; ++k){
            for(unsigned int n=0 ; n < ns_res ; ++n){
              for(unsigned int m=0 ; m < ms_res ; ++m){
                for(unsigned int a = 0; a<vectorization; ++a){

                  int ind_lhs_1 = m;
                  int ind_lhs_2 = k;
                  int ind_s_lhs=a;

                  int ind_rhs_1=k;
                  int ind_rhs_2=n;
                  int ind_s_rhs=a;

                  bool is_vectorized_lhs = false;
                  bool is_vectorized_rhs = false;

                  if(assigned->is_row_major()){
                    if(is_lhs_transposed_) std::swap(ind_lhs_1,ind_lhs_2);

                    if(!use_LHS_shared){
                      if(lhs->is_row_major()){
                        ind_s_lhs = ind_lhs_2%vectorization;
                        ind_lhs_2 /= vectorization;
                      }
                      else{
                        ind_s_lhs = ind_lhs_1%vectorization;
                        ind_lhs_1 /= vectorization;
                      }
                    }
                  }
                  else{
                    if(use_LHS_shared){
                      ind_lhs_1 = ind_lhs_1*vectorization+a;
                    }
                    else{
                      if((lhs->is_row_major() && !is_lhs_transposed_)
                         ||(!lhs->is_row_major() && is_lhs_transposed_)){
                        ind_lhs_1 = ind_lhs_1*vectorization+a;
                        ind_s_lhs = ind_lhs_2%vectorization;
                        ind_lhs_2 /= vectorization;

                      }
                    }
                    if(is_lhs_transposed_) std::swap(ind_lhs_1,ind_lhs_2);
                  }

                  if(assigned->is_row_major()){
                    if(use_RHS_shared){
                      ind_rhs_2 = ind_rhs_2*vectorization+a;
                    }
                    else{
                      if((!rhs->is_row_major() && !is_rhs_transposed_)
                         ||(rhs->is_row_major() && is_rhs_transposed_)){
                        ind_rhs_2 = ind_rhs_2*vectorization+a;
                        ind_s_rhs = ind_rhs_1%vectorization;
                        ind_rhs_1 = ind_rhs_1/vectorization;
                      }
                      else if( (rhs->is_row_major() && !is_rhs_transposed_) ){
                        is_vectorized_rhs=true;
                      }
                    }
                    if(is_rhs_transposed_) std::swap(ind_rhs_1,ind_rhs_2);
                  }
                  else{
                    if(is_rhs_transposed_) std::swap(ind_rhs_1,ind_rhs_2);
                    if(!use_RHS_shared){
                      if(rhs->is_row_major()){
                        ind_s_rhs = ind_rhs_2%vectorization;
                        ind_rhs_2/=vectorization;
                      }
                      else{
                        ind_s_rhs = ind_rhs_1%vectorization;
                        ind_rhs_1/=vectorization;
                      }
                    }
                  }

                  bool is_vectorized = is_vectorized_lhs || is_vectorized_rhs;

                  std::ostringstream res_oss;
                  std::ostringstream lhs_oss;
                  std::ostringstream rhs_oss;

                  stream << "val_res" << m << n << " += ";
                  if(!is_vectorized && vectorization>1) res_oss << ".s" << a;

                  stream << "val_lhs_" << ind_lhs_1 << "_" << ind_lhs_2;
                  if(!is_vectorized_lhs && !use_LHS_shared && vectorization>1) lhs_oss << ".s" << ind_s_lhs;

                  stream << "*";

                  stream << "val_rhs_" << ind_rhs_1 << "_" << ind_rhs_2;
                  if(!is_vectorized_rhs && !use_RHS_shared && vectorization>1) rhs_oss << ".s" << ind_s_rhs;

                  stream << ";" << std::endl;


                  if(is_vectorized)
                    break;
                }
              }
            }
          }


          if(use_RHS_shared){
            for(unsigned int k=0 ; k<ks ; ++k)
              if(!is_rhs_transposed_) stream << "ptr_rhs_" << k << " += " << ks_rhs*rhs_local_size2 - ns_rhs << ";" << std::endl;
          }
          else{
            if(is_rhs_transposed_)
              update_rhs_global_ptr(stream, *rhs, aligned_scalartype, ks, ks_rhs, ns_rhs);
            else
              update_rhs_global_ptr(stream, *rhs, aligned_scalartype, ks, ns_rhs, ks_rhs);
          }



          if(use_LHS_shared){
            for(unsigned int m=0 ; m<ks_lhs ; ++m)
              if(is_lhs_transposed_) stream << "ptr_lhs_" << m << " += " << ks*lhs_local_size2 - ms_lhs << ";" << std::endl;
          }
          else{
            if(is_lhs_transposed_)
              update_lhs_global_ptr(stream, *lhs, aligned_scalartype,ks,ks_lhs,ms_lhs);
            else
              update_lhs_global_ptr(stream, *lhs, aligned_scalartype,ks,ms_lhs,ks_lhs);
          }



          stream.dec_tab();
          stream << "}" << std::endl;

          if(use_LHS_shared){
            if(is_lhs_transposed_){
              if(lhs->is_row_major())
                stream << "offsetLHS += " << kl_lhs << "*" << lhs->size2() << ";" << std::endl;
              else
                stream << "offsetLHS += " << kl_lhs  << ";" << std::endl;
            }
            else{
              if(lhs->is_row_major())
                stream << "offsetLHS += " << kl_lhs << ";" << std::endl;
              else
                stream << "offsetLHS += " << kl_lhs << "*" << lhs->size1() << ";" << std::endl;
            }

          }

          if(use_RHS_shared){
            if(is_rhs_transposed_){
              if(rhs->is_row_major())
                stream << "offsetRHS += " << kl_rhs << ";" << std::endl;
              else
                stream << "offsetRHS += " << kl_rhs << "*" << rhs->size1() << ";" << std::endl;
            }
            else{
              if(rhs->is_row_major())
                stream << "offsetRHS += " << kl_rhs << "*" << rhs->size2() << ";" << std::endl;
              else
                stream << "offsetRHS += " << kl_rhs << ";" << std::endl;
            }
          }


          stream.dec_tab();
          stream << "}" << std::endl;

          if(assigned->is_row_major()){
            for(unsigned int m=0 ; m < ms_res ; ++m){
              for(unsigned int n=0 ; n < ns_res ; ++n){
                stream << "*res_ptr++ = val_res" << m << n << ";" << std::endl;
              }
              if(m<ms_res-1)  stream << "res_ptr+=" << assigned->size2() << " - " << ns_res << ";" << std::endl;
            }
          }
          else{
            for(unsigned int n=0 ; n < ns_res ; ++n){
              for(unsigned int m=0 ; m < ms_res ; ++m){
                stream << "*res_ptr++ = val_res" << m << n << ";" << std::endl;
              }
              if(n<ns_res-1) stream << "res_ptr+=" << assigned->size1() << " - " << ms_res << ";" << std::endl;
            }
          }


        }

      private:
        profile profile_;

        bool is_lhs_transposed_;
        bool is_rhs_transposed_;
    };

  }

}

#endif
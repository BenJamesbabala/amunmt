#pragma once

#include "mblas.h"

template <class Backend, class Weights>
class SlowGRU {
  typedef typename Backend::Matrix Matrix;
  
  public:
    SlowGRU(const Weights& model)
    : w_(model) {}
          
    void GetNextState(Matrix& NextState,
                      const Matrix& State,
                      const Matrix& Context) const {
      
      using namespace mblas;
      
      const size_t cols = GetStateLength();
      
      // @TODO: Optimization
      // @TODO: Launch streams to perform GEMMs in parallel
      // @TODO: Join matrices and perform single GEMM --------
      Prod(RU_, Context, w_.W_);
      Prod(H_,  Context, w_.Wx_);
      // -----------------------------------------------------
      
      // @TODO: Join matrices and perform single GEMM --------
      Prod(Temp1_, State, w_.U_);
      Prod(Temp2_, State, w_.Ux_);        
      // -----------------------------------------------------
      
      // @TODO: Organize into one kernel ---------------------
      BroadcastVec(_1 + _2, RU_, w_.B_); // Broadcasting row-wise
      Element(Logit(_1 + _2), RU_, Temp1_);
      Slice(R_, RU_, 0, cols);
      Slice(U_, RU_, 1, cols);
      
      BroadcastVec(_1 + _2, H_,    w_.Bx1_); // Broadcasting row-wise
      BroadcastVec(_1 + _2, Temp2_, w_.Bx2_); // Broadcasting row-wise
      
      Element(Tanh(_1 + _2 * _3), H_, R_, Temp2_);
      Element((1.0 - _1) * _2 + _1 * _3, U_, H_, State);
      // -----------------------------------------------------
      
      Swap(NextState, U_);
    }
    
    size_t GetStateLength() const {
      return w_.U_.Rows();
    }
    
  private:
    // Model matrices
    const Weights& w_;
    
    // reused to avoid allocation
    mutable Matrix RU_;
    mutable Matrix R_;
    mutable Matrix U_;
    mutable Matrix H_;
    mutable Matrix Temp1_;
    mutable Matrix Temp2_;
};

__global__ void gElementwiseOps(float* out,
                                const float* state,
                                const float* ru,
                                const float* h,
                                const float* t1,
                                const float* t2,
                                const float* b,
                                const float* bx1,
                                const float* bx2,
                                size_t rows, size_t cols);

template <class Backend, class Weights>
class FastGRU {
  
    typedef typename Backend::Matrix Matrix;
  
  public:
    FastGRU(const Weights& model)
    : w_(model) {
      /*for(int i = 0; i < 4; ++i) {
        cudaStreamCreate(&s_[i]);
        cublasCreate(&h_[i]);
        cublasSetStream(h_[i], s_[i]);            
      }*/
    }
          
    void GetNextState(Matrix& NextState,
                      const Matrix& State,
                      const Matrix& Context) const {
      using namespace mblas;
      
      const size_t cols = GetStateLength();
      
      // @TODO: Optimization
      // @TODO: Launch streams to perform GEMMs in parallel
      // @TODO: Join matrices and perform single GEMM --------
      Prod(/*h_[0],*/ RU_, Context, w_.W_);
      Prod(/*h_[1],*/ H_,  Context, w_.Wx_);
      // -----------------------------------------------------
      
      // @TODO: Join matrices and perform single GEMM --------
      Prod(/*h_[2],*/ Temp1_, State, w_.U_);
      Prod(/*h_[3],*/ Temp2_, State, w_.Ux_);        
      // -----------------------------------------------------
      //cudaDeviceSynchronize();
      
      ElementwiseOps(NextState, State, RU_, H_, Temp1_, Temp2_);
    }
        
    void ElementwiseOps(Matrix& NextState,
                        const Matrix& State,
                        const Matrix& RU,
                        const Matrix& H,
                        const Matrix& Temp1,
                        const Matrix& Temp2) const {
      const size_t rows = State.Rows();
      const size_t cols = State.Cols();
      NextState.Resize(rows, cols);
      
      int blocks  = std::min(MAX_BLOCKS, (int)rows);
      int threads = std::min(MAX_THREADS, (int)cols);
      gElementwiseOps<<<blocks, threads>>>(NextState.data(), State.data(),
                                          RU.data(), H.data(),
                                          Temp1.data(), Temp2.data(),
                                          w_.B_.data(), w_.Bx1_.data(), w_.Bx2_.data(),
                                          rows, cols);
      cudaStreamSynchronize(0);
    }
    
    size_t GetStateLength() const {
      return w_.U_.Rows();
    }

    
  private:
    // Model matrices
    const Weights& w_;
    
    cublasHandle_t h_[4];
    cudaStream_t s_[4];
        
    // reused to avoid allocation
    mutable Matrix RU_;
    mutable Matrix H_;
    mutable Matrix Temp1_;
    mutable Matrix Temp2_;
};

template<class T>
using GRU = FastGRU<T>;
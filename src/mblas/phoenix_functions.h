#pragma once

#include <cmath>
#include <boost/phoenix/phoenix.hpp>

#include "simd_math_prims.h"

namespace mblas
{
  template <class T>
  auto Exp(const T& x) -> decltype(boost::phoenix::bind(expapprox, x))
  {
    return boost::phoenix::bind(expapprox, x);  
  }
  
  template <typename T>
  auto Tanh(const T& x) -> decltype(boost::phoenix::bind(tanhapprox, x)) {
    return boost::phoenix::bind(tanhapprox, x);  
  }
  
  template <typename T>
  auto Log(const T& x) -> decltype(boost::phoenix::bind(logapprox, x)) {
    return boost::phoenix::bind(logapprox, x);  
  }
  
  float logit(float x);
  
  float max(float x, float y);
  
  template <typename T>
  auto Logit(const T& x) -> decltype(boost::phoenix::bind(logit, x)) { 
    return boost::phoenix::bind(logit, x);  
  }
  
  template <typename T1, typename T2>
  auto Max(const T1& x, const T2& y) -> decltype(boost::phoenix::bind(max, x, y)) { 
    return boost::phoenix::bind(max, x, y);  
  }
}
#pragma once

#include <stdio.h>

#include "useful_utils.cpp"

struct Vec2F64
{
   f64 elems[2];

   Vec2F64(f64 x1, f64 x2)
   {
      elems[0] = x1;
      elems[1] = x2;
   }
};

struct Mat2x2F64
{
   f64 elems[4];

   Mat2x2F64(f64 a11, f64 a21, f64 a12, f64 a22)
   {
      elems[0] = a11;
      elems[1] = a21;
      elems[2] = a12;
      elems[3] = a22;
   }
};

static inline
Vec2F64 matvecmul(Mat2x2F64 A, Vec2F64 x)
{
   f64 b1 = A.elems[0] * x.elems[0] + A.elems[2] * x.elems[1];
   f64 b2 = A.elems[1] * x.elems[0] + A.elems[3] * x.elems[1];
   return Vec2F64(b1, b2);
}

static inline
void printVec2F64(Vec2F64 x)
{
   printf("[%f, %f]", x.elems[0], x.elems[1]);
}

static inline
void printlnVec2F64(Vec2F64 x)
{
   printVec2F64(x);
   printf("\n");
}

static inline
bool isapprox(Vec2F64 a, Vec2F64 b)
{
   return isapprox(a.elems[0], b.elems[0]) && isapprox(a.elems[1], b.elems[1]);
}

struct Polynomial
{
#define maxnumcoeffs 20
   f64 coeffs[maxnumcoeffs];
};

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
void printMat2x2F64(Mat2x2F64 A)
{
   printf("[ %f\t%f\n", A.elems[0], A.elems[2]);
   printf("  %f\t%f ]\n", A.elems[1], A.elems[3]);
}

static inline
Vec2F64 matvecmul(Mat2x2F64 A, Vec2F64 x)
{
   f64 b1 = A.elems[0] * x.elems[0] + A.elems[2] * x.elems[1];
   f64 b2 = A.elems[1] * x.elems[0] + A.elems[3] * x.elems[1];
   return Vec2F64(b1, b2);
}

static inline
Mat2x2F64 matmul(Mat2x2F64 A, Mat2x2F64 B)
{
   f64 c11 = A.elems[0] * B.elems[0] + A.elems[2] * B.elems[1];
   f64 c21 = A.elems[1] * B.elems[0] + A.elems[3] * B.elems[1];
   f64 c12 = A.elems[0] * B.elems[2] + A.elems[2] * B.elems[3];
   f64 c22 = A.elems[1] * B.elems[2] + A.elems[3] * B.elems[3];
   return Mat2x2F64(c11, c21, c12, c22);
}

static inline
bool isapprox(Mat2x2F64 A, Mat2x2F64 B)
{
   for (int i = 0; i < 4; i += 1)
   {
      if (!isapprox(A.elems[i], B.elems[i]))
         return false;
   }
   return true;
}

static inline
Mat2x2F64 operator+(Mat2x2F64 A, Mat2x2F64 B)
{
   f64 c11 = A.elems[0] + B.elems[0];
   f64 c21 = A.elems[1] + B.elems[1];
   f64 c12 = A.elems[2] + B.elems[2];
   f64 c22 = A.elems[3] + B.elems[3];
   return Mat2x2F64(c11, c21, c12, c22);
}

static inline
Mat2x2F64 operator*(f64 t, Mat2x2F64 A)
{
   return Mat2x2F64(t * A.elems[0], t * A.elems[1], t * A.elems[2], t * A.elems[3]);
}

static inline
Mat2x2F64 expm(Mat2x2F64 A)
{
   Mat2x2F64 result(1, 0, 0, 1);
   Mat2x2F64 An(1, 0, 0, 1);
   f64 factorial = 1;
   for (int i = 1; i < 20; i += 1)
   {
      An = matmul(An, A);
      factorial *= i;
      result = result + (1/factorial) * An;
   }
   return result;
}

/*
#define maxnumcoeffs 20
struct Polynomial
{
   f64 coeffs[maxnumcoeffs];
   u8 degree;
};

Polynomial newPolynomial(f64 *coeffs, u8 degree)
{
   assert(degree < maxnumcoeffs);
   Polynomial p;
   p.degree = degree;
   for (int n = 0; n <= p.degree; n += 1)
   {
      p.coeffs[n] = coeffs[n];
   }
   return p;
}

f64 eval(Polynomial p, f64 x)
{
   f64 result = 0;
   for (int n = 0; n <= p.degree; n += 1)
   {
      result += p.coeffs[n] * pow(x, n);
   }
   return result;
}

u64 factorial(u32 n)
{
   u64 result = 1;
   for (u32 i = 1; i <= n; i += 1)
      result *= i;
   return result;
}

// Padé approximant to the exponential function
// DOI. 10.1137/090768539

Polynomial p_km(u8 k, u8 m)
{
   assert(k < maxnumcoeffs);
   Polynomial p;
   p.degree = k;
   for (u8 j = 0; j <= k; j += 1)
   {
      u64 num = factorial(k + m - j) * factorial(k);
      u64 den = factorial(k + m) * factorial(k - j) * factorial(j);
      p.coeffs[j] = (f64) num / (f64) den;
   }
   return p;
}

Polynomial q_km(u8 k, u8 m)
{
   assert(m < maxnumcoeffs);
   Polynomial q;
   q.degree = m;
   for (u8 j = 0; j <= m; j += 1)
   {
      u64 num = factorial(k + m - j) * factorial(m);
      if (j & 1)
         num *= -1;
      u64 den = factorial(k + m) * factorial(m - j) * factorial(j);
      q.coeffs[j] = (f64) num / (f64) den;
   }
   return q;
}
*/

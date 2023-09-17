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

struct ComplexF32
{
   f32 rl;
   f32 im;
};

static inline
bool isapprox(ComplexF32 a, ComplexF32 b)
{
   return isapprox(a.rl, b.rl) && isapprox(a.im, b.im);
}

struct ComplexF64
{
   f64 rl;
   f64 im;
};

static inline
ComplexF64 toComplexF64(ComplexF32 a)
{
   ComplexF64 result;
   result.rl = (f64) a.rl;
   result.im = (f64) a.im;
   return result;
}

static inline
bool isapprox(ComplexF64 a, ComplexF64 b)
{
   return isapprox(a.rl, b.rl) && isapprox(a.im, b.im);
}

static inline
void printComplexF64(ComplexF64 x)
{
   printf("%f ", x.rl);
   if (x.im > 0)
      printf("+");
   else
      printf("-");
   printf(" %fi", fabs(x.im));
}

struct Eigen
{
   ComplexF64 values[2];
   ComplexF64 vectors[2][2];
};

// TODO: compute eigenvectors
static inline
Eigen decomposition(Mat2x2F64 A)
{
   Eigen result;
   f64 a11 = A.elems[0];
   f64 a21 = A.elems[1];
   f64 a12 = A.elems[2];
   f64 a22 = A.elems[3];

   f64 trA = a11 + a22;
   f64 detA = a11 * a22 - a12 * a21;
   f64 discriminant = trA * trA - 4 * detA;

   if (discriminant < 0)
   {
      f64 λ1_rl = 0.5 * trA;
      f64 λ1_im = 0.5 * sqrt(-discriminant);
      f64 λ2_rl = λ1_rl;
      f64 λ2_im = -λ1_im;
      result.values[0] = (ComplexF64) {λ1_rl, λ1_im};
      result.values[1] = (ComplexF64) {λ2_rl, λ2_im};
   }
   else
   {
      f64 λ1 = 0.5 * (trA + sqrt(discriminant));
      f64 λ2 = 0.5 * (trA - sqrt(discriminant));
      result.values[0] = (ComplexF64) {λ1, 0};
      result.values[1] = (ComplexF64) {λ2, 0};
   }
   return result;
}

#pragma once

#include <stdio.h>

#include "useful_utils.cpp"

struct Vec2F64
{
   f64 elems[2];

   Vec2F64()
   {
      elems[0] = 0;
      elems[1] = 0;
   }

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

   Mat2x2F64()
   {
      elems[0] = 0;
      elems[1] = 0;
      elems[2] = 0;
      elems[3] = 0;
   }

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

struct Mat4x4F64
{
   f64 elems[4][4]; // column-major order: elems[c][r]

   Mat4x4F64()
   {
      for (int i = 0; i < 4; i += 1)
         for (int j = 0; j < 4; j += 1)
            elems[i][j] = 0;
   }

   Mat4x4F64(
      f64 a11, f64 a12, f64 a13, f64 a14,
      f64 a21, f64 a22, f64 a23, f64 a24,
      f64 a31, f64 a32, f64 a33, f64 a34,
      f64 a41, f64 a42, f64 a43, f64 a44)
   {
      elems[0][0] = a11;
      elems[0][1] = a21;
      elems[0][2] = a31;
      elems[0][3] = a41;
      elems[1][0] = a12;
      elems[1][1] = a22;
      elems[1][2] = a32;
      elems[1][3] = a42;
      elems[2][0] = a13;
      elems[2][1] = a23;
      elems[2][2] = a33;
      elems[2][3] = a43;
      elems[3][0] = a14;
      elems[3][1] = a24;
      elems[3][2] = a34;
      elems[3][3] = a44;
   }
};

static inline
void print(Mat4x4F64 A)
{
   printf("[\n");
   printf("[%f, %f, %f, %f]\n", A.elems[0][0], A.elems[1][0], A.elems[2][0], A.elems[3][0]);
   printf("[%f, %f, %f, %f]\n", A.elems[0][1], A.elems[1][1], A.elems[2][1], A.elems[3][1]);
   printf("[%f, %f, %f, %f]\n", A.elems[0][2], A.elems[1][2], A.elems[2][2], A.elems[3][2]);
   printf("[%f, %f, %f, %f]\n", A.elems[0][3], A.elems[1][3], A.elems[2][3], A.elems[3][3]);
   printf("]\n");
}

static inline
Mat4x4F64 matmul(Mat4x4F64 A, Mat4x4F64 B)
{
   Mat4x4F64 C;
   for (int c = 0; c < 4; c += 1)
   {
      for (int r = 0; r < 4; r += 1)
      {
         for (int k = 0; k < 4; k += 1)
         {
            C.elems[c][r] += A.elems[k][r] * B.elems[c][k];
         }
      }
   }
   return C;
}

static inline
Mat4x4F64 operator*(f64 t, Mat4x4F64 A)
{
   Mat4x4F64 C;
   for (int i = 0; i < 4; i += 1)
   {
      for (int j = 0; j < 4; j += 1)
      {
         C.elems[i][j] = t * A.elems[i][j];
      }
   }
   return C;
}

static inline
Mat4x4F64 operator+(Mat4x4F64 A, Mat4x4F64 B)
{
   Mat4x4F64 C;
   for (int i = 0; i < 4; i += 1)
   {
      for (int j = 0; j < 4; j += 1)
      {
         C.elems[i][j] = A.elems[i][j] + B.elems[i][j];
      }
   }
   return C;
}

static inline
bool isapprox(Mat4x4F64 A, Mat4x4F64 B, f64 tol = 1e-5)
{
   for (int i = 0; i < 4; i += 1)
   {
      for (int j = 0; j < 4; j += 1)
      {
         if (!isapprox(A.elems[i][j], B.elems[i][j], tol))
         {
            return false;
         }
      }
   }
   return true;
}

Mat4x4F64 Identity4()
{
   Mat4x4F64 A;
   A.elems[0][0] = 1;
   A.elems[1][1] = 1;
   A.elems[2][2] = 1;
   A.elems[3][3] = 1;
   return A;
}

static inline
Mat4x4F64 expm(Mat4x4F64 A)
{
   Mat4x4F64 result = Identity4();
   Mat4x4F64 An = Identity4();
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
ComplexF64 operator+(ComplexF64 c, f64 a)
{
   return {c.rl + a, c.im};
}

static inline
ComplexF64 operator+(f64 a, ComplexF64 c)
{
   return c + a;
}

static inline
ComplexF64 operator-(ComplexF64 c, f64 a)
{
   return c + (-a);
}

static inline
ComplexF64 operator-(f64 a, ComplexF64 c)
{
   return {a - c.rl, -c.im};
}

static inline
ComplexF64 operator*(f64 a, ComplexF64 c)
{
   return {a * c.rl, a * c.im};
}

static inline
ComplexF64 operator/(ComplexF64 c, f64 a)
{
   return (1/a) * c;
}

static inline
f64 abs(ComplexF64 c)
{
   return sqrt(c.rl * c.rl + c.im * c.im);
}

static inline
ComplexF64 conjugate(ComplexF64 c)
{
   return {c.rl, -c.im};
}

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

struct Vec2C64
{
   ComplexF64 elems[2];
};

static inline
Vec2C64 operator*(f64 a, Vec2C64 v)
{
   return {a * v.elems[0], a * v.elems[1]};
}

static inline
f64 norm(Vec2C64 v)
{
   return sqrt(pow(abs(v.elems[0]), 2) + pow(abs(v.elems[1]), 2));
}

static inline
Vec2C64 normalize(Vec2C64 v)
{
   return 1/norm(v) * v;
}

// useful references:
// - https://en.wikipedia.org/wiki/Eigenvalue_algorithm#2%C3%972_matrices
// - https://people.math.harvard.edu/~knill/teaching/math21b2004/exhibits/2dmatrices/index.html
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

   ComplexF64 λ1 = {0, 0};
   ComplexF64 λ2 = {0, 0};
   if (discriminant < 0)
   {
      λ1.rl = 0.5 * trA;
      λ1.im = 0.5 * sqrt(-discriminant);
      λ2 = conjugate(λ1);
   }
   else
   {
      f64 sqrt_discr = sqrt(discriminant);
      λ1.rl = 0.5 * (trA + sqrt_discr);
      λ2.rl = 0.5 * (trA - sqrt_discr);
   }

   Vec2C64 v1;
   Vec2C64 v2;
   if (a21 != 0)
   {
      v1.elems[0] = λ1 - a22;
      v1.elems[1] = {a21, 0};
      v2.elems[0] = λ2 - a22;
      v2.elems[1] = {a21, 0};
   }
   else if (a12 != 0)
   {
      v1.elems[0] = {a12, 0};
      v1.elems[1] = λ1 - a11;
      v2.elems[0] = {a12, 0};
      v2.elems[1] = λ2 - a11;
   }
   else
   {
      v1.elems[0] = {1, 0};
      v1.elems[1] = {0, 0};
      v2.elems[0] = {0, 0};
      v2.elems[1] = {1, 0};
   }
   v1 = normalize(v1);
   v2 = normalize(v2);

   result.values[0] = λ1;
   result.values[1] = λ2;
   result.vectors[0][0] = v1.elems[0];
   result.vectors[0][1] = v1.elems[1];
   result.vectors[1][0] = v2.elems[0];
   result.vectors[1][1] = v2.elems[1];
   return result;
}


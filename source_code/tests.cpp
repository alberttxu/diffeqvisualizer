#include <stdio.h>
#include <assert.h>

#include <raylib.h>
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/rlImGui/rlImGui.h"

#include "useful_utils.cpp"
#include "julia_helpers.cpp"
#include "linearalgebra.cpp"

typedef f64 (*g_ptr)(f64 x);
g_ptr g = NULL;

int test_julia(void)
{
   jl_init();
   eval("include(\"source_code/tests.jl\")");

   {
      puts("===========================");
      jl_value_t *ret;
      ret = jl_eval_string("f(3.0)");
      AN(ret);
      if (jl_typeis(ret, jl_float64_type))
      {
         double ret_unboxed = jl_unbox_float64(ret);
         printf("f(3.0) in C: %e \n", ret_unboxed);
      }
      else
      {
         printf("ERROR: unexpected return type\n");
      }
   }

   {
      puts("===========================");
      jl_value_t *ret;
      ret = jl_eval_string("@cfunction(g, Float64, (Float64,))");
      AN(ret);
      g = (g_ptr) jl_unbox_voidpointer(ret);
      showfloat(g(1.0));
   }

   {
      puts("===========================");
      puts("reverse test");
      jl_value_t* array_type = jl_apply_array_type((jl_value_t*)jl_float64_type, 1);
      jl_array_t* x          = jl_alloc_array_1d(array_type, 10);
      double *xData = (double*)jl_array_data(x);
      puts("before");
      for(size_t i=0; i<jl_array_len(x); i++)
      {
         xData[i] = i;
         showfloat(xData[i]);
      }
      jl_function_t *func = jl_get_function(jl_base_module, "reverse!");
      jl_call1(func, (jl_value_t*)x);
      puts("after");
      for(size_t i=0; i<jl_array_len(x); i++)
         showfloat(xData[i]);
   }

   {
      puts("===========================");
      jl_value_t *array_type = jl_apply_array_type((jl_value_t *) jl_float64_type, 1);
      jl_array_t *x = jl_alloc_array_1d(array_type, 2);
      JL_GC_PUSH1(&x);
      f64 *xData = (f64 *) jl_array_data(x);
      xData[0] = 1.0;
      xData[1] = 2.0;

      jl_function_t *h = jl_get_function(jl_main_module, "h");
      jl_value_t *boxedans = jl_call1(h, (jl_value_t *) x);
      check_if_julia_exception_occurred();
      assert(jl_typeis(boxedans, jl_float64_type));
      f64 ans = jl_unbox_float64(boxedans);
      printf("h([%f,%f]) = %f\n", xData[0], xData[1], ans);

      JL_GC_POP();
   }

   {
      puts("===========================");
      jl_value_t *array_type = jl_apply_array_type((jl_value_t *) jl_float64_type, 1);
      jl_array_t *x = jl_alloc_array_1d(array_type, 2);
      f64 *xData = (f64 *) jl_array_data(x);
      xData[0] = 1.0;
      xData[1] = 0.0;

      jl_value_t *matrix_type = jl_apply_array_type((jl_value_t *) jl_float64_type, 2);
      jl_array_t *A = jl_alloc_array_2d(matrix_type, 2, 2);
      f64 *AData = (f64 *) jl_array_data(A);
      AData[0] = 0.0;
      AData[1] = -1.0;
      AData[2] = 1.0;
      AData[3] = 0.0;

      jl_value_t *t = jl_box_float64(0.2);
      JL_GC_PUSH3(&x, &A, &t);

      jl_function_t *solve_autonomous = jl_get_function(jl_main_module, "solve_autonomous");
      jl_value_t *boxedans = jl_call3(solve_autonomous,
                                      (jl_value_t *)x, (jl_value_t *)A, (jl_value_t *)t);
      check_if_julia_exception_occurred();
      jl_array_t *xt = (jl_array_t *)boxedans;
      f64 *xtData = (f64 *)jl_array_data(xt);
      printf("exp(0.2 * [0 1; -1 0]) * [1.0,0] = [%f,%f]\n",
            xtData[0], xtData[1]);
      assert(isapprox(xtData[0], 0.9800665778412415));
      assert(isapprox(xtData[1], -0.19866933079506127));

      JL_GC_POP();
   }

   {
      puts("===========================");
      jl_value_t *matrix_type = jl_apply_array_type((jl_value_t *) jl_float64_type, 2);
      jl_array_t *A = jl_alloc_array_2d(matrix_type, 2, 2);
      f64 *AData = (f64 *) jl_array_data(A);
      // column-major order
      AData[0] = 0.804694;
      AData[1] = -0.737554;
      AData[2] = 0.419185;
      AData[3] = 0.726183;

      Eigen eigen = decomposition(A);
      printf("A = [%f %f; %f %f]\n", AData[0], AData[2], AData[1], AData[3]);
      printf("eigenvalues: ");
         printComplexF64(eigen.values[0]);
         printf(", ");
         printComplexF64(eigen.values[1]);
         printf("\n");
      printf("eigenvectors: ");
         printf("[");
         printComplexF64(eigen.vectors[0][0]);
         printf(", ");
         printComplexF64(eigen.vectors[0][1]);
         printf("]");
         printf(", ");
         printf("[");
         printComplexF64(eigen.vectors[1][0]);
         printf(", ");
         printComplexF64(eigen.vectors[1][1]);
         printf("]");
         printf("\n");
   }

   jl_atexit_hook(0);
   return 0;
}

void test_raylib_imgui(void)
{
	// Initialization
	//--------------------------------------------------------------------------------------
	int screenWidth = 1280;
	int screenHeight = 800;

	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
	InitWindow(screenWidth, screenHeight, "raylib-Extras [ImGui] example - simple ImGui Demo");
   SetTargetFPS(144);
	rlImGuiSetup(true);

	// Main game loop
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		BeginDrawing();
		ClearBackground(DARKGRAY);

		// start ImGui Conent
		rlImGuiBegin();

		// show ImGui Content
		bool open = true;
		ImGui::ShowDemoWindow(&open);

		// end ImGui Content
		rlImGuiEnd();

		EndDrawing();
		//----------------------------------------------------------------------------------
	}
	rlImGuiShutdown();

	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------
}

void test_ourlinearalgebra()
{
   {
   puts("==== matrix vector mul ====");
   Vec2F64 x(-1.030979186488353, -0.4758607105915283);
   Mat2x2F64 A(-1.05695, 0.444654, -1.44453, 0.530531);
   Vec2F64 b = matvecmul(A, x);
   assert(isapprox(b, Vec2F64(1.7770899240891282, -0.7108874546900397)));
   }

   {
   puts("==== matrix mul ====");
   Mat2x2F64 A(1, 3, 2, 4);
   Mat2x2F64 B(5, 7, 6, 8);
   assert(isapprox(matmul(A, B), Mat2x2F64(19, 43, 22, 50)));
   }

   {
   Mat4x4F64 A(
      -0.0873228, -0.363667, -0.992568 ,  0.300909,
       0.0403427,  1.91092 , -0.939411 , -0.949167,
      -0.15097  , -2.30569 ,  0.0779773,  0.362943,
      -1.21857  , -1.34864 ,  0.339221 ,  0.526252);
   Mat4x4F64 B(
       0.255513,  1.45659 , -1.07141   ,  0.631184,
      -0.781111, -1.24806 , -0.54187   ,  1.94567 ,
       1.82433 , -0.130912, -0.531278  , -0.598238,
      -0.465686, -0.165724,  0.00281076,  1.27497 );
   Mat4x4F64 ans(
      -1.68915,  0.406754,  0.818793,  0.214747,
      -2.75412, -2.04589 , -0.582273,  3.09533,
       1.73567,  2.58738 ,  1.37073 , -4.16533,
       1.11586, -0.223392,  1.85763 , -2.92514);

   Mat4x4F64 AB = matmul(A, B);
   assert(isapprox(AB, ans, 1e-4));
   }

   {
   puts("==== matrix add ====");
   Mat2x2F64 A(1, 3, 2, 4);
   Mat2x2F64 B(5, 7, 6, 8);
   assert(isapprox(A + B, Mat2x2F64(6, 10, 8, 12)));
   }

   {
   puts("==== matrix scalar mul ====");
   f64 t = 2.0;
   Mat2x2F64 A(1, 3, 2, 4);
   assert(isapprox(t * A, Mat2x2F64(2, 6, 4, 8)));
   }

   {
   puts("==== matrix exponential ====");
   Mat2x2F64 A(0, 0, 1, 0);
   assert(isapprox(expm(A), Mat2x2F64(1, 0, 1, 1)));
   f64 t = 2.0;
   assert(isapprox(expm(t * A), Mat2x2F64(1, 0, t, 1)));

   Mat2x2F64 B(-0.03140377097524905, 0.5774392661113738, -0.5030549245201645, 1.9722014759266104);
   assert(isapprox(expm(B), Mat2x2F64(0.6678580086237933, 1.7115461994715255, -1.4910689222541662, 6.606600222449386)));
   }

   {
   puts("==== eigen decomposition ====");
   Mat2x2F64 A(1, 3, 2, 4);
   Eigen eigen = decomposition(A);
   assert(isapprox(eigen.values[0].rl, 5.372281323269014));
   assert(isapprox(eigen.values[0].im, 0));
   assert(isapprox(eigen.values[1].rl, -0.3722813232690143));
   assert(isapprox(eigen.values[1].im, 0));
   assert(isapprox(eigen.vectors[0][0].rl, 0.415974));
   assert(isapprox(eigen.vectors[0][1].rl, 0.909377));
   assert(isapprox(eigen.vectors[1][0].rl, -0.824565));
   assert(isapprox(eigen.vectors[1][1].rl, 0.565767));
   assert(eigen.vectors[0][0].im == 0);
   assert(eigen.vectors[0][1].im == 0);
   assert(eigen.vectors[1][0].im == 0);
   assert(eigen.vectors[1][1].im == 0);
   }
}

int main(void)
{
   /* test_julia(); */
   /* test_raylib_imgui(); */
   test_ourlinearalgebra();
   return 0;
}

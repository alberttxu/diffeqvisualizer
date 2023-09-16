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

   /* puts("==== polynomial eval ===="); */
   /* f64 coeffs[3] = {1, 2, 3}; */
   /* u8 degree = 2; */
   /* Polynomial p = newPolynomial(coeffs, degree); */
   /* assert(isapprox(eval(p, 5), 86)); */

   /* puts("==== factorial ===="); */
   /* assert(factorial(7) == 5040); */
}

int main(void)
{
   /* test_julia(); */
   /* test_raylib_imgui(); */
   test_ourlinearalgebra();
   return 0;
}

#include <stdio.h>
#include <assert.h>

#include <raylib.h>
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/rlImGui/rlImGui.h"

#include "useful_utils.cpp"
#include "julia_helpers.cpp"

typedef f64 (*g_ptr)(f64 x);
g_ptr g = NULL;

int test_julia(void)
{
   jl_init();
   eval("include(\"source_code/compute.jl\")");

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

int main(void)
{
   /* test_julia(); */
   test_raylib_imgui();
   return 0;
}

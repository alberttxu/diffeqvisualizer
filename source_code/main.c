#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <raylib.h>
#include <julia.h>

#include "useful_utils.c"

// modified from
// https://blog.esciencecenter.nl/10-examples-of-embedding-julia-in-c-c-66282477e62c
void check_if_julia_exception_occurred(void)
{
   jl_value_t *ex = jl_exception_occurred();
   if (ex == NULL)
      return;

   jl_printf(jl_stderr_stream(), "Exception:\n");
   jl_call2(
      jl_get_function(jl_base_module, "showerror"),
      jl_stderr_obj(),
      ex
   );
   jl_printf(jl_stderr_stream(), "\n");
   jl_atexit_hook(1);
   exit(1);
}

jl_value_t *eval(const char* code)
{
   jl_value_t *result = jl_eval_string(code);
   check_if_julia_exception_occurred();
   assert(result && "Missing return value but no exception occurred!");
   return result;
}


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
      f64 *xtData = jl_array_data(xt);
      printf("exp(0.2 * [0 1; -1 0]) * [1.0,0] = [%f,%f]\n",
            xtData[0], xtData[1]);
      assert(isapprox(xtData[0], 0.9800665778412415));
      assert(isapprox(xtData[1], -0.19866933079506127));

      JL_GC_POP();
   }

   jl_atexit_hook(0);
   return 0;
}


const int screenWidth = 800;
const int screenHeight = 450;
#define pixelsperunit 100

// 0,0 = center of screen
// +y = up
// +x = right
static inline
Vector2 coords2pixels(Vector2 graph_coords)
{
   Vector2 pixel_coords = {
       graph_coords.x * pixelsperunit + screenWidth / 2,
      -graph_coords.y * pixelsperunit + screenHeight / 2
   };
   return pixel_coords;
}

int appmain(void)
{
   InitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard input");
   SetTargetFPS(60);

#define histcapacity 20
   Vector2 recentBallPositions[histcapacity] = {0}; // ring buffer
   int curidx = 0;
   int histsize = 0;

   f32 theta = 0;
   f32 radius = 1;

   f64 t = 0;

   jl_init();
   eval("include(\"source_code/compute.jl\")");

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

   JL_GC_PUSH2(&x, &A);

   jl_function_t *solve_autonomous = jl_get_function(jl_main_module, "solve_autonomous");
   check_if_julia_exception_occurred();

   while (!WindowShouldClose())   // Detect window close button or ESC key
   {
      if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyDown(KEY_W))
         break;

#if 0
      Vector2 ballPosition = {radius * cosf(theta), radius * sinf(theta)};
#else

      jl_value_t *boxedans = jl_call3(solve_autonomous,
                                      (jl_value_t *)x, (jl_value_t *)A, jl_box_float64(t));
      JL_GC_PUSH1(&boxedans);
      check_if_julia_exception_occurred();
      jl_array_t *xt = (jl_array_t *)boxedans;
      f64 *xtData = jl_array_data(xt);
      Vector2 ballPosition = {(f32)xtData[0], (f32)xtData[1]};
      JL_GC_POP();
#endif

      recentBallPositions[curidx] = coords2pixels(ballPosition);
      {
         BeginDrawing();
         ClearBackground(RAYWHITE);

         for (int i = 0; i < histsize; i++)
         {
            f32 radius = 6.0f - 0.5f * i;
            int j = curidx - i;
            if (j < 0)
              j += histcapacity;
            DrawCircleV(recentBallPositions[j], radius, MAROON);
         }

         DrawFPS(10, 10);
         EndDrawing();
      }
      curidx = (curidx+1) % histcapacity;
      histsize = min(histcapacity, histsize + 1);

      theta += 0.05f;
      t += 0.05;
   }

   CloseWindow();
   jl_atexit_hook(0);
   return 0;
}

int main(void)
{
   /* test_julia(); */
   appmain();
   return 0;
}

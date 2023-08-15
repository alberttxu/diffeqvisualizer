#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <raylib.h>
#include <julia.h>

#include "useful_utils.c"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;

// modified from
// https://blog.esciencecenter.nl/10-examples-of-embedding-julia-in-c-c-66282477e62c
void handle_julia_exception(void)
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
   handle_julia_exception();
   assert(result && "Missing return value but no exception occurred!");
   return result;
}

int main(void)
{

   jl_init();

   eval("include(\"source_code/compute.jl\")");

   jl_atexit_hook(0);
   return 0;
}


typedef f64 (*g_ptr)(f64 x);
g_ptr g = NULL;

int main3(void)
{
   jl_init();

   jl_value_t *ret;
   ret = jl_eval_string("include(\"source_code/compute.jl\")");
   AN(ret);

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

   ret = jl_eval_string("@cfunction(g, Float64, (Float64,))");
   AN(ret);
   g = (g_ptr) jl_unbox_voidpointer(ret);
   showfloat(g(1.0));

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

int main2(int argc, char **argv)
{
   InitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard input");
   SetTargetFPS(60);

#define histcapacity 20
   Vector2 recentBallPositions[histcapacity] = {0}; // ring buffer
   int curidx = 0;
   int histsize = 0;

   f32 theta = 0;
   f32 radius = 1;

   /* f64 t = 0; */

   while (!WindowShouldClose())   // Detect window close button or ESC key
   {
      if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyDown(KEY_W))
         break;

#if 1
      Vector2 ballPosition = {radius * cosf(theta), radius * sinf(theta)};
#else

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
      /* t += 0.05; */
   }

   CloseWindow();
   return 0;
}

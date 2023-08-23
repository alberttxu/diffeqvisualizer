#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <raylib.h>
#include <julia.h>

#include "useful_utils.c"
#include "julia_helpers.c"

#define RAYGUI_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "../dependencies/raygui-3.6/src/raygui.h"
#pragma GCC diagnostic pop

#include "../dependencies/tracy/public/tracy/TracyC.h"

#ifdef __cplusplus
extern "C" {
#endif

#define screenWidth 1618
#define screenHeight 1000
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
   SetTargetFPS(62);

#define numballs 30
#define histcapacity 50
   Vector2 recentBallPositions[numballs][histcapacity] = {0}; // ring buffer
   int curidx = 0;
   int histsize = 0;

   f64 t = 0;

   jl_init();
   eval("include(\"source_code/compute.jl\")");

   jl_value_t *array_type = jl_apply_array_type((jl_value_t *) jl_float64_type, 2);
   jl_array_t *x = jl_alloc_array_2d(array_type, 2, numballs);
   f64 *xData = (f64 *) jl_array_data(x);
   for (int i = 0; i < numballs; i += 1)
   {
      xData[2*i + 0] = randfloat64(-5, 5);
      xData[2*i + 1] = randfloat64(-5, 5);
   }

   jl_value_t *matrix_type = jl_apply_array_type((jl_value_t *) jl_float64_type, 2);
   jl_array_t *A = jl_alloc_array_2d(matrix_type, 2, 2);
   f64 *AData = (f64 *) jl_array_data(A);
   AData[0] = 0.6483820385272346;
   AData[1] = -1.8086122374496931;
   AData[2] = 0.9185619253668453;
   AData[3] = -0.7559322222668067;

   JL_GC_PUSH2(&x, &A);

   jl_function_t *solve_autonomous = jl_get_function(jl_main_module, "solve_autonomous");
   check_if_julia_exception_occurred();

   while (!WindowShouldClose())   // Detect window close button or ESC key
   {
      TracyCFrameMark;
      BeginDrawing();

      if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyDown(KEY_W))
         break;

      f64 *xtData;
      {
         TracyCZoneN(julia, "julia", true);
         jl_value_t *boxedans = jl_call3(solve_autonomous,
                                         (jl_value_t *)x, (jl_value_t *)A, jl_box_float64(t));
         JL_GC_PUSH1(&boxedans);
         check_if_julia_exception_occurred();
         jl_array_t *xt = (jl_array_t *)boxedans;
         xtData = (f64 *)jl_array_data(xt);
         TracyCZoneEnd(julia);
      }

      Vector2 ballPositions[numballs];
      for (int i = 0; i < numballs; i += 1)
      {
         ballPositions[i].x = (f32)xtData[2*i + 0];
         ballPositions[i].y = (f32)xtData[2*i + 1];
      }

      JL_GC_POP();

      for (int n = 0; n < numballs; n++)
      {
         recentBallPositions[n][curidx] = coords2pixels(ballPositions[n]);
      }

      bool reset = false;
      {
         TracyCZoneN(draw, "draw", true);

         ClearBackground(RAYWHITE);

         for (int n = 0; n < numballs; n++)
         {
            for (int i = 0; i < histsize; i++)
            {
               f32 radius = 6.0f - 0.1f * i;
               int j = curidx - i;
               if (j < 0)
                 j += histcapacity;
               DrawCircleV(recentBallPositions[n][j], radius, MAROON);
            }
         }

         reset = GuiButton((Rectangle){ 25, 255, 100, 30 }, "reset");

         DrawFPS(10, 10);
         DrawText(TextFormat("t = %f", t), 10, 30, 20, DARKGRAY);

         TracyCZoneEnd(draw);
      }

      t += 0.02;

      if (reset)
      {
         t = 0;
         histsize = 0;

         for (int i = 0; i < numballs; i += 1)
         {
            xData[2*i + 0] = randfloat64(-5, 5);
            xData[2*i + 1] = randfloat64(-5, 5);
         }
      }

      curidx = (curidx+1) % histcapacity;
      histsize = min(histcapacity, histsize + 1);

      EndDrawing(); // raylib will wait until next frame
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

#ifdef __cplusplus
} // extern C
#endif

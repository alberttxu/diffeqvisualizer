// standard library
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// third-party libraries
#include <raylib.h>
#include <julia.h>
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/rlImGui/rlImGui.h"

#define RAYGUI_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "../dependencies/raygui/src/raygui.h"
#pragma GCC diagnostic pop

#include "../dependencies/tracy/public/tracy/Tracy.hpp"

// our code
#include "useful_utils.cpp"
#include "julia_helpers.cpp"

#define screenwidth 1618
#define screenheight 1000
#define pixelsperunit 100
#define targetfps 62
#define targetperiod (1.0/(f64)targetfps)

// 0,0 = center of screen
// +y = up
// +x = right
static inline
Vector2 coords2pixels(Vector2 graph_coords)
{
   Vector2 pixel_coords = {
       graph_coords.x * pixelsperunit + screenwidth / 2,
      -graph_coords.y * pixelsperunit + screenheight / 2
   };
   return pixel_coords;
}

int main(void)
{
   InitWindow(screenwidth, screenheight, "raylib [core] example - keyboard input");
   SetTargetFPS(targetfps);
   rlImGuiSetup(true);

#define numballs 30
#define histcapacity 20
   Vector2 recentBallPositions[numballs][histcapacity]; // ring buffer
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

   f64 prevframetime_ms = 0;
   bool paused = false;
   bool resetwasclicked = false;
   bool pausewasclicked = false;
   bool resumewasclicked = false;

   while (!WindowShouldClose())   // Detect window close button or ESC key
   {
      FrameMark;
      /* f64 t_framestart = GetTime(); */
      PollInputEvents();

      if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyDown(KEY_W))
         break;

      BeginDrawing();

      ClearBackground(RAYWHITE);

      DrawText(TextFormat("Draw time: %02.02f ms", prevframetime_ms), 10, 50, 20, DARKGRAY);
      DrawText(TextFormat("t = %f", t), 10, 30, 20, DARKGRAY);

      Vector2 ballPositions[numballs];

      { ZoneScopedN("julia");

      jl_value_t *matrix_2xN_ballpositions = jl_call3(
            solve_autonomous, (jl_value_t *)x, (jl_value_t *)A, jl_box_float64(t));
      JL_GC_PUSH1(&matrix_2xN_ballpositions);
      check_if_julia_exception_occurred();

      jl_array_t *xt = (jl_array_t *)matrix_2xN_ballpositions;
      f64 *xtData = (f64 *)jl_array_data(xt);
      for (int i = 0; i < numballs; i += 1)
      {
         ballPositions[i].x = (f32)xtData[2*i + 0];
         ballPositions[i].y = (f32)xtData[2*i + 1];
      }
      JL_GC_POP();
      }

      for (int n = 0; n < numballs; n++)
      {
         recentBallPositions[n][curidx] = coords2pixels(ballPositions[n]);
      }

      { ZoneScopedN("draw balls");
      for (int n = 0; n < numballs; n++)
      {
         for (int i = 0; i < histsize; i++)
         {
            f32 radius = 3.0f - 0.1f * i;
            int j = curidx - i;
            if (j < 0)
               j += histcapacity;
            DrawCircleV(recentBallPositions[n][j], radius, MAROON);
         }
      }
      }

      { ZoneScopedN("Post-iteration work");

      rlImGuiBegin();
      ImGui::Begin("Controls");

      resetwasclicked = ImGui::Button("reset");
      if (resetwasclicked)
      {
         t = 0;
         histsize = 0;

         for (int i = 0; i < numballs; i += 1)
         {
            xData[2*i + 0] = randfloat64(-5, 5);
            xData[2*i + 1] = randfloat64(-5, 5);
         }
      }

      if (paused)
      {
         DrawText("Paused", screenwidth - 100, 20, 20, DARKGRAY);
         resumewasclicked = ImGui::Button("resume");
         if (resumewasclicked)
            paused = false;
      }
      else
      {
         curidx = (curidx+1) % histcapacity;
         histsize = min(histcapacity, histsize + 1);
         t += 0.02;
         pausewasclicked = ImGui::Button("pause");
         if (pausewasclicked)
            paused = true;
      }

      ImGui::End();
      rlImGuiEnd();
      }

      EndDrawing();

      /* f64 t_frameend = GetTime(); */
      /* f64 period = t_frameend - t_framestart; */
      /* prevframetime_ms = period * 1000; */
      /* WaitTime(max(0, targetperiod - period)); */
   }

   rlImGuiShutdown();
   CloseWindow();
   jl_atexit_hook(0);
   return 0;
}

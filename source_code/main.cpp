// standard library
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// third-party libraries
#include <raylib.h>
#include <julia.h>
#include "../dependencies/rlImGui/rlImGui.h"
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/tracy/public/tracy/Tracy.hpp"

// our code
#include "useful_utils.cpp"
#include "julia_helpers.cpp"

#define screenwidth 1618
#define screenheight 1000
#define targetfps 62
#define targetperiod (1.0/(f64)targetfps)

// zoom level
int pixelsperunit = 100;

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

static inline
void drawcoordaxes()
{
   int x0 = screenwidth/2;
   int y0 = screenheight/2;
   DrawLine(0, y0, screenwidth-1, y0, BLACK);
   DrawLine(x0, 0, x0, screenheight-1, BLACK);

   int ticklen = 5;
   for (int x = x0, xval = 0; x < screenwidth; x += pixelsperunit, xval += 1)
   {
      DrawLine(x, y0 + ticklen, x, y0 - ticklen, BLACK);
      if (xval != 0 && xval % 10 == 0)
         DrawText(TextFormat("%d", xval), x - 10, y0 + ticklen + 10, 20, DARKGRAY);
   }
   for (int x = x0, xval = 0; x >= 0; x -= pixelsperunit, xval -= 1)
   {
      DrawLine(x, y0 + ticklen, x, y0 - ticklen, BLACK);
      if (xval != 0 && xval % 10 == 0)
         DrawText(TextFormat("%d", xval), x - 15, y0 + ticklen + 10, 20, DARKGRAY);
   }
   for (int y = y0, yval = 0; y < screenheight; y += pixelsperunit, yval -= 1)
   {
      DrawLine(x0 - ticklen, y, x0 + ticklen, y, BLACK);
      if (yval != 0 && yval % 10 == 0)
         DrawText(TextFormat("%d", yval), x0 + ticklen + 15, y - 10, 20, DARKGRAY);
   }
   for (int y = y0, yval = 0; y >= 0; y -= pixelsperunit, yval += 1)
   {
      DrawLine(x0 - ticklen, y, x0 + ticklen, y, BLACK);
      if (yval != 0 && yval % 10 == 0)
         DrawText(TextFormat("%d", yval), x0 + ticklen + 15, y - 10, 20, DARKGRAY);
   }
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
   AData[0] = -0.97;
   AData[1] = 0;
   AData[2] = 25;
   AData[3] = -0.3;

   JL_GC_PUSH2(&x, &A);

   jl_function_t *solve_autonomous = jl_get_function(jl_main_module, "solve_autonomous");
   check_if_julia_exception_occurred();

   f64 prevframetime_ms = 0;
   bool paused = false;
   bool resetwasclicked = false;
   bool pausewasclicked = false;
   bool resumewasclicked = false;

   f64 newAData[4];
   memcpy(newAData, AData, 4 * sizeof(newAData[0]));

   while (!WindowShouldClose())   // Detect window close button or ESC key
   {
      FrameMark;
      f64 t_framestart = GetTime();

      if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyDown(KEY_W))
         break;

      pixelsperunit = (int) (powf(1.05f, GetMouseWheelMove()) * pixelsperunit);
      pixelsperunit = clampint(pixelsperunit, 20, 1000);

      BeginDrawing();

      ClearBackground(RAYWHITE);

      drawcoordaxes();

      DrawText(TextFormat("Draw time: %02.02f ms", prevframetime_ms), 10, 50, 20, DARKGRAY);
      DrawText(TextFormat("t = %f", t), 10, 30, 20, DARKGRAY);
      DrawText(TextFormat("pixelsperunit = %d", pixelsperunit), 10, 70, 20, DARKGRAY);

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
         recentBallPositions[n][curidx] = ballPositions[n];
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
            DrawCircleV(coords2pixels(recentBallPositions[n][j]), radius, MAROON);
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
         memcpy(AData, newAData, 4 * sizeof(newAData[0]));
      }

      if (paused)
      {
         DrawText("Paused", screenwidth - 100, 20, 20, DARKGRAY);
         resumewasclicked = ImGui::Button("resume");
         if (resumewasclicked || IsKeyPressed(KEY_SPACE))
            paused = false;
      }
      else
      {
         curidx = (curidx+1) % histcapacity;
         histsize = min(histcapacity, histsize + 1);
         t += 0.02;
         pausewasclicked = ImGui::Button("pause");
         if (pausewasclicked || IsKeyPressed(KEY_SPACE))
            paused = true;
      }
      ImGui::End();

      ImGui::Begin("Matrix");
      ImGui::InputDouble("A11", &newAData[0]);
      ImGui::InputDouble("A21", &newAData[1]);
      ImGui::InputDouble("A12", &newAData[2]);
      ImGui::InputDouble("A22", &newAData[3]);
      ImGui::End();

      rlImGuiEnd();
      }

      f64 t_frameend = GetTime();
      f64 period = t_frameend - t_framestart;
      prevframetime_ms = period * 1000;

      EndDrawing();
   }

   rlImGuiShutdown();
   CloseWindow();
   jl_atexit_hook(0);
   return 0;
}

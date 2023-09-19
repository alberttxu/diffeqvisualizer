// standard library
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// third-party libraries
#include <raylib.h>
#include "../dependencies/rlImGui/rlImGui.h"
#include "../dependencies/imgui/imgui.h"
#include "../dependencies/tracy/public/tracy/Tracy.hpp"

// our code
#include "useful_utils.cpp"

/* #define JULIA_BACKEND */

#ifdef JULIA_BACKEND
   #include <julia.h>
   #include "julia_helpers.cpp"
#else
   #include "linearalgebra.cpp"
#endif

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
Vector2 coords2pixels(Vec2F64 graph_coords)
{
   Vector2 coords_f32;
   coords_f32.x = (f32) graph_coords.elems[0];
   coords_f32.y = (f32) graph_coords.elems[1];
   return coords2pixels(coords_f32);
}

static inline
Vec2F64 pixels2coords(Vector2 pixel_coords)
{
   return {
       ((f64)pixel_coords.x - screenwidth / 2) / pixelsperunit,
      -((f64)pixel_coords.y - screenheight / 2) / pixelsperunit,
   };
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

#define numtrajectories 100

static inline
void resetstates(f64 *arr_2xN)
{
   f64 boxlim = 40;
   for (int i = 0; i < numtrajectories; i += 1)
   {
      arr_2xN[2*i + 0] = randfloat64(-boxlim, boxlim);
      arr_2xN[2*i + 1] = randfloat64(-boxlim, boxlim);
   }
}

#define histcapacity 16
struct Queue
{
   Vec2F64 recentpositions[histcapacity];
   int curidx;
   int size;
};

void initQueue(Queue *q)
{
   memset(q->recentpositions, 0, histcapacity * sizeof(q->recentpositions[0]));
   q->curidx = 0;
   q->size = 0;
}

void updateposition(Queue *q, Vec2F64 position)
{
   q->recentpositions[q->curidx] = position;
   q->curidx = (q->curidx + 1) % histcapacity;
   q->size = min(histcapacity, q->size + 1);
}

#ifdef JULIA_BACKEND
static inline
void addstate(f64 *currentstates, int idx, Vec2F64 newtrajectorycoords)
{
   assert(inarraybounds(&currentstates[2*idx + 0], currentstates, &currentstates[2*numtrajectories]));
   assert(inarraybounds(&currentstates[2*idx + 1], currentstates, &currentstates[2*numtrajectories]));
   currentstates[2*idx + 0] = newtrajectorycoords.elems[0];
   currentstates[2*idx + 1] = newtrajectorycoords.elems[1];
}
#else
static inline
void addstate(Vec2F64 *currentstates, int idx, Vec2F64 newtrajectorycoords)
{
   currentstates[idx] = newtrajectorycoords;
}
#endif

int main(void)
{
   InitWindow(screenwidth, screenheight, "raylib [core] example - keyboard input");
   SetTargetFPS(targetfps);
   rlImGuiSetup(true);
   ImGuiIO& io = ImGui::GetIO();

   Queue trajectories[numtrajectories];
   int newtrajidx = 0;
   for (int i = 0; i < numtrajectories; i++)
      initQueue(&trajectories[i]);

   f64 t = 0;
#define dt 0.02

#ifdef JULIA_BACKEND
   jl_init();
   eval("include(\"source_code/compute.jl\")");

   /* jl_value_t *jlVecF64type = jl_apply_array_type((jl_value_t *) jl_float64_type, 1); */
   jl_value_t *jlMatF64type = jl_apply_array_type((jl_value_t *) jl_float64_type, 2);

   jl_array_t *x = jl_alloc_array_2d(jlMatF64type, 2, numtrajectories);
   f64 *currentstates = (f64 *) jl_array_data(x);

   jl_array_t *A = jl_alloc_array_2d(jlMatF64type, 2, 2);
   f64 *AData = (f64 *) jl_array_data(A);

   JL_GC_PUSH2(&x, &A);

   jl_function_t *solve_autonomous = getfunc("solve_autonomous");
#else
   Vec2F64 currentstates[numtrajectories];
   Mat2x2F64 A(0, 0, 0, 0);
   f64 *AData = A.elems;
#endif
   resetstates((f64 *)currentstates);

   f32 newAData[4] = {0, 0, 0, 0};

   f64 prevframetime_ms = 0;
   bool paused = false;
   bool resetwasclicked = false;
   bool pausewasclicked = false;
   bool resumewasclicked = false;

   while (!WindowShouldClose())   // Detect window close button or ESC key
   {
      FrameMark;
      f64 t_framestart = GetTime();

      if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyDown(KEY_W))
         break;

      if (!io.WantCaptureMouse)
         pixelsperunit = (int) (powf(1.05f, GetMouseWheelMove()) * pixelsperunit);
      pixelsperunit = clampint(pixelsperunit, 20, 1000);

      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !io.WantCaptureMouse)
      {
         Vec2F64 newtrajectorycoords = pixels2coords(GetMousePosition());
         initQueue(&trajectories[newtrajidx]);
         addstate(currentstates, newtrajidx, newtrajectorycoords);
         newtrajidx = (newtrajidx + 1) % numtrajectories;
      }

      { ZoneScopedN("update trajectories");
#ifdef JULIA_BACKEND
      jl_value_t *matrix_2xN_newstates = call(solve_autonomous, x, A, jl_box_float64(dt));
      JL_GC_PUSH1(&matrix_2xN_newstates);
      f64 *newstates = (f64 *)jl_array_data((jl_array_t *) matrix_2xN_newstates);

      if (!paused)
      {
         for (int i = 0; i < numtrajectories; i++)
         {
            updateposition(&trajectories[i], *(Vec2F64 *)&newstates[2*i]);
         }
         memcpy(currentstates, newstates, numtrajectories * 2 * sizeof(f64));
      }
      JL_GC_POP();
#else
      Mat2x2F64 updateMatrix = expm(dt * A);
      for (int i = 0; i < numtrajectories; i += 1)
      {
         Vec2F64 newstate = matvecmul(updateMatrix, currentstates[i]);

         if (!paused)
         {
            updateposition(&trajectories[i], newstate);
            currentstates[i] = newstate;
         }
      }
#endif
      }

      BeginDrawing();
      ClearBackground(RAYWHITE);
      drawcoordaxes();

      DrawText(TextFormat("Draw time: %02.02f ms", prevframetime_ms), 10, 50, 20, DARKGRAY);
      DrawText(TextFormat("t = %f", t), 10, 30, 20, DARKGRAY);
      DrawText(TextFormat("pixelsperunit = %d", pixelsperunit), 10, 70, 20, DARKGRAY);

      { ZoneScopedN("draw trajectories");
      for (int i = 0; i < numtrajectories; i++)
      {
         Queue trajectory = trajectories[i];
         for (int ago = 0; ago < trajectory.size; ago++)
         {
            f32 radius = 3.0f - 0.1f * ago;
            int j = trajectory.curidx - 1 - ago;
            if (j < 0)
               j += histcapacity;
            DrawCircleV(coords2pixels(trajectory.recentpositions[j]), radius, MAROON);
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

         resetstates((f64 *)currentstates);
         for (int i = 0; i < numtrajectories; i++)
            initQueue(&trajectories[i]);
      }

      if (paused)
      {
         DrawText("Paused", screenwidth - 100, 20, 20, DARKGRAY);
         resumewasclicked = ImGui::Button("resume");
         if (resumewasclicked || (IsKeyPressed(KEY_SPACE) && !io.WantCaptureKeyboard))
            paused = false;
      }
      else
      {
         t += dt;

         pausewasclicked = ImGui::Button("pause");
         if (pausewasclicked || (IsKeyPressed(KEY_SPACE) && !io.WantCaptureKeyboard))
            paused = true;
      }
      ImGui::End();

      {
      ImGui::Begin("Matrix editor");
      bool A_was_modified[4] = {false, false, false, false};

      ImGui::BeginTable("A", 2);
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      A_was_modified[0] = ImGui::SliderFloat("A11", &newAData[0], -20, 20);
      ImGui::TableNextColumn();
      A_was_modified[1] = ImGui::SliderFloat("A12", &newAData[2], -20, 20);
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      A_was_modified[2] = ImGui::SliderFloat("A21", &newAData[1], -20, 20);
      ImGui::TableNextColumn();
      A_was_modified[3] = ImGui::SliderFloat("A22", &newAData[3], -20, 20);
      ImGui::EndTable();

      Eigen eigen;
      if (any(A_was_modified, 4))
      {
         for (int i = 0; i < 4; i += 1)
            AData[i] = (f64) newAData[i];
      }

      eigen = decomposition(A);

      ImGui::Text("eigenvalues:\n%f + %f i,\n%f + %f i\n",
            eigen.values[0].rl, eigen.values[0].im,
            eigen.values[1].rl, eigen.values[1].im);
      ImGui::Text("eigenvectors:\n[%f + %f i, %f + %f i]\n[%f + %f i, %f + %f i]",
            eigen.vectors[0][0].rl, eigen.vectors[0][0].im,
            eigen.vectors[0][1].rl, eigen.vectors[0][1].im,
            eigen.vectors[1][0].rl, eigen.vectors[1][0].im,
            eigen.vectors[1][1].rl, eigen.vectors[1][1].im);
      ImGui::End();
      }

      rlImGuiEnd();
      }

      f64 t_frameend = GetTime();
      f64 period = t_frameend - t_framestart;
      prevframetime_ms = period * 1000;

      EndDrawing();
   }

   return 0;
}

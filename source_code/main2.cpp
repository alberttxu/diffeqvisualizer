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
#ifdef WEB
   #include <emscripten.h>
#endif

// our code
#include "useful_utils.cpp"
#include "linearalgebra.cpp"

#define targetfps 62
#define targetperiod (1.0/(f64)targetfps)

int screenwidth = 1200;
int screenheight = 800;
// zoom level
int pixelsperunit = 20;

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

f64 t = 0;
constexpr f64 dt = 0.02;
Vec2F64 currentstate;

bool paused = false;
bool pausewasclicked = false;
bool resumewasclicked = false;

void gameloop()
{
   FrameMark;
   static int framenumber = 0;
   framenumber += 1;
   static ImGuiIO& io = ImGui::GetIO();
   static f64 prevframetime_ms = 0;
   f64 t_framestart = GetTime();

   screenwidth = GetScreenWidth();
   screenheight = GetScreenHeight();

   if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyDown(KEY_W))
      return;

   if (!io.WantCaptureMouse)
      pixelsperunit = (int) (powf(1.05f, GetMouseWheelMove()) * pixelsperunit);
   pixelsperunit = clampint(pixelsperunit, 20, 1000);

   BeginDrawing();
   rlImGuiBegin();
   ClearBackground(RAYWHITE);
   drawcoordaxes();

   DrawText(TextFormat("Frame time: %02.02f ms", prevframetime_ms), 10, 50, 20, DARKGRAY);
   DrawText(TextFormat("t = %f", t), 10, 30, 20, DARKGRAY);
   DrawText(TextFormat("pixelsperunit = %d", pixelsperunit), 10, 70, 20, DARKGRAY);

   constexpr f32 groundY_pix = 500;
   constexpr f32 wallX_pix = 50;
   constexpr int pixelspermeter = 100;
   constexpr f32 groundwidth_m = 8;
   constexpr f32 wallheight_m = 3;
   constexpr f32 boxsize_m = 1;

   static f32 equilibrium_pos_m = groundwidth_m / 2;
   static f32 box_pos_m = equilibrium_pos_m;

   DrawLineEx(
         (Vector2){wallX_pix, groundY_pix - wallheight_m * pixelspermeter},
         (Vector2){wallX_pix, groundY_pix}, 4, BLACK);
   DrawLineEx(
         (Vector2){wallX_pix, groundY_pix},
         (Vector2){wallX_pix + groundwidth_m * pixelspermeter, groundY_pix}, 4, BLACK);

   ImGui::Begin("Box");

   bool boxwasmoved = ImGui::SliderFloat(
         "box position",
         &box_pos_m,
         0.5f * boxsize_m,
         groundwidth_m - 0.5f * boxsize_m);

   currentstate.elems[0] = box_pos_m - equilibrium_pos_m;

   if (boxwasmoved)
   {
      paused = true;
      currentstate.elems[1] = 0;
   }


   DrawRectangleLinesEx(
         (Rectangle){
            wallX_pix + (box_pos_m - 0.5f * boxsize_m) * pixelspermeter,
            groundY_pix - boxsize_m * pixelspermeter,
            boxsize_m * pixelspermeter,
            boxsize_m * pixelspermeter},
         4,
         BLUE);

   static f32 boxMass_kg = 0.5;
   static f32 k_springconstant = 5;
   ImGui::SliderFloat("mass, kg", &boxMass_kg, 0.01, 5);
   ImGui::SliderFloat("k_spring", &k_springconstant, 0, 20);
   DrawCircleV(coords2pixels(currentstate), 3, MAROON);

   Mat2x2F64 A = {0, -k_springconstant / boxMass_kg, 1, 0};
   Eigen eigen = decomposition(A);
   Vec2F64 newstate = matvecmul(expm(dt * A), currentstate);

   ImGui::Text("A:[%f %f\n%f %f]", A.elems[0], A.elems[2], A.elems[1], A.elems[3]);
   ImGui::Text("eigenvalues:\n%f + %f i,\n%f + %f i\n",
         eigen.values[0].rl, eigen.values[0].im,
         eigen.values[1].rl, eigen.values[1].im);
   ImGui::Text("eigenvectors:\n[%f + %f i, %f + %f i]\n[%f + %f i, %f + %f i]",
         eigen.vectors[0][0].rl, eigen.vectors[0][0].im,
         eigen.vectors[0][1].rl, eigen.vectors[0][1].im,
         eigen.vectors[1][0].rl, eigen.vectors[1][0].im,
         eigen.vectors[1][1].rl, eigen.vectors[1][1].im);

   if (paused)
   {
      DrawText("Paused", screenwidth - 100, 20, 20, DARKGRAY);
      resumewasclicked = ImGui::Button("resume");
      if (resumewasclicked || (IsKeyPressed(KEY_SPACE) && !io.WantCaptureKeyboard))
         paused = false;
   }
   else
   {
      currentstate = newstate;
      box_pos_m = newstate.elems[0] + equilibrium_pos_m;
      t += dt;

      pausewasclicked = ImGui::Button("pause");
      if (pausewasclicked || (IsKeyPressed(KEY_SPACE) && !io.WantCaptureKeyboard))
         paused = true;
   }


   f64 t_frameend = GetTime();
   f64 period = t_frameend - t_framestart;
   prevframetime_ms = period * 1000;

   ImGui::End();
   rlImGuiEnd();
   EndDrawing();
}

int main(void)
{
   SetConfigFlags(FLAG_WINDOW_RESIZABLE);
   InitWindow(screenwidth, screenheight, "diffeqvisualizer");
   rlImGuiSetup(true);

#ifdef WEB
   // https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop
   int fps = 0; // let browser handle fps
   bool simulate_infinite_loop = false;
   emscripten_set_main_loop(gameloop, fps, simulate_infinite_loop);
#else
   SetTargetFPS(targetfps);
   while (!WindowShouldClose())   // Detect window close button or ESC key
      gameloop();
#endif

   return 0;
}

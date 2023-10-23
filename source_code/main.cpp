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
#ifdef JULIA_BACKEND
   #include <julia.h>
   #include "julia_helpers.cpp"
#endif

#define targetfps 62
#define targetperiod (1.0/(f64)targetfps)

int screenwidth = 800;
int screenheight = 600;
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

int framenumber = 0;

#include "trajectories.cpp"
#include "harmonic_oscillator.cpp"
#include "one_dimension.cpp"

void gameloop()
{
   FrameMark;
   framenumber += 1;
   f64 t_framestart = GetTime();

   screenwidth = GetScreenWidth();
   screenheight = GetScreenHeight();

   if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyDown(KEY_W))
      return;

   ImGuiIO& io = ImGui::GetIO();
   if (!io.WantCaptureMouse)
      pixelsperunit = (int) (powf(1.05f, GetMouseWheelMove()) * pixelsperunit);
   pixelsperunit = clampint(pixelsperunit, 20, 1000);

   BeginDrawing();
   rlImGuiBegin();
   ClearBackground(RAYWHITE);

   ImGui::Begin("Examples");
   const char* examples[] = {
      "1-D",
      "Trajectories (2-D)",
      "Harmonic oscillator",
   };
   static int example_idx = 1;
   ImGui::Combo("Demo", &example_idx, examples, IM_ARRAYSIZE(examples));
   ImGui::End();

   if (example_idx == 0)
      gameloop_onedim();
   else if (example_idx == 1)
      gameloop_trajectories();
   else if (example_idx == 2)
      gameloop_oscillator();

   f64 t_frameend = GetTime();
   f64 period = t_frameend - t_framestart;
   prevframetime_ms = period * 1000;

   rlImGuiEnd();
   EndDrawing();
}

#define USE_C_IMG

#ifndef USE_C_IMG
#include <unistd.h>
#include <dirent.h>
#else
#include "../assets/xdoteqAx.c"
#endif

int main(void)
{
   SetConfigFlags(FLAG_WINDOW_RESIZABLE);
   InitWindow(screenwidth, screenheight, "diffeqvisualizer");
   rlImGuiSetup(true);

   {
#ifndef USE_C_IMG
      char cwd[200];
      if (getcwd(cwd, sizeof(cwd)) != NULL)
          printf("Current working dir: %s\n", cwd);

      puts("==== files in the current directory ====");
      DIR *d;
      struct dirent *dir;
      d = opendir(".");
      if (d) {
         while ((dir = readdir(d)) != NULL) {
            printf("%s\n", dir->d_name);
         }
         closedir(d);
      }
      puts("================");

      if (access("assets/", F_OK) != 0)
      {
         puts("assets folder was not found");
      }
      equation_texture = LoadTexture("assets/xdoteqAx.png");

#else
      Image equation_img;
      equation_img.data = xdoteqAx;
      equation_img.width = arrlen(xdoteqAx[0]) / 4;
      equation_img.height = arrlen(xdoteqAx);
      equation_img.mipmaps = 1;
      equation_img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
      showptr(equation_img.data);
      showint(equation_img.width);
      showint(equation_img.height);
      equation_texture = LoadTextureFromImage(equation_img);
#endif
   }

   for (int i = 0; i < numtrajectories; i++)
      initTrajectory(&trajectories[i]);

#ifdef JULIA_BACKEND
   jl_init();
   eval("include(\"source_code/compute.jl\")");

   /* jl_value_t *jlVecF64type = jl_apply_array_type((jl_value_t *) jl_float64_type, 1); */
   jl_value_t *jlMatF64type = jl_apply_array_type((jl_value_t *) jl_float64_type, 2);

   x_jlarr = jl_alloc_array_2d(jlMatF64type, 2, numtrajectories);
   currentstates = (f64 *) jl_array_data(x_jlarr);

   A = jl_alloc_array_2d(jlMatF64type, 2, 2);
   AData = (f64 *) jl_array_data(A);

   JL_GC_PUSH2(&x_jlarr, &A);
   solve_autonomous = getfunc("solve_autonomous");
#else
   AData = A.elems;
#endif
   resetstates((f64 *) currentstates);

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

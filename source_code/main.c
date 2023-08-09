#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <raylib.h>

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

int main(int argc, char **argv)
{
   InitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard input");
   SetTargetFPS(60);

#define histcapacity 20
   Vector2 recentBallPositions[histcapacity] = {0}; // ring buffer
   int curidx = 0;
   int histsize = 0;

   f32 theta = 0;
   f32 radius = 3;

   while (!WindowShouldClose())   // Detect window close button or ESC key
   {
      if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyDown(KEY_W))
         break;

      Vector2 ballPosition = {radius * cosf(theta), radius * sinf(theta)};
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
   }

   CloseWindow();
   return 0;
}

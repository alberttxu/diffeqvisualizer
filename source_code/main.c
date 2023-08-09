#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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


int main(int argc, char **argv)
{
   const int screenWidth = 800;
   const int screenHeight = 450;

   InitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard input");
   SetTargetFPS(60);

   Vector2 ballPosition = { (float)screenWidth/2, (float)screenHeight/2 };

#define histcapacity 20
   Vector2 recentBallPositions[histcapacity] = {0}; // ring buffer
   int curidx = 0;
   int histsize = 0;

   f32 speed = 20;

   while (!WindowShouldClose())   // Detect window close button or ESC key
   {
      if (IsKeyDown(KEY_RIGHT)) ballPosition.x += speed;
      if (IsKeyDown(KEY_LEFT)) ballPosition.x -= speed;
      if (IsKeyDown(KEY_UP)) ballPosition.y -= speed;
      if (IsKeyDown(KEY_DOWN)) ballPosition.y += speed;

      if (ballPosition.x >= screenWidth)
         ballPosition.x -= screenWidth;
      if (ballPosition.x < 0)
         ballPosition.x += screenWidth;
      if (ballPosition.y >= screenHeight)
         ballPosition.y -= screenHeight;
      if (ballPosition.y < 0)
         ballPosition.y += screenHeight;

      BeginDrawing();
      {
         ClearBackground(RAYWHITE);
         DrawText("move the ball with arrow keys", 10, 10, 20, DARKGRAY);

         recentBallPositions[curidx] = ballPosition;

         for (int i = 0; i < histsize; i++)
         {
            f32 radius = 10.0f - 0.5f * i;
            int j = curidx - i;
            if (j < 0)
              j += histcapacity;
            DrawCircleV(recentBallPositions[j], radius, MAROON);
         }
      }
      EndDrawing();

      curidx = (curidx+1) % histcapacity;
      histsize = min(histcapacity, histsize + 1);
   }

   CloseWindow();
   return 0;
}

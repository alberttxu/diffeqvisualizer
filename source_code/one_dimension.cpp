#pragma once

#include <math.h>

void gameloop_onedim()
{
   // draw x axis
   {
      int x0 = screenwidth/2;
      int y0 = screenheight/2;
      DrawLine(0, y0, screenwidth-1, y0, BLACK);
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
      DrawText("0", x0 - 4, y0 + ticklen + 10, 20, DARKGRAY);
   }

   DrawText(TextFormat("Frame time: %02.02f ms", prevframetime_ms), 10, 50, 20, DARKGRAY);
   DrawText(TextFormat("t = %f", t), 10, 30, 20, DARKGRAY);
   DrawTexture(equation_texture, 250, 30, WHITE);

   static f32 A = 0;
   static f32 x_pos = 0;

   DrawCircleV(coords2pixels((Vector2){x_pos, 0}), 5, RED);

   ImGui::Begin("1-D");
   ImGui::SliderFloat("A", &A, -5, 5);
   ImGui::SliderFloat("x", &x_pos, -10, 10);
   ImGui::End();

   x_pos = expf((f32)dt * A) * x_pos;
   t += dt;
}

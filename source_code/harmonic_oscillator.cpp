#pragma once

void gameloop_oscillator()
{
   drawcoordaxes();

   DrawText(TextFormat("Frame time: %02.02f ms", prevframetime_ms), 10, 50, 20, DARKGRAY);
   DrawText(TextFormat("t = %f", t), 10, 30, 20, DARKGRAY);
   DrawText(TextFormat("pixelsperunit = %d", pixelsperunit), 10, 70, 20, DARKGRAY);

   constexpr f32 groundY_pix = 500;
   constexpr f32 wallX_pix = 50;
   constexpr int pixelspermeter = 50;
   constexpr f32 groundwidth_m = 8;
   constexpr f32 wallheight_m = 3;
   constexpr f32 boxsize_m = 1;
   constexpr f32 maxdisp = 0.5f * groundwidth_m - 0.5f * boxsize_m;

   static f32 disp_m = 0;
   static Vec2F64 currentstate = {0, 0};
   static f32 boxMass_kg = 0.5;
   static f32 k_springconstant = 5;
   // helper variables for drawing
   static f32 eq_distfromwall_m = groundwidth_m / 2;
   static f32 box_distfromwall_m = eq_distfromwall_m;

   DrawLineEx(
         (Vector2){wallX_pix, groundY_pix - wallheight_m * pixelspermeter},
         (Vector2){wallX_pix, groundY_pix}, 4, BLACK);
   DrawLineEx(
         (Vector2){wallX_pix, groundY_pix},
         (Vector2){wallX_pix + groundwidth_m * pixelspermeter, groundY_pix}, 4, BLACK);

   ImGui::Begin("Box");

   bool boxwasmoved = ImGui::SliderFloat(
         "equilib. displacement",
         &disp_m,
         -maxdisp,
         maxdisp);
   currentstate.elems[0] = disp_m;
   box_distfromwall_m = eq_distfromwall_m + disp_m;

   bool masswaschanged = ImGui::SliderFloat("mass, kg", &boxMass_kg, 0.01, 5);
   bool kwaschanged = ImGui::SliderFloat("k_spring", &k_springconstant, 0, 20);

   if (boxwasmoved || masswaschanged || kwaschanged)
   {
      paused = true;
      currentstate.elems[1] = 0;
   }

   // draw box
   DrawRectangleLinesEx(
         (Rectangle){
            wallX_pix + (box_distfromwall_m - 0.5f * boxsize_m) * pixelspermeter,
            groundY_pix - boxsize_m * pixelspermeter,
            boxsize_m * pixelspermeter,
            boxsize_m * pixelspermeter},
         4,
         BLUE);

   DrawCircleV(coords2pixels(currentstate), 6, MAROON);

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

   ImGuiIO& io = ImGui::GetIO();
   if (paused)
   {
      t = 0;

      DrawText("Paused", screenwidth - 100, 20, 20, DARKGRAY);
      resumewasclicked = ImGui::Button("resume");
      if (resumewasclicked || (IsKeyPressed(KEY_SPACE) && !io.WantCaptureKeyboard))
         paused = false;
   }
   else
   {
      currentstate = newstate;
      disp_m = newstate.elems[0];
      t += dt;

      pausewasclicked = ImGui::Button("pause");
      if (pausewasclicked || (IsKeyPressed(KEY_SPACE) && !io.WantCaptureKeyboard))
         paused = true;
   }
   ImGui::End();
}

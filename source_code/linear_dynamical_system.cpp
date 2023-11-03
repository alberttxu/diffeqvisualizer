#pragma once

#include "trajectories.cpp"

Mat2x2F64 B;
Vec2F64 u_input;

void step_with_control_input(Mat2x2F64 A, Mat2x2F64 B)
{
   // The upper left block of the matrix exponential of the block matrix
   //      A B
   //      0 0
   // is equal to the matrix exponential of A.
   // https://math.stackexchange.com/questions/658276/integral-of-matrix-exponential/4105683#4105683
   Mat4x4F64 Atilde = BlockMatrix(
         A,       B,
         Zero2x2(), Identity2x2()
   );
   Mat4x4F64 exp_dtAtilde = expm(dt * Atilde);
   Mat2x2F64 dynamicsUpdateMatrix = getUpperLeftBlock(exp_dtAtilde);
   Mat2x2F64 inputUpdateMatrix = getUpperRightBlock(exp_dtAtilde);

   for (int i = 0; i < numtrajectories; i += 1)
   {
      Vec2F64 newstate = matvecmul(dynamicsUpdateMatrix, currentstates[i]);
      newstate = newstate + matvecmul(inputUpdateMatrix, u_input);

      updateposition(&trajectories[i], newstate);
      currentstates[i] = newstate;
   }
}

void gameloop_lineardynamicalsystem()
{
   ImGuiIO& io = ImGui::GetIO();
   if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !io.WantCaptureMouse)
   {
      Vec2F64 newcoords = pixels2coords(GetMousePosition());
      initTrajectory(&trajectories[newtrajidx]);
      addstate(currentstates, newtrajidx, newcoords);
      newtrajidx = (newtrajidx + 1) % numtrajectories;

      time_since_last_spawn = 0;
   }
   else if (!paused && spawn_new_trajectories)
   {
      time_since_last_spawn += dt;

      while (time_since_last_spawn > spawn_period)
      {
         Vec2F64 newcoords = randomcoord();
         initTrajectory(&trajectories[newtrajidx]);
         addstate(currentstates, newtrajidx, newcoords);
         newtrajidx = (newtrajidx + 1) % numtrajectories;

         time_since_last_spawn -= spawn_period;
      }
   }

   if (!paused)
   {
      step_with_control_input(A, B);
   }

   drawcoordaxes();

   DrawText(TextFormat("Frame time: %02.02f ms", drawtime_ms), 10, 50, 20, DARKGRAY);
   DrawText(TextFormat("t = %f", t), 10, 30, 20, DARKGRAY);

   { ZoneScopedN("draw trajectories");
   for (int i = 0; i < numtrajectories; i++)
   {
      Trajectory trajectory = trajectories[i];
      Vector2 points[histcapacity];
      for (int i = 0; i < trajectory.size; i += 1)
      {
         points[i] = coords2pixels(getRecentPos(trajectory, i));
      }
      for (int i = 0; i < trajectory.size - 1; i += 1)
      {
         DrawLineEx(points[i], points[i + 1], 2, MAROON);
      }
   }
   }

   Eigen eigen = decomposition(A);

   // draw the real parts of the eigenvectors
   if (show_eigenvectors)
   {
      float thickness = 3;
      float lenscale = 1000;
      Color v1color = eigen.values[0].rl > 0 ? GREEN : BLUE;
      Color v2color = eigen.values[1].rl > 0 ? GREEN : BLUE;
      DrawLineEx(
         coords2pixels((Vector2){
            (f32) eigen.vectors[0][0].rl * lenscale,
            (f32) eigen.vectors[0][1].rl * lenscale}),
         coords2pixels((Vector2){
            (f32) -eigen.vectors[0][0].rl * lenscale,
            (f32) -eigen.vectors[0][1].rl * lenscale}),
         thickness,
         v1color);
      DrawLineEx(
         coords2pixels((Vector2){
            (f32) eigen.vectors[1][0].rl * lenscale,
            (f32) eigen.vectors[1][1].rl * lenscale}),
         coords2pixels((Vector2){
            (f32) -eigen.vectors[1][0].rl * lenscale,
            (f32) -eigen.vectors[1][1].rl * lenscale}),
         thickness,
         v2color);
   }

   { ZoneScopedN("Post-iteration work");

   ImGui::Begin("Controls");
   resetwasclicked = ImGui::Button("reset");
   ImGui::SameLine();
   if (resetwasclicked)
   {
      t = 0;

      resetstates((f64 *)currentstates);
      for (int i = 0; i < numtrajectories; i++)
         initTrajectory(&trajectories[i]);
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
   ImGui::Checkbox("spawn new trajectories", &spawn_new_trajectories);
   ImGui::Checkbox("show eigenvectors", &show_eigenvectors);

   f32 maxval = 5;
   static f32 newAData[4] = {0, 0, 0, 0}; // row-major order because of ImGui
   ImGui::SliderFloat2("A11, A12", newAData + 0, -maxval, maxval);
   ImGui::SliderFloat2("A21, A22", newAData + 2, -maxval, maxval);
   AData[0] = (f64) newAData[0];
   AData[1] = (f64) newAData[2];
   AData[2] = (f64) newAData[1];
   AData[3] = (f64) newAData[3];

   ImGui::Text("eigenvalues:\n%f + %f i,\n%f + %f i\n",
         eigen.values[0].rl, eigen.values[0].im,
         eigen.values[1].rl, eigen.values[1].im);
   ImGui::Text("eigenvectors:\n[%f + %f i, %f + %f i]\n[%f + %f i, %f + %f i]",
         eigen.vectors[0][0].rl, eigen.vectors[0][0].im,
         eigen.vectors[0][1].rl, eigen.vectors[0][1].im,
         eigen.vectors[1][0].rl, eigen.vectors[1][0].im,
         eigen.vectors[1][1].rl, eigen.vectors[1][1].im);

   static f32 newBData[4] = {0, 0, 0, 0}; // row-major order because of ImGui
   ImGui::SliderFloat2("B11, B12", newBData + 0, -maxval, maxval);
   ImGui::SliderFloat2("B21, B22", newBData + 2, -maxval, maxval);
   B.elems[0] = (f64) newBData[0];
   B.elems[1] = (f64) newBData[2];
   B.elems[2] = (f64) newBData[1];
   B.elems[3] = (f64) newBData[3];

   static f32 _u_input[2] = {0, 0};
   ImGui::SliderFloat2("u1, u2", _u_input, -maxval, maxval);
   u_input.elems[0] = (f64) _u_input[0];
   u_input.elems[1] = (f64) _u_input[1];

   ImGui::End();
   }

   DrawTexture(equation_texture, 250, 30, WHITE);
}

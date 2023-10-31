#pragma once

#define histcapacity 16
struct Trajectory
{
   Vec2F64 recentpositions[histcapacity];
   int curidx;
   int size;
};

void initTrajectory(Trajectory *t)
{
   memset(t->recentpositions, 0, histcapacity * sizeof(t->recentpositions[0]));
   t->curidx = 0;
   t->size = 0;
}

void updateposition(Trajectory *t, Vec2F64 position)
{
   t->recentpositions[t->curidx] = position;
   t->curidx = (t->curidx + 1) % histcapacity;
   t->size = min(histcapacity, t->size + 1);
}

#define numtrajectories 400
Trajectory trajectories[numtrajectories];
int newtrajidx = 0;

#ifdef JULIA_BACKEND
   f64 *currentstates;
   jl_array_t *A;
   jl_array_t *x_jlarr;
   jl_function_t *solve_autonomous;
#else
   Vec2F64 currentstates[numtrajectories];
   Mat2x2F64 A;
#endif

f64 *AData;
f32 newAData[4] = {0, 0, 0, 0}; // row-major order because of ImGui
bool spawn_new_trajectories = true;
bool show_eigenvectors = true;

constexpr f64 trajectory_lifetime_s = 5;
f64 time_since_last_spawn = 0;
constexpr f64 spawn_period = trajectory_lifetime_s / numtrajectories;

#ifdef JULIA_BACKEND
void step(jl_array_t *A)
{
   ZoneScoped;
   jl_value_t *matrix_2xN_newstates = call(solve_autonomous, x_jlarr, A, jl_box_float64(dt));
   JL_GC_PUSH1(&matrix_2xN_newstates);
   f64 *newstates = (f64 *)jl_array_data((jl_array_t *) matrix_2xN_newstates);

   for (int i = 0; i < numtrajectories; i++)
   {
      updateposition(&trajectories[i], *(Vec2F64 *)&newstates[2*i]);
   }
   memcpy(currentstates, newstates, numtrajectories * 2 * sizeof(f64));
   JL_GC_POP();
}
#else

void step(Mat2x2F64 A)
{
   Mat2x2F64 updateMatrix = expm(dt * A);
   for (int i = 0; i < numtrajectories; i += 1)
   {
      Vec2F64 newstate = matvecmul(updateMatrix, currentstates[i]);
      updateposition(&trajectories[i], newstate);
      currentstates[i] = newstate;
   }
}
#endif

Vec2F64 getRecentPos(Trajectory t, int ago)
{
   assert(ago < t.size);
   int idx = t.curidx - 1 - ago;
   if (idx < 0)
      idx += histcapacity;
   return t.recentpositions[idx];
}

Vec2F64 getMostRecentPos(Trajectory t)
{
   assert(t.size > 0);
   return getRecentPos(t, 0);
}

Vec2F64 getLeastRecentPos(Trajectory t)
{
   assert(t.size > 0);
   return getRecentPos(t, t.size - 1);
}


#define boxlim 20.0

static inline
void resetstates(f64 *arr_2xN)
{
   for (int i = 0; i < numtrajectories; i += 1)
   {
      arr_2xN[2*i + 0] = randfloat64(-boxlim, boxlim);
      arr_2xN[2*i + 1] = randfloat64(-boxlim, boxlim);
   }
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

void gameloop_trajectories()
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
         Vec2F64 newcoords = { randfloat64(-boxlim, boxlim), randfloat64(-boxlim, boxlim) };
         initTrajectory(&trajectories[newtrajidx]);
         addstate(currentstates, newtrajidx, newcoords);
         newtrajidx = (newtrajidx + 1) % numtrajectories;

         time_since_last_spawn -= spawn_period;
      }
   }

   if (!paused)
   {
      step(A);
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
   bool A_was_modified[2] = {false, false};
   A_was_modified[0] = ImGui::SliderFloat2("a11, a12", newAData + 0, -maxval, maxval);
   A_was_modified[1] = ImGui::SliderFloat2("a21, a22", newAData + 2, -maxval, maxval);
   if (any(A_was_modified, 2))
   {
      AData[0] = (f64) newAData[0];
      AData[1] = (f64) newAData[2];
      AData[2] = (f64) newAData[1];
      AData[3] = (f64) newAData[3];
   }

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

   DrawTexture(equation_texture, 250, 30, WHITE);
}

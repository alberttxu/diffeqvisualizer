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
bool spawn_new_trajectories = true;
bool show_eigenvectors = true;
bool show_trajeigencomponents = false;

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

void step(Mat2x2F64 myA)
{
   for (int i = 0; i < 4; i++)
   {
      AData[i] = myA.elems[i];
   }
   step(A);
}

#else

void step(Mat2x2F64 A)
{
   Mat2x2F64 dynamicsUpdateMatrix = expm(dt * A);
   for (int i = 0; i < numtrajectories; i += 1)
   {
      Vec2F64 newstate = matvecmul(dynamicsUpdateMatrix, currentstates[i]);

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

static inline
Vec2F64 randomcoord()
{
   f64 xmax = 0.5 * screenwidth / pixelsperunit;
   f64 ymax = 0.5 * screenheight / pixelsperunit;
   return {randfloat64(-xmax, xmax), randfloat64(-ymax, ymax)};
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
         Vec2F64 newcoords = randomcoord();
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
         DrawLineEx(points[i], points[i + 1], 3-0.1f*i, DARKGRAY);
      }
   }
   }

   Eigen eigen = decomposition(A);

   Vec2F64 v1rl = {eigen.vectors[0][0].rl, eigen.vectors[0][1].rl};
   Vec2F64 v2rl = {eigen.vectors[1][0].rl, eigen.vectors[1][1].rl};

   // draw the real parts of the eigenvectors
   if (show_eigenvectors)
   {
      f32 thickness = 3;
      f64 lenscale = 1000;
      Color v1color = eigen.values[0].rl > 0 ? GREEN : BLUE;
      Color v2color = eigen.values[1].rl > 0 ? GREEN : BLUE;
      DrawLineEx(
         coords2pixels(lenscale * v1rl),
         coords2pixels(-lenscale * v1rl),
         thickness,
         v1color);
      DrawLineEx(
         coords2pixels(lenscale * v2rl),
         coords2pixels(-lenscale * v2rl),
         thickness,
         v2color);
   }

   bool eigvals_are_real = eigen.values[0].im == 0 && eigen.values[1].im == 0;
   if (eigvals_are_real && show_trajeigencomponents)
   {
      constexpr int subset = 1;
      for (int i = 0; i < subset; i++)
      {
         Mat2x2F64 V = { v1rl.elems[0], v1rl.elems[1], v2rl.elems[0], v2rl.elems[1] };
         Vec2F64 x = currentstates[i];
         LinsolveResult result = linsolve(V, x);
         if (result.error_occurred)
         {
            continue;
         }

         Vec2F64 eigencoords = result.x;
         Vec2F64 v1comp = eigencoords.elems[0] * v1rl;
         Vec2F64 v2comp = eigencoords.elems[1] * v2rl;
         Color v1color = eigen.values[0].rl > 0 ? GREEN : BLUE;
         Color v2color = eigen.values[1].rl > 0 ? GREEN : BLUE;
         f32 thickness = 2;
         DrawLineEx(
            coords2pixels(x),
            coords2pixels(x - v1comp),
            thickness,
            v1color);
         DrawLineEx(
            coords2pixels(x),
            coords2pixels(x - v2comp),
            thickness,
            v2color);
      }
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
   ImGui::Checkbox("show trajectory eigen components", &show_trajeigencomponents);

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

   ImGui::End();
   }

   DrawTexture(equation_texture, 250, 30, WHITE);
}

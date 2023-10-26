#pragma once

int framenumber = 0;
f64 t = 0;
f64 t_prevframe = 0;
f64 dt = 0;
f64 drawtime_ms = 0;

// UI related
bool paused = false;
bool resetwasclicked = false;
bool pausewasclicked = false;
bool resumewasclicked = false;

Texture2D equation_texture;

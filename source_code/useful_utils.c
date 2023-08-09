#pragma once

#include <assert.h>
// taken from https://github.com/varnishcache/varnish-cache/blob/master/include/vas.h
#define AZ(foo)		do { assert((foo) == 0); } while (0)
#define AN(foo)		do { assert((foo) != 0); } while (0)

// debug macros
#define str(x) #x
#define showint(x) printf(str(x)" = %d\n", x)
#define showuint64(x) printf(str(x)" = %llu\n", x)
#define showfloat(x) printf(str(x)" = %f\n", (double)x)
#define showhex(x) printf(str(x)" = 0x%x\n", x)
#define showptr(x) printf(str(x)" = %p\n", x)
#define showaddr(x) printf("&" str(x)" = %p\n", &x)

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#define min3(a,b,c) min(a, min(b, c))
#define max3(a,b,c) max(a, max(b, c))

void swapint(int *a, int *b)
{
   int temp = *a;
   *a = *b;
   *b = temp;
}

void swapfloat(float *a, float *b)
{
   float temp = *a;
   *a = *b;
   *b = temp;
}

// https://stackoverflow.com/a/16659263/5150450
// double clamp(double d, double min, double max) {
//    const double t = d < min ? min : d;
//    return t > max ? max : t;
// }

float clampfloat(float x, float l, float r) {
   const float t = x < l ? l : x;
   return t > r ? r : t;
}

int clampint(int x, int l, int r) {
   const int t = x < l ? l : x;
   return t > r ? r : t;
}

float average(float *arr, int n)
{
   if (n == 0)
      return 0;

   float avg = 0;
   for (int i = 0; i < n; i++)
      avg += arr[i];
   avg /= n;
   return avg;
}

bool inarraybounds(void *ptr, void *low, void *high)
{
   return ptr >= low && ptr < high;
}

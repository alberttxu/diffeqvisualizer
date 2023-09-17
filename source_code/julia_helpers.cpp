#pragma once

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include <julia.h>

#include "linearalgebra.cpp"

// modified from
// https://blog.esciencecenter.nl/10-examples-of-embedding-julia-in-c-c-66282477e62c
static inline
void check_if_julia_exception_occurred(void)
{
   jl_value_t *ex = jl_exception_occurred();
   if (ex == NULL)
      return;

   jl_printf(jl_stderr_stream(), "Exception:\n");
   jl_call2(
      jl_get_function(jl_base_module, "showerror"),
      jl_stderr_obj(),
      ex
   );
   jl_printf(jl_stderr_stream(), "\n");
   jl_atexit_hook(1);
   exit(1);
}

static inline
jl_value_t *eval(const char* code)
{
   jl_value_t *result = jl_eval_string(code);
   check_if_julia_exception_occurred();
   assert(result && "Missing return value but no exception occurred!");
   return result;
}

static inline
jl_function_t *getfunc(const char *func)
{
   jl_function_t *result = jl_get_function(jl_main_module, func);
   check_if_julia_exception_occurred();
   return result;
}

static inline
jl_value_t *call(jl_function_t *f, void *arg1)
{
   jl_value_t *result = jl_call1(f, (jl_value_t *)arg1);
   check_if_julia_exception_occurred();
   return result;
}

static inline
jl_value_t *call(jl_function_t *f, void *arg1, void *arg2)
{
   jl_value_t *result = jl_call2(f, (jl_value_t *)arg1, (jl_value_t *)arg2);
   check_if_julia_exception_occurred();
   return result;
}

static inline
jl_value_t *call(jl_function_t *f, void *arg1, void *arg2, void *arg3)
{
   jl_value_t *result = jl_call3(f, (jl_value_t *)arg1, (jl_value_t *)arg2, (jl_value_t *)arg3);
   check_if_julia_exception_occurred();
   return result;
}

static inline
Eigen decomposition(jl_array_t *A)
{
   Eigen result;
   jl_function_t *eigen = getfunc("eigen_ComplexF64");
   jl_function_t *getfield = getfunc("getfield");

   jl_value_t *EigenObj = call(eigen, A);
   jl_array_t *eigenvalues = (jl_array_t *) call(getfield, EigenObj, jl_symbol("values"));
   jl_array_t *eigenvectors = (jl_array_t *) call(getfield, EigenObj, jl_symbol("vectors"));

   ComplexF64 *eigenvaluesData = (ComplexF64 *) jl_array_data(eigenvalues);
   ComplexF64 *eigenvectorsData = (ComplexF64 *) jl_array_data(eigenvectors);

   memcpy(&result.values, eigenvaluesData, 2 * sizeof(ComplexF64));
   memcpy(&result.vectors, eigenvectorsData, 4 * sizeof(ComplexF64));
   return result;
}

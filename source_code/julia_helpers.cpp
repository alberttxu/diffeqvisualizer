#pragma once

#include <stdlib.h>
#include <julia.h>

// modified from
// https://blog.esciencecenter.nl/10-examples-of-embedding-julia-in-c-c-66282477e62c
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

jl_value_t *eval(const char* code)
{
   jl_value_t *result = jl_eval_string(code);
   check_if_julia_exception_occurred();
   assert(result && "Missing return value but no exception occurred!");
   return result;
}
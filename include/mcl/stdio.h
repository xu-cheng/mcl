#pragma once

#ifdef SGX_ENCLAVE
#include <cstdarg>

inline int sgx_printf(const char* fmt, ...)
{
  char buffer[512] = {'\0'};
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, 512, fmt, args);
  va_end(args);
  int ret;
  ocall_print(&ret, buffer);
  return ret;
}
#define MCL_PRINTF sgx_printf

#else

#include <cstdio>
#define MCL_PRINTF printf

#endif

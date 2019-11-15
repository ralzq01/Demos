#include <demo.h>

extern "C" {

void _C_API_ThreadInfo(char* msg) {
  ThreadInfo(msg);
}

int _C_API_VerifyParallel(int data) {
  return VerifyParallel(&data);
}

} // C Extern: avoid name mangling
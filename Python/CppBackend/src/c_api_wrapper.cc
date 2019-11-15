#include <demo.h>

extern "C" {

void _C_API_ThreadInfo(char* msg) {
  ThreadInfo(msg);
}

} // C Extern: avoid name mangling
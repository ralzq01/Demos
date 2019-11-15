#include <iostream>

// define from shared library
void ThreadInfo(char* msg);

int main() {
  char* msg = "HelloThread";
  ThreadInfo(msg);
  return 0;
}
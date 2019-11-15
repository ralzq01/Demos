#include <demo.h>
#include <iostream>
#include <thread>

void PrintMsg(char* msg, int thread_id) {
  std::cout << "Thread " << thread_id << ": " 
            << msg << std::endl;
}

void ThreadInfo(char* msg) {
  std::thread t[3];
  for(int i = 0; i < 3; ++i){
    t[i] = std::thread(PrintMsg, msg, i);
  }
  for (int i = 0; i < 3; ++i) {
    if (t[i].joinable()) {
      t[i].join();
    }
  }
  std::cout << "All threads finished." << std::endl; 
}

void SelfAdd(int* data, int times) {
  for(int i = 0; i < times; ++i) {
    *data = *data + 1;
  }
}

int VerifyParallel(int* data) {
  // each thread add data for 10000 times
  // which may cause data race due to parallel
  // property.
  std::thread t[3];
  int orig = *data;
  for(int i = 0; i < 3; ++i) {
    t[i] = std::thread(SelfAdd, data, 10000);
  }
  for(int i = 0; i < 3; ++i) {
    if (t[i].joinable()) {
      t[i].join();
    }
  }
  std::cout << "All threads finished." << std::endl;
  return *data;
}
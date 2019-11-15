from dylib import *

libname = 'lib/libdemo.so'
lib = load_dynamic_lib(libname)


if __name__ == '__main__':

  msg = 'HelloThread'
  c_msg = c_str(msg)
  lib._C_API_ThreadInfo(c_msg)
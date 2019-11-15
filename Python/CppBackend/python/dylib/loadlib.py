import os
from ctypes import *

def load_dynamic_lib(lib_name):
  """
  For Linux: Search $LD_LIBRARY_PATH to find
  needed dynamic libs
  """
  # get LD_LIBRARY_PATH
  dylibs = os.environ['LD_LIBRARY_PATH']
  # search each dylib path
  for dylibpath in dylibs.split(':'):
    dylib = os.path.join(dylibpath, lib_name)
    if os.path.exists(dylib):
      libs = cdll.LoadLibrary(dylib)
      return libs
  # search default path
  libs = cdll.LoadLibrary(lib_name)
  return libs

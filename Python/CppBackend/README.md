## Demo for Python with C++ Backend

### Run

```sh
$ make
python python/demo.py
```

For runing test program:
```sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/abspath/to/CppBackend/lib
./test
```

### Implementation Notes

* C++ Name Mangling

  C++ code compiled with g++ compiler will encounter the name mangling problem after loading the dynamic library at Python end, where you can't get the functions by orginal names which are defined in `.cc` files.

  To avoid this, it's better to provide a `C`-style function (no namespace, class memthod, etc.) inside `extern "C" {}` to tell the compiler (e.g. g++) not to use the name mangling. See the details at `src/c_api_wrapper.cc`

* Python ctypes

  Details are [here](https://docs.python.org/3.7/library/ctypes.html#module-ctypes)
# Low
No fuss, just systems programming.

You need a recent version of the LLVM libraries and support programs in order to build the Low compiler. Use `lowc simple.low` to compile the program from Low to LLVM IR, then use `lli simple.ll` to run the program using the LLVM interpreter.

Currently still missing from the language:

- unions
- enums
- function pointer calling
- cool literals
- [integer literals](https://github.com/golang/go/issues/12711#issuecomment-142338246)
- switch
- defer
- methods, interfaces, type switch
- multiple return values
- memory alloc/free
- threadlocal global variables

Refer to [1] and [2] for the C grammar and lexical analysis specification. Refer to [3] for the LLVM IR specification, and [4] and [5] for the LLVM C API documentation. Refer to [6] for details on how `defer`, `panic`, and `recover` interact in the Go language.
To get an overview of the LLVM C/C++ API `readelf -Ws /usr/lib/libLLVM.so` can be used in case the online version is not helpful.

[1]: http://www.quut.com/c/ANSI-C-grammar-y-2011.html
[2]: http://www.quut.com/c/ANSI-C-grammar-l-2011.html
[3]: http://llvm.org/docs/LangRef.html
[4]: http://llvm.org/doxygen/
[5]: http://llvm.org/docs/doxygen/html/group__LLVMCCoreInstructionBuilder.html
[6]: http://blog.golang.org/defer-panic-and-recover

# Setup

## Ubuntu trusty
Install dependencies:
```
sudo apt-get install build-essentials llvm-3.6 llvm-3.6-dev zlib1g-dev libedit-dev cmake
```

## Fedora 23
Fetch dependencies:
```
# Install build tools, Note: this will pull a lot of packages,
# you may install them one by one, which ones is left as an exercise for the reader
sudo dnf groupinstall "Development Tools" "Development Libraries"

# Install build system and llvm dependencies
sudo dnf install cmake llvm llvm-devel
```

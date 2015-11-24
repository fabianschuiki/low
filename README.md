# Low
No fuss, just systems programming.

You need a recent version of the LLVM libraries and support programs in order to build the Low compiler. Use `lowc simple.low` to compile the program from Low to LLVM IR, then use `lli simple.ll` to run the program using the LLVM interpreter.

Currently still missing from the language:

- unions
- enums
- typed and untyped constants
- function pointer calling
- switch
- defer
- multiple return values
- memory alloc/free
- proper error reporting (everything is an assert at the moment)

Refer to [1] and [2] for the C grammar and lexical analysis specification. Refer to [3] for the LLVM IR specification, and [4] for the LLVM C API documentation. Refer to [5] for details on how `defer`, `panic`, and `recover` interact in the Go language.

[1]: http://www.quut.com/c/ANSI-C-grammar-y-2011.html
[2]: http://www.quut.com/c/ANSI-C-grammar-l-2011.html
[3]: http://llvm.org/docs/LangRef.html
[4]: http://llvm.org/doxygen/
[5]: http://blog.golang.org/defer-panic-and-recover

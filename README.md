# Low
No fuss, just systems programming.

An evolution of the C language, designed to give the programmer more power over the underlying hardware. Cleaner, more expressive syntax, declaration order independence, no lossy implicit casts, proper bitfields, and fast builds through dependency management by the `lowc` compiler. If you write kernels or embedded code in C, you'll love **Low**.

[![Build Status](https://travis-ci.org/fabianschuiki/low.svg)](https://travis-ci.org/fabianschuiki/low)

Currently still missing from the language:

- unions
- enums
- cool literals
- switch
- defer
- methods, interfaces, type switch
- multiple return values
- threadlocal global variables

Refer to [1] and [2] for the C grammar and lexical analysis specification. Refer to [3] for the LLVM IR specification, and [4] and [5] for the LLVM C API documentation. Refer to [6] for details on how `defer`, `panic`, and `recover` interact in the Go language.

To get an overview of the LLVM C/C++ API `readelf -Ws /usr/lib/libLLVM.so` can be used in case the online version is not helpful.

[1]: http://www.quut.com/c/ANSI-C-grammar-y-2011.html
[2]: http://www.quut.com/c/ANSI-C-grammar-l-2011.html
[3]: http://llvm.org/docs/LangRef.html
[4]: http://llvm.org/doxygen/
[5]: http://llvm.org/docs/doxygen/html/group__LLVMCCoreInstructionBuilder.html
[6]: http://blog.golang.org/defer-panic-and-recover
[7]: https://github.com/golang/go/issues/12711#issuecomment-142338246



# Building

You might have to install LLVM and its dependencies first. Read on for a guide on that. Once you're setup, build the project as follows:

	# for hackers and contributors
	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE=debug ..
	make

	# for package maintainers
	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE=release ..
	make
	make install

Currently, the Low compiler only produces LLVM IR rather than native machine code. To build and run a program written in Low, do the following:

	lowc foo.low
	lli foo.ll

Refer to `examples/deps` for an example on how build Low files into a library and use a Makefile and the LLVM linker to package things into a final executable.


### Ubuntu 14.04 LTS (Trusty Tahr)

	sudo apt-get install build-essentials llvm-3.6 llvm-3.6-dev zlib1g-dev libedit-dev cmake

### Fedora 23

	# Install build tools, Note: this will pull a lot of packages,
	# you may install them one by one, which ones is left as an exercise for the reader
	sudo dnf groupinstall "Development Tools" "Development Libraries"

	# Install build system and llvm dependencies
	sudo dnf install cmake llvm llvm-devel

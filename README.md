# Simple-Compiler

The Simple Compiler is a cross-compiler written in C++, using a single pass to compile the source code into ARM64 assembly. This project is loosely based on the [Teeny Tiny Compiler](https://austinhenley.com/blog/teenytinycompiler1.html) by Austin Henley, which is the basis of my lexing and parsing. However, I have expanded on functionality, including function declarations and other operators. Also, the Teeny Tiny compiler compiles into C, where as this compiler output ARM64.

This compiler can be broken up into 3 sections - lexing, parsing, and emitting. The lexer goes through the source file, character by character, to determine what each token actually is. After the code is tokenized, the parser finds the relations between these tokens - throwing syntax errors if necessary. The emitter simply keeps track of all the compiled code and data, organizing them and writing the results to a file.

## Lexing

The [lexer.h](/src/lexer.h)

## Parsing
## Emitting

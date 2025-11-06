# Simple-Compiler

The Simple Compiler is a cross-compiler written in C, with the aim of performing multiple passes over the source code. First the source code will be tokenized and checked for simple syntax errors. Then the syntax will be broken up into a abstract syntax tree (AST), which has many purposes. It will help to understand the flow of a program, and can find more complex syntax errors. Finally, the resulting tree will be interpreted into another language, ARM64 assembly.

# Simple-Compiler

The Simple Compiler is a cross-compiler written in C++, using a single pass to compile the source code into ARM64 assembly. This project is loosely based on the [Teeny Tiny Compiler](https://austinhenley.com/blog/teenytinycompiler1.html) by Austin Henley, which is the basis of my lexing and parsing. However, I have expanded on functionality, including function declarations and other operators. Also, the Teeny Tiny compiler compiles into C, where as this compiler output ARM64.

This compiler can be broken up into 3 sections - lexing, parsing, and emitting. The lexer goes through the source file, character by character, to determine what each token actually is. After the code is tokenized, the parser finds the relations between these tokens - throwing syntax errors if necessary. The emitter simply keeps track of all the compiled code and data, organizing them and writing the results to a file.

---

## Lexing

The [lexer.h](/src/lexer.h) file is used to tokenize the source file, so most importantly is the token definition. This enumarator, TOKEN_TYPE, defines every acceptable within the syntax of this language. There are various keywords, operators, and tokens to help read files. The lexer.h header file is used by parser.h, meaning the compilation of the code depends on these tokens.

```c++
enum TOKEN_TYPE : int {
        // File parts, and constants
        INVALID = -2,
        END = -1,
        NEWLINE = 0,
        NUMBER,
        STRING,
        IDENTIFIER,
        // Keywords
        INT = 101,
        FLOAT,
        TEXT,
        IF,
        THEN,
        ENDIF,
        WHILE,
        DO,
        ENDWHILE,
        FUNC,
        IS,
        ENDFUNC,
        PRINT,
        LABEL,
        GOTO,
        // Operators
        EQ = 201,
        PLUS,
        MINUS,
        ASTERISK,
        SLASH,
        MODULO,
        GT,
        LT,
        GTEQ,
        LTEQ,
        EQEQ,
        NEQ,
        COMMENT // #
};

```

The actual work done in the file is within the Lexer class. There are various helper functions, like nextChar() and peek() which move onto the next character and lookahead to the next character respectively. There is an abort function to exit the program if there is a lexing error, such as a keyword being misspelled or an undefined operator. The most important class method is getToken(), which finds the next acceptable token in the source code.
```c++
class Lexer {
        public:
                Lexer(string input);
                Lexer();
                void nextChar();
                char peek();
                Token getToken();
                void abort(string message);
                void skipWhitespace();
                void skipComment();

                string source;
                int curPos;
                char curChar;
};
```
```c++
Token Lexer::getToken() {
        skipWhitespace();
        skipComment();
        Token curToken(string(1,'\0'), TOKEN_TYPE::END);

        // get operators
        if (curChar == '+') {
                curToken = Token(string(1, curChar), TOKEN_TYPE::PLUS);
        } else if (curChar == '-') {
                curToken = Token(string(1, curChar), TOKEN_TYPE::MINUS);
        } else if (curChar == '*') {
                curToken = Token(string(1, curChar), TOKEN_TYPE::ASTERISK);
        } else if (curChar == '/') {
                curToken = Token(string(1, curChar), TOKEN_TYPE::SLASH);
        } else if (curChar == '%') {
                curToken = Token(string(1, curChar), TOKEN_TYPE::MODULO);
        } else if (curChar == '!') {
                // ! must be followed by = to be an operator
                // abort if it's not there
        } else if (curChar == '=') {
                // needs to distinguise assignment (=) and comparison (==)
                // must look past the current character to see if there's another =
        } else if (curChar == '>') {
                // checks if the next character is '='
                // if so its less than or equal, other it's just less than
        } else if (curChar == '<') {
                // same as greater than, just looks for '<' symbol
        } else if (curChar == '\n') {
                // detects a new line, an important part of the syntax
        } else if (curChar == '\0') {
                // this means its the end of the file
        } else if (curChar == '\"') { // detects strings. goes from first quote to next quote and find the substring
                // just continues until the next " is found
                // detects for certain characters like \n \t and \r, which are forbidden
        } else if (isdigit(curChar)) { // check for number, same way as string. need to also check for decimal point
                // reads all numeric characters, sometimes followed by a decimal and more numerals
        } else if (isalpha(curChar)) { // checks for identifiers and keywords. needs to start with letter and be alphanumeric
                // continues reading until a non alphabetic character is found
        } else {
                abort("Unknown token: " + curToken.text + "\n");
        }

        nextChar();
        return curToken;
}
```

## Parsing
## Emitting
The emitter is the simplest component of the compiler, and is mainly controlled by the parser. After the parser determines the function of a line of code, the parser tells the emitter to produce a corresponding line (or in my case many lines) of code. Again, the [emitter.h](/src/emitter.h) file is simply a class, Emitter, which controls all functionality. ARM64 code has a particaular structure which can be seen below.

```asm
.global _start
.text
_start:

;code goes here

.data

;variables and memory go here
```

Due to this, the emitter must keep track of three things, code to executed, any variables and string literals to go in the .data sections, and any defined functions. As such, there are class methods to write to each of the sections individually. At the end, the emitter calls writeFile(), and all of the code is written to the output file.

```c++
class Emitter {
        public:
                Emitter(string filePath);
                Emitter();
                void emit(string codeIn);
                void emitLine(string codeIn);
                void headerLine(string codeIn);
                void dataLine(string codeIn);
                void functionLine(string codeIn);
                void writeFile();
                void abort(string message);

                string path;
                string header;
                string code;
                string functions;
                string data;
};
```

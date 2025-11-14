#include <iostream>
#include <string>
#include <cstdlib>
#include <cctype>

using namespace std;

#ifndef LEXER_H
#define LEXER_H

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
	ELSE,
	ENDIF,
	WHILE,
	DO,
	ENDWHILE,
	FUNC,
	IS,
	USING,
	WITH,
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
	COMMA,
	COMMENT // #
};

string tokenTypeToString(TOKEN_TYPE type) {
	switch(type) {
		// LITERALS
		case END: return "END";
		case NEWLINE: return "NEWLINE";
		case NUMBER: return "NUMBER";
		case STRING: return "STRING";
		case IDENTIFIER: return "IDENTIFIER";
		// KEYWORDS
		case INT: return "INT";
		case FLOAT: return "FLOAT";
		case TEXT: return "TEXT";

		case IF: return "IF";
		case THEN: return "THEN";
		case ELSE: return "ELSE";
		case ENDIF: return "ENDIF";

		case WHILE: return "WHILE";
		case DO: return "DO";
		case ENDWHILE: return "ENDWHILE";

		case FUNC: return "FUNC";
		case IS: return "IS";
		case USING: return "USING";
		case WITH: return "WITH";
		case ENDFUNC: return "ENDFUNC";

		case PRINT: return "PRINT";
		case LABEL: return "LABEL";
		case GOTO: return "GOTO";
		// OPERATORS
		case EQ: return "EQ";
		case PLUS: return "PLUS";
		case MINUS: return "MINUS";
		case ASTERISK: return "ASTERISK";
		case SLASH: return "SLASH";
		case MODULO: return "MODULO";
		case GT: return "GT";
		case LT: return "LT";
		case GTEQ: return "GTEQ";
		case LTEQ: return "LTEQ";
		case EQEQ: return "EQEQ";
		case NEQ: return "NEQ";
		case COMMENT: return "COMMENT";
		case COMMA: return "COMMA";

		default: return "INVALID";
	}
}

class Token {
	public:
		string text;
		TOKEN_TYPE type;

		Token() {
			text = "INVALID";
			type = INVALID;
		}

		Token(string tokenText, TOKEN_TYPE tokenType) {
			text = tokenText;
			type = tokenType;
		}

		TOKEN_TYPE checkIfKeyword(string tokenText) {
			if (tokenText == "INT") return INT;
			else if (tokenText == "FLOAT") return FLOAT;
			else if (tokenText == "TEXT") return TEXT;
			else if (tokenText == "IF") return IF;
			else if (tokenText == "THEN") return THEN;
			else if (tokenText == "ELSE") return ELSE;
			else if (tokenText == "ENDIF") return ENDIF;
			else if (tokenText == "WHILE") return WHILE;
			else if (tokenText == "DO") return DO;
			else if (tokenText == "ENDWHILE") return ENDWHILE;
			else if (tokenText == "LABEL") return LABEL;
			else if (tokenText == "GOTO") return GOTO;
			else if (tokenText == "PRINT") return PRINT;
			else if (tokenText == "FUNC") return FUNC;
			else if (tokenText == "IS") return IS;
			else if (tokenText == "WITH") return WITH;
			else if (tokenText == "USING") return USING;
			else if (tokenText == "ENDFUNC") return ENDFUNC;
			else return INVALID;
		}
};

// Lexer object :
/*

*/
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

void Lexer::abort(string message) {
        cerr << message << endl;
        exit(1);
}

void Lexer::nextChar() {
        curPos += 1;
        if (curPos >= source.length()) { // end of line
                curChar = '\0';
        } else { // not done, get next
                curChar = source[curPos];
        }
}

Lexer::Lexer() {
	source = "\n";
	curPos = -1;
	curChar = '\0';
	nextChar();
}

Lexer::Lexer(string input) {
	source = input + '\n';
	curPos = -1;
	curChar = '\0';
	nextChar();
}

void Lexer::skipWhitespace() {
	while (curChar == ' ' || curChar == '\t' || curChar == '\r') nextChar();
}

void Lexer::skipComment() {
	if (curChar == '#') {
		while (curChar != '\n') nextChar();
	}
}

char Lexer::peek() {
	if (curPos + 1 >= source.length()) {
		return '\0';
	} else {
		return source[curPos + 1];
	}
}

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
	} else if (curChar == ',') {
		curToken = Token(string(1, curChar), TOKEN_TYPE::COMMA);
	} else if (curChar == '!') {
		if (peek() == '=') {
			char lastChar = curChar;
			nextChar();

			string concat = string(1,lastChar) = string(1, curChar);
			curToken = Token(concat, TOKEN_TYPE::NEQ);
		} else {
			abort("Expected !=, got !" + string(1, peek()));
		}
	} else if (curChar == '=') {
		if (peek() == '=') {
			char lastChar = curChar;
			nextChar();
			string concat =  string(1, lastChar) + string(1, curChar);
			curToken = Token(concat, TOKEN_TYPE::EQEQ);
		} else {
			curToken = Token(string(1, curChar), TOKEN_TYPE::EQ);
		}
	} else if (curChar == '>') {
		if (peek() == '=') {
			char lastChar = curChar;
			nextChar();

			string concat = string(1, lastChar) + string(1, curChar);
			curToken = Token(concat, TOKEN_TYPE::GTEQ);
		} else {
			curToken = Token(string(1, curChar), TOKEN_TYPE::GT);
		}
	} else if (curChar == '<') {
		if (peek() == '=') {
			char lastChar = curChar;
			nextChar();
			string concat = string(1, lastChar) + string(1, curChar);
			curToken = Token(concat, TOKEN_TYPE::LTEQ);
		} else {
			curToken = Token(string(1, curChar), TOKEN_TYPE::LT);
		}
	} else if (curChar == '\n') {
		curToken = Token(string(1, curChar), TOKEN_TYPE::NEWLINE);
	} else if (curChar == '\0') {
		curToken = Token(string(1, curChar), TOKEN_TYPE::END);
	} else if (curChar == '\"') { // detects strings. goes from first quote to next quote and find the substring
		nextChar();
		int startPos = curPos;

		while (curChar != '\"') {
			if (curChar == '\r' || curChar == '\t' || curChar == '\n') {
				abort("Forbidden character in string");
			}
			nextChar();
		}

		curToken.text = source.substr(startPos, curPos - startPos);
		curToken.type = TOKEN_TYPE::STRING;
	} else if (isdigit(curChar)) { // check for number, same way as string. need to also check for decimal point
		int startPos = curPos;
		while (isdigit(peek())) {
			nextChar();
		}

		if (peek() == '.') {
			nextChar();
			if (!isdigit(peek())) {
				abort("Forbidden character in number");
			}

			while (isdigit(peek())) {
				nextChar();
			}
		}

		curToken = Token(source.substr(startPos, curPos - startPos + 1), TOKEN_TYPE::NUMBER);
	} else if (isalpha(curChar)) { // checks for identifiers and keywords. needs to start with letter and be alphanumeric
		int startPos = curPos;
		while (isalnum(peek())) {
			nextChar();
		}

		curToken.text = source.substr(startPos, curPos - startPos + 1);
		TOKEN_TYPE keyword = curToken.checkIfKeyword(curToken.text);
		if (keyword == INVALID) {
			curToken.type = TOKEN_TYPE::IDENTIFIER;
		} else {
			curToken.type = keyword;
		}
	} else {
		abort("Unknown token: " + curToken.text + "\n");
	}

	nextChar();
	return curToken;
}

#endif

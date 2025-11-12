#include "lexer.h"
#include "emitter.h"
#include <vector>
#include <algorithm>

using namespace std;

class SymbolMap {
	public:
		SymbolMap() {
		}

		string getLabel(string name) {
			int index = find(symbols.begin(), symbols.end(), name) - symbols.begin();

			return "V" + to_string(index);
		}

		string getLabel(int index) {
			return "V" + to_string(index);
		}

		int exists(string name) {
			if (find(symbols.begin(), symbols.end(), name) == symbols.end()) return 0; // doesn't exist
			else return 1; // exists
		}

		void push_back(string name) {
			symbols.push_back(name);
		}

		int size() {
			return symbols.size();
		}

		vector<string> symbols;
};

class Parser {
	public:
		Parser(Lexer& inputLexer, Emitter& inputEmitter);
		void abort(string message);
		int checkToken(TOKEN_TYPE kind);
		int checkPeek(TOKEN_TYPE kind);
		void nextToken();
		void match(TOKEN_TYPE kind);
		// Sytanx function declarations
		void program();
		void statement(TOKEN_TYPE caller = TOKEN_TYPE::INVALID);
		void nl();
		void expression(TOKEN_TYPE caller = TOKEN_TYPE::INVALID);
		void term(TOKEN_TYPE caller = TOKEN_TYPE::INVALID);
		void unary(TOKEN_TYPE caller = TOKEN_TYPE::INVALID);
		void primary(TOKEN_TYPE caller = TOKEN_TYPE::INVALID);
		void condition(string exitLabel, TOKEN_TYPE caller = TOKEN_TYPE::INVALID);

		Lexer& lexer;
		Emitter& emitter;
		Token curToken;
		Token peekToken;

		SymbolMap symbolMap;
		vector <string> labels;
		vector <string> gotos;
		vector <string> stringLiterals;
		vector <string> functions;
		vector <string> functionsUsed;

		int ifCount;
		int whileCount;

//		vector<int> registerFile(32, 0);
};

Parser::Parser(Lexer& inputLexer, Emitter& inputEmitter) : lexer(inputLexer), emitter(inputEmitter) {
	ifCount = 0;
	whileCount = 0;

	nextToken();
	nextToken();
}

// Returns 1 when the current token matches the expected type
int Parser::checkToken(TOKEN_TYPE kind) {
	if (kind == curToken.type) return 1;
	else return 0;
}

// Returns 1 when the peeked token matches the expected type
int Parser::checkPeek(TOKEN_TYPE kind) {
	if (kind == peekToken.type) return 1;
	else return 0;
}

// Gets the next token for peeked, moves the peeked value to the cur value
void Parser::nextToken() {
	curToken = peekToken;
	peekToken = lexer.getToken();
}

// Exit message
void Parser::abort(string message) {
	cerr << "Error (PARSER):\n";
	cerr << message << endl;
	exit(1);
}

// Combines nextToken and checkToken, and aborts on failure
void Parser::match(TOKEN_TYPE kind) {
	if (checkToken(kind) != 1) {
		abort("Expected " + tokenTypeToString(kind) + ", got " + tokenTypeToString(curToken.type));
	}
	nextToken();
}

// --------------- SYNTAX FUNCTIONS

// Program is made of statements. Do each one until you reach the end
void Parser::program() {
	cout << "PROGRAM\n";

	emitter.headerLine(".global _start");
	emitter.headerLine(".text");
	emitter.headerLine("\n_start:");

	while (checkToken(TOKEN_TYPE::NEWLINE) == 1) nextToken();

	while (checkToken(TOKEN_TYPE::END) != 1) {
		statement();
	}

	for (int i = 0; i < gotos.size(); i++) {
		if (find(labels.begin(), labels.end(), gotos[i]) == labels.end()) {
			abort("Attemping to GOTO undeclared label, " + gotos[i]);
		}
	}

	for (int i = 0; i < symbolMap.size(); i++) {
		emitter.dataLine(symbolMap.getLabel(i) + ": .quad 0");
	}

	for (int i = 0; i < stringLiterals.size(); i++) {
		string label = "S" + to_string(i);
		emitter.dataLine(label + ": .asciz \"" + stringLiterals[i] + "\"");
		emitter.dataLine(label + "_len = . - " + label);
	}

	emitter.emitLine("mov x8, #93");
	emitter.emitLine("mov x0, #0");
	emitter.emitLine("svc #0");
}

void Parser::statement(TOKEN_TYPE caller) {
	// Print statement
	if (caller == TOKEN_TYPE::FUNC) {	
		if (checkToken(TOKEN_TYPE::PRINT)) { // Should be PRINT - STRING | EXPRESSION - NL
			cout << "FUNC-STATEMENT-PRINT\n";
			nextToken();

			if (checkToken(TOKEN_TYPE::STRING)) { // String is for a literal, text is keyword to define variable
				// check literals table for copy, add it if not.
				if (find(stringLiterals.begin(), stringLiterals.end(), curToken.text) == stringLiterals.end()) stringLiterals.push_back(curToken.text);

				int index = find(stringLiterals.begin(), stringLiterals.end(), curToken.text) - stringLiterals.begin();

				emitter.functionLine("mov x0, #1");
				emitter.functionLine("adr x1, S" + to_string(index));
				emitter.functionLine("ldr x2, =S" + to_string(index) + "_len");
				emitter.functionLine("mov x8, #64");
				emitter.functionLine("svc #0");

				nextToken();
			} else {
				abort("Print expression not yet implemented");
				expression(caller);
			}
		} else if (checkToken(TOKEN_TYPE::IF)) { // IF condition THEN statement ENDIF
			cout << "FUNC-STATEMENT-IF\n";
			nextToken();
			condition("XIF" + to_string(ifCount), caller);

			match(TOKEN_TYPE::THEN);
			nl();

			while (checkToken(TOKEN_TYPE::ENDIF) == 0) {
				statement(caller);
			}
			match(TOKEN_TYPE::ENDIF);
			
			emitter.functionLine("XIF" + to_string(ifCount) + ":");
			ifCount++;

		} else if (checkToken(TOKEN_TYPE::WHILE)) { // WHILE condition DO statement ENDWHILE
			cout << "FUNC-STATEMENT-WHILE\n";
			nextToken();
			emitter.functionLine("SWHILE" + to_string(whileCount) + ":");

			condition("XWHILE" + to_string(whileCount), caller);
			
			match(TOKEN_TYPE::DO);
			nl();

			while (checkToken(TOKEN_TYPE::ENDWHILE) == 0) {
				statement(caller);
			}
			match(TOKEN_TYPE::ENDWHILE);
			
			emitter.functionLine("B SWHILE" + to_string(whileCount)); 
			emitter.functionLine("XWHILE" + to_string(whileCount) + ":");
			whileCount++;

		} else if (checkToken(TOKEN_TYPE::FUNC)) { // FUNC identifier IS nl {statement} ENDFUNC nl
			abort("Cannot define a function inside of a function");
		} else if (checkToken(TOKEN_TYPE::LABEL)) { // LABEL identifier
			abort("Cannot put a label inside a function");
		} else if (checkToken(TOKEN_TYPE::GOTO)) { // GOTO identifier
			cout << "FUNC-STATEMENT-GOTO\n";
			nextToken();
			gotos.push_back(curToken.text); // add to the GOTOs list

			emitter.functionLine("b L" + curToken.text);
			match(TOKEN_TYPE::IDENTIFIER);
		} else if (checkToken(TOKEN_TYPE::INT)) { // INT identifier = expression
			cout << "FUNC-STATEMENT-INT\n";
			nextToken();

			if (symbolMap.exists(curToken.text)) {
				abort("Symbol (" + curToken.text + ") is already declared.");
			}

			symbolMap.push_back(curToken.text);
			string identLabel = symbolMap.getLabel(curToken.text);

			match(TOKEN_TYPE::IDENTIFIER);
			match(TOKEN_TYPE::EQ);

			expression(caller);

			emitter.functionLine("adr x13, " + identLabel);
			emitter.functionLine("str x11, [x13]");

		} else if (checkToken(TOKEN_TYPE::FLOAT)) { // FLOAT identifier = expression
			cout << "FUNC-STATEMENT-FLOAT\n";
			nextToken();

			if (symbolMap.exists(curToken.text)) {
				abort("Symbol (" + curToken.text + ") is already declared.");
			} else {
				symbolMap.push_back(curToken.text);
			}
			string identLabel = symbolMap.getLabel(curToken.text);

			match(TOKEN_TYPE::IDENTIFIER);
			match(TOKEN_TYPE::EQ);

			expression(caller);

			emitter.functionLine("adr x13, " + identLabel);
			emitter.functionLine("str x11, [x13]");

		} else if (checkToken(TOKEN_TYPE::TEXT)) { // TEXT identifier = expression
			cout << "FUNC-STATEMENT-TEXT\n";
			nextToken();

			if (symbolMap.exists(curToken.text)) {
				abort("Symbol (" + curToken.text + ") is already declared.");
			} else {
				symbolMap.push_back(curToken.text);
			}

			string identLabel = symbolMap.getLabel(curToken.text);

			match(TOKEN_TYPE::IDENTIFIER);
			match(TOKEN_TYPE::EQ);

			expression(caller);

			emitter.functionLine("adr x13, " + identLabel);
			emitter.functionLine("str x10, [x13]");

		} else if (checkToken(TOKEN_TYPE::IDENTIFIER)) {// identifier "=" expression
			cout << "FUNC-STATEMENT-ASSIGN\n";

			if (!symbolMap.exists(curToken.text)) {
				abort("Symbol (" + curToken.text + ") does not exist.");
			}

			string identLabel = symbolMap.getLabel(curToken.text);
			nextToken();

			match(TOKEN_TYPE::EQ);

			expression(caller);

			emitter.functionLine("adr x13, " + identLabel);
			emitter.functionLine("str x11, [x13]");
		
		} else {
			abort("Invalid state at " + string(curToken.text) + " (" + tokenTypeToString(curToken.type) + ").");
		}
	} else {
		if (checkToken(TOKEN_TYPE::PRINT)) { // Should be PRINT - STRING | EXPRESSION - NL
			cout << "STATEMENT-PRINT\n";
			nextToken();

			if (checkToken(TOKEN_TYPE::STRING)) { // String is for a literal, text is keyword to define variable
				// check literals table for copy, add it if not.
				if (find(stringLiterals.begin(), stringLiterals.end(), curToken.text) == stringLiterals.end()) stringLiterals.push_back(curToken.text);

				int index = find(stringLiterals.begin(), stringLiterals.end(), curToken.text) - stringLiterals.begin();

				emitter.emitLine("mov x0, #1");
				emitter.emitLine("adr x1, S" + to_string(index));
				emitter.emitLine("ldr x2, =S" + to_string(index) + "_len");
				emitter.emitLine("mov x8, #64");
				emitter.emitLine("svc #0");

				nextToken();
			} else {
				abort("Print expression not yet implemented");
				expression(caller);
			}
		} else if (checkToken(TOKEN_TYPE::IF)) { // IF condition THEN statement ENDIF
			cout << "STATEMENT-IF\n";
			nextToken();
			condition("XIF" + to_string(ifCount), caller);

			match(TOKEN_TYPE::THEN);
			nl();

			while (checkToken(TOKEN_TYPE::ENDIF) == 0) {
				statement(caller);
			}
			match(TOKEN_TYPE::ENDIF);
			
			emitter.emitLine("XIF" + to_string(ifCount) + ":");
			ifCount++;

		} else if (checkToken(TOKEN_TYPE::WHILE)) { // WHILE condition DO statement ENDWHILE
			cout << "STATEMENT-WHILE\n";
			nextToken();
			emitter.emitLine("SWHILE" + to_string(whileCount) + ":");

			condition("XWHILE" + to_string(whileCount), caller);
			
			match(TOKEN_TYPE::DO);
			nl();

			while (checkToken(TOKEN_TYPE::ENDWHILE) == 0) {
				statement(caller);
			}
			match(TOKEN_TYPE::ENDWHILE);
			
			emitter.emitLine("B SWHILE" + to_string(whileCount)); 
			emitter.emitLine("XWHILE" + to_string(whileCount) + ":");
			whileCount++;

		} else if (checkToken(TOKEN_TYPE::FUNC)) { // FUNC identifier IS nl {statement} ENDFUNC nl
			cout << "STATEMENT-FUNCTION\n";
			nextToken();

			if (find(functions.begin(), functions.end(), curToken.text) != functions.end()) {
				abort("Function (" + curToken.text + ") already exists");
			}

			functions.push_back(curToken.text);

			string bLabel = "FUNC" + to_string(find(functions.begin(), functions.end(), curToken.text) - functions.begin());
			emitter.functionLine(bLabel + ":");

			nextToken();
			match(TOKEN_TYPE::IS);
			nl();

			while (!checkToken(TOKEN_TYPE::ENDFUNC)) {
				statement(TOKEN_TYPE::FUNC);
			}

			match(TOKEN_TYPE::ENDFUNC);
			emitter.functionLine("br lr");

		} else if (checkToken(TOKEN_TYPE::LABEL)) { // LABEL identifier
			cout << "STATEMENT-LABEL\n";
			nextToken();

			if (find(labels.begin(), labels.end(), curToken.text) != labels.end()) { // element exists if != to the end of labels
				abort("Label (" + curToken.text + ") already exists"); 
			}
			labels.push_back(curToken.text);

			emitter.emitLine("L" + curToken.text + ":");
			match(TOKEN_TYPE::IDENTIFIER);
		} else if (checkToken(TOKEN_TYPE::GOTO)) { // GOTO identifier
			cout << "STATEMENT-GOTO\n";
			nextToken();

			gotos.push_back(curToken.text); // add to the GOTOs list

			emitter.emitLine("b L" + curToken.text);
			match(TOKEN_TYPE::IDENTIFIER);
		} else if (checkToken(TOKEN_TYPE::INT)) { // INT identifier = expression
			cout << "STATEMENT-INT\n";
			nextToken();

			if (symbolMap.exists(curToken.text)) {
				abort("Symbol (" + curToken.text + ") is already declared.");
			}

			symbolMap.push_back(curToken.text);
			string identLabel = symbolMap.getLabel(curToken.text);

			match(TOKEN_TYPE::IDENTIFIER);
			match(TOKEN_TYPE::EQ);

			expression(caller);

			emitter.emitLine("adr x13, " + identLabel);
			emitter.emitLine("str x11, [x13]");

		} else if (checkToken(TOKEN_TYPE::FLOAT)) { // FLOAT identifier = expression
			cout << "STATEMENT-FLOAT\n";
			nextToken();

			if (symbolMap.exists(curToken.text)) {
				abort("Symbol (" + curToken.text + ") is already declared.");
			} else {
				symbolMap.push_back(curToken.text);
			}
			string identLabel = symbolMap.getLabel(curToken.text);

			match(TOKEN_TYPE::IDENTIFIER);
			match(TOKEN_TYPE::EQ);

			expression(caller);

			emitter.emitLine("adr x13, " + identLabel);
			emitter.emitLine("str x11, [x13]");

		} else if (checkToken(TOKEN_TYPE::TEXT)) { // TEXT identifier = expression
			cout << "STATEMENT-TEXT\n";
			nextToken();

			if (symbolMap.exists(curToken.text)) {
				abort("Symbol (" + curToken.text + ") is already declared.");
			} else {
				symbolMap.push_back(curToken.text);
			}

			string identLabel = symbolMap.getLabel(curToken.text);

			match(TOKEN_TYPE::IDENTIFIER);
			match(TOKEN_TYPE::EQ);

			expression(caller);

			emitter.emitLine("adr x13, " + identLabel);
			emitter.emitLine("str x10, [x13]");

		} else if (checkToken(TOKEN_TYPE::IDENTIFIER)) { // identifier "=" expression
			cout << "STATEMENT-ASSIGN\n";

			if (!symbolMap.exists(curToken.text)) {
				abort("Symbol (" + curToken.text + ") does not exist.");
			}

			string identLabel = symbolMap.getLabel(curToken.text);
			nextToken();

			match(TOKEN_TYPE::EQ);

			expression(caller);

			emitter.emitLine("adr x13, " + identLabel);
			emitter.emitLine("str x11, [x13]");
		} else if (checkToken(TOKEN_TYPE::DO)) { // "DO" identifier
			cout << "STATEMENT-FUNCTIONCALL";
			nextToken();
			cout << " (" + curToken.text + ")";
			if (find(functions.begin(), functions.end(), curToken.text) == functions.end()) {
				abort("Function " + curToken.text + " does not exist");
			}
			
			string bLabel = "FUNC" + to_string(find(functions.begin(), functions.end(), curToken.text) - functions.begin());

			emitter.emitLine("bl " + bLabel);
			match(TOKEN_TYPE::IDENTIFIER);

		} else {
			abort("Invalid state at " + string(curToken.text) + " (" + tokenTypeToString(curToken.type) + ").");
		}
	}
	

	nl();
}

void Parser::nl() {
	cout << "NEWLINE\n";

	match(TOKEN_TYPE::NEWLINE);

	while (checkToken(TOKEN_TYPE::NEWLINE)) {
		nextToken();
	}
}

// expression ::= term {("+" | "/") term}
void Parser::expression(TOKEN_TYPE caller) {
	cout << "EXPRESSION\n";

	term(caller);

	if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mov x11, x10");
	else emitter.emitLine("mov x11, x10");

	while (checkToken(TOKEN_TYPE::PLUS) || checkToken(TOKEN_TYPE::MINUS)) {
		TOKEN_TYPE lastType = curToken.type;

		nextToken();
		term(caller);

		if (lastType == TOKEN_TYPE::PLUS) { // +
			if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("add x11, x11, x10");
			else emitter.emitLine("add x11, x11, x10");
		} else if (lastType == TOKEN_TYPE::MINUS) { // -
			if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("sub x11, x11, x10");
			else emitter.emitLine("sub x11, x11, x10");
		}
	}
}

// term ::= unary {("*" | "/") unary}
void Parser::term(TOKEN_TYPE caller) {
	cout << "TERM\n";

	unary(caller); // hold each unary in r10. do operations on r9 and put the results in r10
	
	if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mov x10, x9");
	else emitter.emitLine("mov x10, x9");


	while (checkToken(TOKEN_TYPE::ASTERISK) || checkToken(TOKEN_TYPE::SLASH) || checkToken(TOKEN_TYPE::MODULO)) {
		TOKEN_TYPE lastType = curToken.type;
		nextToken();
		unary(caller);

		if (lastType == TOKEN_TYPE::ASTERISK) { 	// multiply
			if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mul x10, x10, x9");
			else emitter.emitLine("mul x10, x10, x9");
		} else if (lastType == TOKEN_TYPE::SLASH) { // divide
			if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("sdiv x10, x10, x9");
			else emitter.emitLine("sdiv x10, x10, x9");
		} else {
			if (caller == TOKEN_TYPE::FUNC) {
				emitter.functionLine("udiv x8, x10, x9");  		// x10 is dividend x9 is divisor x8 is quotient
				emitter.functionLine("msub x10, x8, x9, x10");		// q = Dvnd / Dvsr + Rem -> REm = Q * 
			} else {							// x10 = x10 - (x9 * x10) = x10 -
				emitter.emitLine("udiv x8, x10, x9");
				emitter.emitLine("msub x10, x8, x9, x10");
			}
		}
	}
}

// unary ::= ["+" | "-"] primary
void Parser::unary(TOKEN_TYPE caller) {
	cout << "UNARY\n";

	TOKEN_TYPE lastType = curToken.type;

	// If theres a +/- skip and go to primary
	if (curToken.type == TOKEN_TYPE::PLUS || curToken.type == TOKEN_TYPE::MINUS) {
		nextToken();
	}
	primary(caller);

	if (lastType == TOKEN_TYPE::MINUS) {
		if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mvn x9, x9");
		else emitter.emitLine("mvn x9, x9");
	}
}

// primary ::= number | identifier
void Parser::primary(TOKEN_TYPE caller) { // Primary held in r9
	cout << "PRIMARY (" << curToken.text << ")\n";

	if (checkToken(TOKEN_TYPE::NUMBER)) {
		if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mov x9, #" + curToken.text);
		else emitter.emitLine("mov x9, #" + curToken.text);
		nextToken();
	} else if (checkToken(TOKEN_TYPE::IDENTIFIER)) {
		if (!symbolMap.exists(curToken.text)) {
			abort("Undeclared symbol (" + curToken.text);
		}

		if (caller == TOKEN_TYPE::FUNC) {
			 emitter.functionLine("adr x9, " + symbolMap.getLabel(curToken.text));
			 emitter.functionLine("ldr x9, [x9]");
		}
		
		else {
			emitter.emitLine("adr x9, " + symbolMap.getLabel(curToken.text));
			emitter.emitLine("ldr x9, [x9]");
		}

		nextToken();
	} else {
		abort("Expected number or identifier, recieved " + curToken.text);
	}
}

// condition ::= expression (("==" | ">" | ">=" | "<"| "<=") experssion)+
void Parser::condition(string exitLabel, TOKEN_TYPE caller) {
	cout << "CONDITION\n";

	expression();
	// result in r11
	if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mov x12, x11");
	else emitter.emitLine("mov x12, x11");

	TOKEN_TYPE conditional = curToken.type;

	if (checkToken(TOKEN_TYPE::EQEQ) || checkToken(TOKEN_TYPE::NEQ) || checkToken(TOKEN_TYPE::GT) || checkToken(TOKEN_TYPE::GTEQ) || checkToken(TOKEN_TYPE::LT) || checkToken(TOKEN_TYPE::LTEQ)) {
		nextToken();
		expression();
	} else {
		abort("Expected expression, got " + curToken.text);
	}

	emitter.emitLine("cmp x12, x11");

	switch (conditional) {
		case TOKEN_TYPE::EQEQ:
			emitter.emitLine("bne " + exitLabel);
			break;
		case TOKEN_TYPE::NEQ:
			emitter.emitLine("beq " + exitLabel);
			break;
		case TOKEN_TYPE::GT:
			emitter.emitLine("ble " + exitLabel);
			break;
		case TOKEN_TYPE::GTEQ:
			emitter.emitLine("blt " + exitLabel);
			break;
		case TOKEN_TYPE::LT:
			emitter.emitLine("bge " + exitLabel);
			break;
		case TOKEN_TYPE::LTEQ:
			emitter.emitLine("bgt " + exitLabel);
			break;
	}

	/* -------------- Currently removed multiple expressions w/i a condition
	while (checkToken(TOKEN_TYPE::EQEQ) || checkToken(TOKEN_TYPE::GT) || checkToken(TOKEN_TYPE::GTEQ) || checkToken(TOKEN_TYPE::LT) || checkToken(TOKEN_TYPE::LTEQ)) {
		nextToken();
		expression();
	}
	*/
}

/* --------------- GRAMMAR RULES

{program} ::= 		{statement}
{statement} ::= 	"PRINT" (expression | string) nl
			"IF" condition "THEN" nl {statement} "ENDIF" nl
			"WHILE" condition "DO" nl {statement} "ENDWHILE" nl
			"LABEL" identifier nl
			"GOTO" identifier nl
			"INT" identifier "=" expression nl
			"FLOAT" identifier "=" expression nl
			"TEXT" identifier "=" expression nl
			indentifier "=" expression nl
			"FUNC" identifier "IS" nl {statement} "ENDFUNC" nl
			"DO" identifier

{expression} ::= term {("-" | "+") term}
term ::= unary {("*" | "/" | "%") unary}
unary ::= ["-" | "+"] primary
primary ::= number | identifier
condition ::= expression ((">" | ">=" | "<" | "<=" | "==") expression)+
nl ::= '\n'+
*/

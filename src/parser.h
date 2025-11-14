#include "lexer.h"
#include "emitter.h"
#include <vector>
#include <algorithm>

using namespace std;

struct functionParams {
	string name;
	vector<string> params;
};

class FunctionMap {
	public:
		FunctionMap() {}
		
		int exists(string name) { // helper function to figure out if an entry exists
			if (find(functions.begin(), functions.end(), name) == functions.end()) return 0;
			else return 1;
		}

		int paramExists(string name, string param) { // check to see a param is already defined -- maybe not useful
			for (int i = 0; i < paramMap.size(); i++) {
				if (paramMap[i].name == name) {
					for (int j = 0; j < paramMap[i].params.size(); i++) {
						if (paramMap[i].params[j] == param) return 1;
					}
				}
			}
			return 0;
		}

		string getParamOffset(vector<string> params, string param) {
			int idx = find(params.begin(), params.end(), param) - params.begin();

			int posFromBack;
				
			if (params.size() % 2 != 0) { 
				posFromBack = params.size() - idx;
			} else { // for an odd number of params, the offset will be 16 aligned, so there's 8 bytes in padding
				posFromBack = params.size() - idx;
			}

			posFromBack *= 8; // 8 bytes per element

			return "#" + to_string(posFromBack);
		}

		string getLabel(string name) { 	// helper function to get the label for each function
			int idx = find(functions.begin(), functions.end(), name) - functions.begin();

			return "FUNC" + idx;
		}

		vector<string> getParams(string name) { // return the params listed under a function label
			for (int i = 0; i < paramMap.size(); i++) {
				if (paramMap[i].name == name) return paramMap[i].params;
			}
			return {};
		}

		void push_name(string name) { // should be called before push_back 
			functions.push_back(name);
		}

		void push_back(string name, vector<string> params = {}) {
			functionParams entry;
			entry.name = name;
			entry.params = params;

			paramMap.push_back(entry);
		}

		vector<functionParams> paramMap;
		vector<string> functions;
};

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
		void statement(TOKEN_TYPE caller = TOKEN_TYPE::INVALID, vector<string> parameters = {});
		void nl();
		void expression(TOKEN_TYPE caller = TOKEN_TYPE::INVALID, vector<string> parameters = {});
		void term(TOKEN_TYPE caller = TOKEN_TYPE::INVALID, vector<string> parameters = {});
		void unary(TOKEN_TYPE caller = TOKEN_TYPE::INVALID, vector<string> parameters = {});
		void primary(TOKEN_TYPE caller = TOKEN_TYPE::INVALID, vector<string> parameters = {});
		void condition(string exitLabel, TOKEN_TYPE caller = TOKEN_TYPE::INVALID, vector<string> parameters = {});

		Lexer& lexer;
		Emitter& emitter;
		Token curToken;
		Token peekToken;

		SymbolMap symbolMap;
		FunctionMap functionMap;

		vector <string> labels;
		vector <string> gotos;
		vector <string> stringLiterals;
		vector <string> functions;
		

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

void Parser::statement(TOKEN_TYPE caller, vector<string> parameters) {
	// Print statement
	if (caller == TOKEN_TYPE::FUNC) { // -------------------------------------------------------------- IN-FUNCTION STATEMENTS	
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
				expression(caller, parameters);
			}
		} else if (checkToken(TOKEN_TYPE::IF)) { // IF condition THEN statement ENDIF
			cout << "FUNC-STATEMENT-IF\n";
			nextToken();
			condition("XIF" + to_string(ifCount), caller, parameters);

			match(TOKEN_TYPE::THEN);
			nl();

			while (checkToken(TOKEN_TYPE::ENDIF) == 0) {
				statement(caller, parameters);
			}
			match(TOKEN_TYPE::ENDIF);
			
			emitter.functionLine("XIF" + to_string(ifCount) + ":");
			ifCount++;

		} else if (checkToken(TOKEN_TYPE::WHILE)) { // WHILE condition DO statement ENDWHILE
			cout << "FUNC-STATEMENT-WHILE\n";
			nextToken();
			emitter.functionLine("SWHILE" + to_string(whileCount) + ":");

			condition("XWHILE" + to_string(whileCount), caller, parameters);
			
			match(TOKEN_TYPE::DO);
			nl();

			while (checkToken(TOKEN_TYPE::ENDWHILE) == 0) {
				statement(caller, parameters);
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

			expression(caller, parameters);

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

			expression(caller, parameters);

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

			expression(caller, parameters);

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

			expression(caller, parameters);

			emitter.functionLine("adr x13, " + identLabel);
			emitter.functionLine("str x11, [x13]");
		
		} else if (checkToken(TOKEN_TYPE::DO)) { // "DO" identifier
			cout << "STATEMENT-FUNCTIONCALL";
			nextToken();
			cout << " (" + curToken.text + ")";
			if (!functionMap.exists(curToken.text)) {
				abort("Function " + curToken.text + " does not exist");
			}
				
			string bLabel = functionMap.getLabel(curToken.text);

			emitter.functionLine("bl " + bLabel);
			match(TOKEN_TYPE::IDENTIFIER);
		} else {
			abort("Invalid state at " + string(curToken.text) + " (" + tokenTypeToString(curToken.type) + ").");
		}
	} else { // --------------------------------------------------------------------------------------------------- OUT-OF-FUNCTION STATEMENTS
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

			if (functionMap.exists(curToken.text)) {
				abort("Function (" + curToken.text + ") already exists");
			}

			functionMap.push_name(curToken.text);

			string funcIdentifier = curToken.text;
			string bLabel = functionMap.getLabel(curToken.text);	
			emitter.functionLine(bLabel + ":");

			emitter.functionLine("stp fp, lr, [sp, #-16]!");

			match(TOKEN_TYPE::IDENTIFIER);
			
			vector<string> params;

			if (checkToken(TOKEN_TYPE::USING)) { // FUNC identifier USING identifier {"," identifier} IS ...
				cout << "\tPARAMETERS\n";
				nextToken();

				params.push_back(curToken.text);
				match(TOKEN_TYPE::IDENTIFIER);
				
				while (checkToken(TOKEN_TYPE::IS) == 0) {
					match(TOKEN_TYPE::COMMA);
					
					if (find(params.begin(), params.end(), curToken.text) != params.end()) {
						abort("Function parameter (" + curToken.text + ") already exists");
					}
					
					if (symbolMap.exists(curToken.text)) {
						abort("Symbol (" + curToken.text + ") exists outside of the function");
					}

					params.push_back(curToken.text);

					match(TOKEN_TYPE::IDENTIFIER);
				}

				functionMap.push_back(funcIdentifier, params);
			}

			match(TOKEN_TYPE::IS);
			nl();

			while (!checkToken(TOKEN_TYPE::ENDFUNC)) {
				statement(TOKEN_TYPE::FUNC, params);
			}

			match(TOKEN_TYPE::ENDFUNC);
			
			emitter.functionLine("add sp, sp, #" + to_string((params.size() + params.size() % 2) * 8));

			emitter.functionLine("ldp fp, lr, [sp], #16");
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
			cout << " (" + curToken.text + ")\n";
			if (!functionMap.exists(curToken.text)) {
				abort("Function " + curToken.text + " does not exist");
			}
			
			string branchIdentifier = curToken.text;
			string bLabel = functionMap.getLabel(curToken.text);
			//emitter.emitLine("bl " + bLabel); dont do this yet
			match(TOKEN_TYPE::IDENTIFIER);


			if (checkToken(TOKEN_TYPE::WITH)) { // "DO" identifier "WITH" expression {"," expression}
				cout << "\nFUNCTIONCALL-PARAMETERS\n";
				nextToken();
				
				int paramCount = 1;
				expression();

				emitter.emitLine("str x11, [sp, #-8]!");

				while (checkToken(TOKEN_TYPE::NEWLINE) == 0) {
					match(TOKEN_TYPE::COMMA);
					paramCount++;

					expression();
					emitter.emitLine("str x11, [sp, #-8]!");
				}
				
				if (paramCount % 2 != 0) { // stack always has to be 16 aligned
					emitter.emitLine("sub sp, sp, #8");			
				}

				if (paramCount != functionMap.getParams(branchIdentifier).size()) {
					abort("Function (" + branchIdentifier + ") expects " + to_string(functionMap.getParams(branchIdentifier).size()) + " parameters, only recieved " + to_string(paramCount));
				}
			} else {
				if (functionMap.getParams(branchIdentifier).size() != 0) {
					abort("Function (" + branchIdentifier + ") expects arguments");
				}
			}

			emitter.emitLine("bl " + bLabel);
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
void Parser::expression(TOKEN_TYPE caller, vector<string> parameters) {
	cout << "EXPRESSION\n";

	term(caller, parameters);

	if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mov x11, x10");
	else emitter.emitLine("mov x11, x10");

	while (checkToken(TOKEN_TYPE::PLUS) || checkToken(TOKEN_TYPE::MINUS)) {
		TOKEN_TYPE lastType = curToken.type;

		nextToken();
		term(caller, parameters);

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
void Parser::term(TOKEN_TYPE caller, vector<string> parameters) {
	cout << "TERM\n";

	unary(caller, parameters); // hold each unary in r10. do operations on r9 and put the results in r10
	
	if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mov x10, x9");
	else emitter.emitLine("mov x10, x9");


	while (checkToken(TOKEN_TYPE::ASTERISK) || checkToken(TOKEN_TYPE::SLASH) || checkToken(TOKEN_TYPE::MODULO)) {
		TOKEN_TYPE lastType = curToken.type;
		nextToken();
		unary(caller, parameters);

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
void Parser::unary(TOKEN_TYPE caller, vector<string> parameters) {
	cout << "UNARY\n";

	TOKEN_TYPE lastType = curToken.type;

	// If theres a +/- skip and go to primary
	if (curToken.type == TOKEN_TYPE::PLUS || curToken.type == TOKEN_TYPE::MINUS) {
		nextToken();
	}
	primary(caller, parameters);

	if (lastType == TOKEN_TYPE::MINUS) {
		if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mvn x9, x9");
		else emitter.emitLine("mvn x9, x9");
	}
}

// primary ::= number | identifier
void Parser::primary(TOKEN_TYPE caller, vector<string> parameters) { // Primary held in r9
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
			 if (!parameters.empty()) {
			 	if (find(parameters.begin(), parameters.end(), curToken.text) != parameters.end()) {
					emitter.functionLine("ldr x9, [sp, " + functionMap.getParamOffset(parameters, curToken.text) + "]");
				} else {
			 		emitter.functionLine("adr x9, " + symbolMap.getLabel(curToken.text));
			 		emitter.functionLine("ldr x9, [x9]");
				}
			}
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
void Parser::condition(string exitLabel, TOKEN_TYPE caller, vector<string> parameters) {
	cout << "CONDITION\n";

	expression(caller, parameters);
	// result in r11
	if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("mov x12, x11");
	else emitter.emitLine("mov x12, x11");

	TOKEN_TYPE conditional = curToken.type;

	if (checkToken(TOKEN_TYPE::EQEQ) || checkToken(TOKEN_TYPE::NEQ) || checkToken(TOKEN_TYPE::GT) || checkToken(TOKEN_TYPE::GTEQ) || checkToken(TOKEN_TYPE::LT) || checkToken(TOKEN_TYPE::LTEQ)) {
		nextToken();
		expression(caller, parameters);
	} else {
		abort("Expected expression, got " + curToken.text);
	}
	
	if (caller == TOKEN_TYPE::FUNC) emitter.functionLine("cmp x12, x11");
	else emitter.emitLine("cmp x12, x11");

	string branchType = "";

	switch (conditional) {
		case TOKEN_TYPE::EQEQ:
			branchType = "bne ";
			break;
		case TOKEN_TYPE::NEQ:
			branchType = "beq "; 
			break;
		case TOKEN_TYPE::GT:
			branchType = "ble "; 	
			break;
		case TOKEN_TYPE::GTEQ:
			branchType = "blt ";
			break;
		case TOKEN_TYPE::LT:
			branchType = "bge ";
			break;
		case TOKEN_TYPE::LTEQ:
			branchType = "bgt ";
			break;
	}

	if (caller == TOKEN_TYPE::FUNC) emitter.functionLine(branchType + exitLabel);
	else emitter.emitLine(branchType + exitLabel);

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

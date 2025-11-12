#include <iostream>
#include <string>
#include <fstream>


#ifndef EMITTER_H
#define EMITTER_H
using namespace std;

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

Emitter::Emitter(string filePath) {
	path = filePath;

	header = "";
	code = "";
	functions = "";
	data = "";
}

Emitter::Emitter() {
	path = "out.s";

	header = "";
	code = "";
	functions = "";
	data = "";
}

void Emitter::abort(string message) {
	cerr << "Error (EMITTER)\n";
	cerr << message << endl;
	exit(1);
}

void Emitter::emit(string codeIn) {
	code += codeIn;
}

void Emitter::emitLine (string codeIn) { 
	code += codeIn + "\n";
}

void Emitter::headerLine(string codeIn) {
	header += codeIn + "\n";
}

void Emitter::dataLine(string codeIn) {
	data += codeIn + "\n";
}

void Emitter::functionLine(string codeIn) {
	functions += codeIn + "\n";
}

void Emitter::writeFile() {
	ofstream outputFile(path);

	if (!outputFile.is_open()) {
		abort("Cannot open file " + path);
	}

	outputFile << header + code + functions + "\n\t.data\n" + data;
	outputFile.close();
}

#endif

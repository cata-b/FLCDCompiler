#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include "SymbolTable.h"
#include "LexicalAnalyzer.h"
#include "Tokenizer.h"

#include <regex>
using namespace std;

int main(int argc, char** argv)
{
	if (argc < 2)
		cout << "Usage: <inputFile>";
	/*vector<tuple<Token, LexicalAnalyzer::TokenType, SymbolTable::Position>> pif;

	auto errors = LexicalAnalyzer::Analyze(Tokenizer::tokenize(argv[1]), st, pif);
	for (auto& error : errors)
		cout << error.content << " " << error.line << endl;*/

	/*vector<SymbolTable::Position> positions;
	for (int i = 0; i < 1; i++)
		positions.push_back(st.insert(to_string(i)).first);
	for (int i = 0; i < 1; i++)
		positions.push_back(st.insert(to_string(i)).first);

	for (auto& position : positions)
		cout << *position << endl;*/
	for (auto& token : Tokenizer::tokenize(argv[1]))
		cout << token.content << endl;
}
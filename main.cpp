#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <iomanip>
#include "Tokenizer.h"
#include "LexicalAnalyzer.h"
using namespace std;

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		cout << "Usage: <input file> <internal form output file> <symbol table output file>";
		return -1;
	}
	
	vector<tuple<Token, LexicalAnalyzer::TokenType, SymbolTable::Position>> pif;
	SymbolTable st;
	vector<Token> errorTokens;
	try
	{
		errorTokens = LexicalAnalyzer::Analyze(Tokenizer::tokenize(argv[1]), st, pif);
	}
	catch (Tokenizer::Error& tokenizerError)
	{
		cout << string("Tokenizer error: ") + tokenizerError.what() + 
			(tokenizerError.hasToken() ? 
				" token: " + tokenizerError.token().content + " line: " + to_string(tokenizerError.token().line) 
				: "");
		cout << "Did not write symbol table/internal form\n";
		return -1;
	}
	if (errorTokens.size() > 0)
	{
		cout << "Errors:\n\n";
		for (auto& t : errorTokens)
			cout << "Line: " + to_string(t.line) + "; token: " + t.content + "\n";
	}
	else
		cout << "Lexically correct.\n";

	ofstream pifFile(argv[2]);
	for (const auto& entry : pif)
	{
		string token = get<0>(entry).content;
		LexicalAnalyzer::TokenType tokenType = get<1>(entry);
		auto stPosition = get<2>(entry);
		string position = stPosition == st.end() ? "-1" : to_string(stPosition.index());

		if (tokenType == LexicalAnalyzer::TokenType::CONSTANT)
			token = "CONSTANT";
		if (tokenType == LexicalAnalyzer::TokenType::IDENTIFIER)
			token = "IDENTIFIER";

		pifFile << left << setw(sizeof("IDENTIFIER")) << setfill(' ') << token << position + "\n";
	}
	pifFile.close();

	ofstream stFile(argv[3]);
	vector<SymbolTable::Position> positions;
	positions.reserve(st.size());
	for (auto pos = st.begin(); pos != st.end(); ++pos)
		positions.push_back(pos);
	sort(positions.begin(), positions.end(), 
		[](const SymbolTable::Position& p1, const SymbolTable::Position& p2) {
			return p1.index() < p2.index();
		});
	for (auto& position : positions)
		stFile << position.index() << " " << *position << endl;
}
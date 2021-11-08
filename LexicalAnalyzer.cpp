#include "LexicalAnalyzer.h"
#include <regex>
#include <algorithm>
#include "FA.hpp"
#include "CharFAReader.h"

using namespace std;

const vector<pair<function<bool(string)>, LexicalAnalyzer::TokenType>> LexicalAnalyzer::TOKEN_CLASSES = {

	{ [r = regex(R"(int|uint|bool|string|read|print|error|rand|exit|for|while|if|else|break|continue)")] (string t) { return regex_match(t, r); }, TokenType::KEYWORD },
	{ [r = regex(R"((?:[1-9]+[0-9]*)|0)")] (string t){ return regex_match(t, r); }, TokenType::CONSTANT }, // unsigned integer
	{ [fa = CharFAReader::readFA("FLCDFiniteAutomaton/int_const.in")] (string t){ return fa.accepts(t.begin(), t.end()); }, TokenType::CONSTANT}, // integer
	{ [r = regex(R"(true|false)")] (string t){ return regex_match(t, r); }, TokenType::CONSTANT }, // bool
	{ [r = regex(R"(\"[a-zA-Z0-9\+\-*/%\\=<>[\]{}()?!_.|&*^",':; \t]*\")")] (string t){ return regex_match(t, r); }, TokenType::CONSTANT }, // string
	{ [r = regex(R"(==|!=|>=|<=|\|\||&&|\+|-|\*|\/|%|=|<|>|\^|\||&|!)")] (string t){ return regex_match(t, r); }, TokenType::OPERATOR },
	{ [r = regex(R"(\(|\)|\{|\}|\[|\]|,|'|:|;)")] (string t){ return regex_match(t, r); }, TokenType::SEPARATOR },
	{ [fa = CharFAReader::readFA("FLCDFiniteAutomaton/identifier.in")] (string t){ return fa.accepts(t.begin(), t.end()); }, TokenType::IDENTIFIER}
};

vector<Token> LexicalAnalyzer::Analyze(vector<Token> tokens, SymbolTable& symbolTable, vector<tuple<Token, TokenType, SymbolTable::Position>>& pif)
{
	vector<Token> errors;
	for (auto& token : tokens)
	{
		TokenType type = TokenType::ERROR;
		for (const auto& cls : TOKEN_CLASSES)
			if (cls.first(token.content))
			{
				type = cls.second;
				break;
			}
		if (type == TokenType::IDENTIFIER || type == TokenType::CONSTANT)
		{
			pif.push_back(make_tuple(token, type, symbolTable.insert(token.content).first));
			continue;
		}
		if (type == TokenType::ERROR)
		{
			errors.push_back(token);
			continue;
		}
		pif.push_back(make_tuple(token, type, symbolTable.end()));
	}
	return errors;
}

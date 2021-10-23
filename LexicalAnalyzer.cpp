#include "LexicalAnalyzer.h"
#include <regex>
#include <algorithm>

using namespace std;

const vector<pair<regex, LexicalAnalyzer::TokenType>> LexicalAnalyzer::TOKEN_CLASSES = {
	{ regex(R"(int|uint|bool|string|read|print|error|rand|exit|for|while|if|else|break|continue)"), TokenType::KEYWORD },
	{ regex(R"((?:[1-9]+[0-9]*)|0)"), TokenType::CONSTANT },
	{ regex(R"((:?\+|\-)[1-9]+[0-9]*)"), TokenType::CONSTANT },
	{ regex(R"(true|false)"), TokenType::CONSTANT },
	{ regex(R"(\"[a-zA-Z0-9\+\-*/%\\=<>[\]{}()?!_.|&*^",':; \t]*\")"), TokenType::CONSTANT },
	{ regex(R"(==|!=|>=|<=|\|\||&&|\+|-|\*|\/|%|=|<|>|\^|\||&|!)"), TokenType::OPERATOR },
	{ regex(R"(\(|\)|\{|\}|\[|\]|,|'|:|;)"), TokenType::SEPARATOR },
	{ regex(R"([a-zA-Z_]+[a-zA-Z0-9_]*)"), TokenType::IDENTIFIER }
};

vector<Token> LexicalAnalyzer::Analyze(std::vector<Token> tokens, SymbolTable& symbolTable, std::vector<std::tuple<Token, TokenType, SymbolTable::Position>>& pif)
{
	vector<Token> errors;
	for (auto& token : tokens)
	{
		TokenType type = TokenType::ERROR;
		for (const auto& cls : TOKEN_CLASSES)
			if (regex_match(token.content, cls.first))
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

#pragma once

#include "Token.h"
#include "SymbolTable.h"
#include <vector>
#include <regex>

class LexicalAnalyzer
{
public:
	enum class TokenType
	{
		KEYWORD,
		CONSTANT,
		OPERATOR,
		SEPARATOR,
		IDENTIFIER,
		ERROR
	};
	/// <summary>
	/// Analyzes and classifies tokens
	/// </summary>
	/// <param name="tokens">Tokens, as returned by <see cref="Tokenizer::tokenize" /></param>
	/// <param name="symbolTable">Output parameter that will contain all the identifiers and constants from the tokens</param>
	/// <param name="pif">Output parameter that will contain tokens, their type, and their positions in the symbol table (if a token is not inserted in the symbol table, the position is the <see cref="SymbolTable::end">end</see> of that table)</param>
	/// <returns>All the tokens that could not be classified</returns>
	static std::vector<Token> Analyze(std::vector<Token> tokens, SymbolTable& symbolTable, std::vector<std::tuple<Token, TokenType, SymbolTable::Position>>& pif);
private:
	/// <summary>
	/// Maps a regular expression to a token type; the order of the elements is such that, if checking in order, tokens that would match more entries will match the correct entry 
	/// (e.g. true will be classified as a constant, rather than an identifier)
	/// </summary>
	static const std::vector<std::pair<std::regex, TokenType>> TOKEN_CLASSES;
};


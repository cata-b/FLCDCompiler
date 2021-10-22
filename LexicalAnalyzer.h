#pragma once

#include "Token.h"
#include "SymbolTable.h"
#include <vector>
#include <stdexcept>
#include <regex>
#include <functional>

class LexicalAnalyzer
{
public:
	class Error : public std::runtime_error
	{
	private:
		Token token_;
	public:
		Error(std::string what, Token token) : runtime_error(what), token_{ token } {};
		Token token() { return token_; }
	};

	enum class TokenType
	{
		KEYWORD,
		CONSTANT,
		OPERATOR,
		SEPARATOR,
		IDENTIFIER,
		ERROR
	};

	static std::vector<Token> Analyze(std::vector<Token> tokens, SymbolTable& symbolTable, std::vector<std::tuple<Token, TokenType, SymbolTable::Position>>& pif);
private:
	static const std::string ALPHABET_REGEX;
	static const std::vector<std::pair<std::regex, TokenType>> TOKEN_CLASSES;

	// returns tokens that contain something other than the characters in the alphabet
	static std::vector<Token> checkAlphabet(std::vector<Token> tokens);
};


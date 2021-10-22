#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "Token.h"
#include <vector>
#include <string>
#include <stdexcept>
class Tokenizer
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

	static std::vector<Token> tokenize(std::string filename);

private:
	// reads a file and returns tokens that may be either strings of separators or strings of not separators
	static std::vector<Token> splitTokensFromSeparators(std::string filename);

	// splits the strings that are made of separators into individual tokens
	static std::vector<Token> splitSeparators(Token input);

	// removes sequences of tokens that begin with '//' and end with '\n'
	static std::vector<Token> removeComments(std::vector<Token> tokenized);

	// combines token sequences that are on a single line and begin and end with "
	static std::vector<Token> combineStringLiterals(std::vector<Token> tokenizedWithoutComments);

	// removes entries that are whitespaces
	static std::vector<Token> removeWhitespaces(std::vector<Token> tokenizedWithStringLiterals);
};


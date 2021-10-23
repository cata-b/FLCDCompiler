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
		bool hasToken_ = false;
	public:
		Error(std::string what, Token token) : runtime_error(what), token_{ token }, hasToken_(true) {};
		Error(std::string what) : runtime_error(what), hasToken_(false) {};
		bool hasToken() { return hasToken_; }
		Token token() { return token_; }
	};

	/// <summary>
	/// Reads input from a file and splits it into tokens
	/// </summary>
	/// <param name="filename">The name of the file to read</param>
	/// <exception cref="Error">Thrown when the tokenizer cannot read a file or cannot split the content into tokens correctly</exception>
	/// <returns>A vector of tokens</returns>
	static std::vector<Token> tokenize(std::string filename);

private:
	
	/// <summary>
	/// Reads a file and (partially) splits it into tokens
	/// </summary>
	/// <param name="filename">The name of the file to read</param>
	/// <returns>Tokens that may be either strings of separators or strings of not separators</returns>
	static std::vector<Token> splitTokensFromSeparators(std::string filename);

	/// <summary>
	/// Splits the strings that are made of separators into individual tokens
	/// </summary>
	/// <param name="input">Token returned by <see cref="splitTokensFromSeparators"/></param>
	/// <returns>Tokens where all separators are a separate token (taking into account 2-character operators, e.g. >=)</returns>
	static std::vector<Token> splitSeparators(Token input);

	/// <summary>
	/// Removes sequences of tokens that begin with '//' and end with '\n'
	/// </summary>
	/// <param name="tokenizedWithSplitSeparators">Output of <see cref="splitSeparators"/></param>
	/// <returns>The modified tokens</returns>
	static std::vector<Token> removeComments(std::vector<Token> tokenizedWithSplitSeparators);

	/// <summary>
	/// Combines token sequences that are on a single line and begin and end with "
	/// </summary>
	/// <param name="tokenizedWithoutComments">Output of <see cref="removeComments"/></param>
	/// <exception cref="Error">Thrown when a string constant doesn't end on the same line or doesn't end at all</exception>
	/// <returns>Vector of tokens where string literals are single tokens</returns>
	static std::vector<Token> combineStringLiterals(std::vector<Token> tokenizedWithoutComments);

	/// <summary>
	/// Removes entries that are whitespaces
	/// </summary>
	/// <param name="tokenizedWithStringLiterals">Vector of tokens that contains spaces, tabs or newlines as tokens</param>
	/// <returns>The tokens that are not whitespaces</returns>
	static std::vector<Token> removeWhitespaces(std::vector<Token> tokenizedWithStringLiterals);
};


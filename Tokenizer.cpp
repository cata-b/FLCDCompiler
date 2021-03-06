#include "Tokenizer.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>


using namespace std;

#define BUFFER_SIZE (1 << 7) - 1
#define WHITESPACE "\t\n "
#define SEPARATORS "[]{}():;,+-*/%^|&!\\\"\t\n "

// atoms that require reading one character forward, together with possible valid following characters
#define WIDE_SEPARATOR_ATOMS \
	{'/', { '/' }}, \
	{'|', { '|' }}, \
	{'&', { '&' }}, \
	{'=', { '=' }}, \
	{'!', { '=' }}, \
	{'<', { '=' }}, \
	{'>', { '=' }}

size_t countOccurrences(const char* str, char ch)
{
	size_t count = 0;
	for (int i = 0; str[i]; ++i)
		count += (ch == str[i]);
	return count;
};

vector<Token> Tokenizer::tokenize(string filename)
{
	auto data = splitTokensFromSeparators(filename);
	vector<Token> result;
	for (auto& entry : data)
		if (strchr(SEPARATORS, entry.content[0]))
			for (auto atom : splitSeparators(entry))
				result.push_back(atom);
		else
			result.push_back(entry);
	result =
		removeWhitespaces(
			combineStringLiterals(
				combineIntConstants(
					removeComments(result)
				)
			)
		);
	return result;
}

vector<Token> Tokenizer::splitTokensFromSeparators(string filename)
{
	vector<Token> result;

	FILE* inputFile;
	errno_t err;
	if ((err = fopen_s(&inputFile, filename.c_str(), "r")) != 0)
		throw Error(string("Error while reading file: ") + strerror(err));
	char buffer[BUFFER_SIZE + 1];
	char bufferCopy[BUFFER_SIZE + 1];
	size_t bufferOffset = 0;
	size_t amountRead = BUFFER_SIZE;
	size_t currentLine = 1;

	size_t separatorsSize = 1 << 7;
	unique_ptr<char> separators(new char[separatorsSize]);

	auto resizeSeparators = [&separators, &separatorsSize](size_t requiredSize) {
		if (requiredSize > separatorsSize)
		{
			separatorsSize = requiredSize + 1;
			separators.reset(new char[separatorsSize + 1]);
		}
	};

	auto getTokensFromBuffer = [&]()
	{
		strncpy_s(bufferCopy, buffer, amountRead + bufferOffset);
		bufferCopy[amountRead + bufferOffset] = 0;

		char* token, * nextToken = 0;

		size_t sepStart = 0;
		token = strtok_s(buffer, SEPARATORS, &nextToken);
		size_t sepEnd = token - buffer;

		while (token != NULL && nextToken < buffer + amountRead)
		{
			currentLine += countOccurrences(token, '\n');
			size_t requiredSeparatorsSize = sepEnd - sepStart;

			resizeSeparators(requiredSeparatorsSize);

			strncpy(separators.get(), bufferCopy + sepStart, requiredSeparatorsSize);
			separators.get()[requiredSeparatorsSize] = 0;

			if (strlen(separators.get()) > 0)
			{
				currentLine += countOccurrences(separators.get(), '\n');
				result.push_back(Token{ string(separators.get()), currentLine });
			}
			result.push_back(Token{ string(token), currentLine });

			sepStart = token + strlen(token) - buffer;
			token = strtok_s(NULL, SEPARATORS, &nextToken);
			sepEnd = token - buffer;
		}
		if (token == NULL)
		{
			bufferOffset = 0;
			if (sepStart < BUFFER_SIZE)
				result.push_back(Token{ bufferCopy + sepStart, currentLine });
			return;
		}

		size_t requiredSeparatorsSize = sepEnd - sepStart;

		resizeSeparators(requiredSeparatorsSize);

		strncpy(separators.get(), bufferCopy + sepStart, requiredSeparatorsSize);
		separators.get()[requiredSeparatorsSize] = 0;

		if (strlen(separators.get()) > 0)
		{
			currentLine += countOccurrences(separators.get(), '\n');
			result.push_back(Token{ string(separators.get()), currentLine });
		}


		bufferOffset = (buffer + bufferOffset + amountRead) - token;
		strncpy(buffer, bufferCopy + (token - buffer), bufferOffset);
	};

	size_t oldBufferOffset = 0;
	while (!(amountRead + oldBufferOffset < BUFFER_SIZE))
	{
		amountRead = fread(buffer + bufferOffset, sizeof(char), BUFFER_SIZE - bufferOffset, inputFile);
		if (amountRead == 0)
			continue;
		oldBufferOffset = bufferOffset;
		getTokensFromBuffer();
	}
	if (bufferOffset > 0) // characters left in the buffer
	{
		amountRead = bufferOffset;

		getTokensFromBuffer();
	}

	return result;
}

vector<Token> Tokenizer::splitSeparators(Token input)
{
	vector<Token> result;
	size_t line = input.line - countOccurrences(input.content.c_str(), '\n');
	static const unordered_map<char, vector<char>> wideSeparators{ WIDE_SEPARATOR_ATOMS };

	for (int i = 0; i < input.content.length(); ++i)
	{
		char current = input.content[i];
		if (current == '\n')
			++line;
		if (i < input.content.length() - 1)
		{
			auto mapIt = wideSeparators.find(current);
			if (mapIt != wideSeparators.end())
			{
				bool foundPair = false;
				for (const auto chr : mapIt->second)
					if (chr == input.content[i + 1])
					{
						foundPair = true;
						result.push_back(Token{ string(1, current) + chr, line });
						break;
					}
				if (foundPair)
				{
					++i;
					continue;
				}
			}
		}
		result.push_back(Token{ string(1, current), line });
	}

	return result;
}

vector<Token> Tokenizer::removeComments(vector<Token> tokenized)
{
	vector<Token> result;

	bool removing = false;

	for (auto&& entry : tokenized)
	{
		if (entry.content == "//")
			removing = true;
		else if (entry.content == "\n")
		{
			auto previouslyRemoving = removing;
			removing = false;
			if (previouslyRemoving)
				continue;
		}

		if (!removing)
			result.push_back(entry);
	}

	return result;
}

std::vector<Token> Tokenizer::combineIntConstants(std::vector<Token> tokenized)
{
	static const auto digits_pattern = regex(R"([0-9]+)");
	static const auto sign_pattern = regex(R"(\+|\-)");
	static const auto sep_pattern = regex(R"(==|!=|>=|<=|\|\||&&|\+|-|\*|\/|%|=|<|>|\^|\||&|!|\(|\)|\{|\}|\[|\]|,|'|:|;)");
	vector<Token> result;
	result.reserve(tokenized.size());
	result.push_back(tokenized[0]);
	for (size_t i = 1; i < tokenized.size(); ++i)
	{
		auto& current = tokenized[i];
		if (regex_match(current.content, digits_pattern))
		{
			auto& prev = tokenized[i - 1];
			if (regex_match(prev.content, sign_pattern))
			{
				bool should_combine = true;
				for (int j = i - 2; j >= 0; --j)
				{
					auto& entry = tokenized[j];
					if (!std::all_of(entry.content.begin(), entry.content.end(), [](char c) {return std::isspace(static_cast<unsigned char>(c)); }))
					{
						if (!regex_match(entry.content, sep_pattern))
							should_combine = false;
						break;
					}
				}
				if (should_combine)
				{
					result.pop_back();
					result.push_back(Token{ prev.content + current.content, prev.line });
					continue;
				}
			}
		}
		result.push_back(current);
	}
	return result;
}

vector<Token> Tokenizer::combineStringLiterals(vector<Token> tokenizedWithoutComments)
{
	vector<Token> result;
	bool accumulatingString = false;
	ostringstream accumulated;
	size_t line = 0;
	Token prevEntry = Token{ "", 0 };
	for (auto& entry : tokenizedWithoutComments)
	{
		line = entry.line;
		if (entry.content == "\"")
		{
			accumulatingString = !accumulatingString;
			accumulated << entry.content;
			if (!accumulatingString)
			{
				result.push_back(Token{ accumulated.str(), line });
				accumulated = ostringstream();
			}
			prevEntry = entry;
			continue;
		}
		else if (entry.content == "\n" && accumulatingString)
		{
			throw Error("Incomplete string literal", prevEntry);
		}
		if (accumulatingString)
			accumulated << entry.content;
		else
			result.push_back(entry);
		prevEntry = entry;
	}
	if (accumulated.str().size() > 0)
		result.push_back(Token{ accumulated.str(), line });
	return result;
}

vector<Token> Tokenizer::removeWhitespaces(vector<Token> tokenizedWithStringLiterals)
{
	vector<Token> result;
	for (auto&& entry : tokenizedWithStringLiterals)
	{
		if (entry.content.length() == 1 && strchr(WHITESPACE, entry.content[0]))
			continue;
		result.push_back(entry);
	}
	return result;
}
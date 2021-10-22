#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "Set.hpp"
/// <summary>
/// Symbol table data structure. Features iterators that will sometimes get updated on insert, so that they are never invalid
/// </summary>
class SymbolTable
{
public:
	/// <summary>
	/// Symbol table const iterator. Will still be valid after inserts in the parent SymbolTable
	/// </summary>
	class Position
	{
	public:
		typedef std::input_iterator_tag iterator_category;
		typedef const std::string value_type;
		typedef std::ptrdiff_t difference_type;
		typedef const std::string* pointer;
		typedef const std::string& reference;

		Position(Position&& other);
		Position(const Position& other);
		Position(SymbolTable* parent, Set<std::string>::Iterator iterator);
		~Position();

		SymbolTable& symbolTable() const;

		Position& operator++();
		Position operator++(int);

		bool operator==(Position other) const;
		bool operator!=(Position other) const;

		reference operator*() const;
		pointer operator->() const;

		friend class SymbolTable;
	private:
		struct Data
		{
			SymbolTable* parent_;
			Set<std::string>::Iterator iterator_;
		};
		Data* data_ = nullptr;
		void update(SymbolTable* parent, Set<std::string>::Iterator iterator);
	};

	/// <summary> Inserts a symbol into the symbol table </summary>
	/// <param name="symbol">The symbol to insert</param>
	/// <returns>
	/// A Position of where the symbol is and whether or not the insertion happened; if a similar symbol already exists in the table, 
	/// the Position points to that
	/// </returns>
	std::pair<Position, bool> insert(const std::string& symbol);

	/// <summary>
	/// Searches for an entry in the SymbolTable
	/// </summary>
	/// <param name="symbol">The symbol to look for</param>
	/// <returns>Position to the symbol or <see cref="end">end()</see> if it was not found</returns>
	Position find(const std::string& symbol);

	Position begin();
	Position end();
private:
	Set<std::string> data_;
	std::unordered_set<Position::Data*> positions_;

	void subscribe(Position::Data* subscriber);
	void unsubscribe(Position::Data* subscriber);
};
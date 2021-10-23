#include "SymbolTable.h"
#include <optional>

#include <iostream>
#pragma region Symbol Table Methods

std::pair<typename SymbolTable::Position, bool> SymbolTable::insert(const std::string& symbol)
{
	if (data_.willInvalidateIteratorsOnInsert())
	{
		std::vector<std::pair<Position::Data*, std::string>> setCopy;
		setCopy.reserve(data_.size());
		std::vector<Position::Data*> endPositions;

		for (auto* positionData : positions_)
		{
			if (positionData->iterator_ == data_.end())
				endPositions.push_back(positionData);
			else
				setCopy.push_back({ positionData, *(positionData->iterator_) });
		}

		auto ret = data_.insert(symbol);
		
		for (auto entry : setCopy) // modify all positions to point to the data
			entry.first->iterator_ = data_.find(entry.second);
		for (auto endPos : endPositions)
			endPos->iterator_ = data_.end();
			
		return { Position(this, ret.first), ret.second };
	}
	auto ret = data_.insert(symbol);
	return { Position(this, ret.first), ret.second };
}

typename SymbolTable::Position SymbolTable::find(const std::string& symbol)
{
	return Position(this, data_.find(symbol));
}

size_t SymbolTable::size() const
{
	return data_.size();
}

SymbolTable::Position SymbolTable::begin()
{
	return Position(this, data_.begin());
}

SymbolTable::Position SymbolTable::end()
{
	return Position(this, data_.end());
}

SymbolTable::~SymbolTable()
{
	destructed_ = true;
}

void SymbolTable::subscribe(Position::Data* subscriber)
{
	positions_.insert(subscriber);
}

void SymbolTable::unsubscribe(Position::Data* subscriber)
{
	positions_.erase(positions_.find(subscriber));
}

#pragma endregion

#pragma region Symbol Table Position Methods

SymbolTable::Position::Position(Position&& other)
{
	data_ = other.data_;
	other.data_ = nullptr;
}

SymbolTable::Position::Position(const Position& other) 
{
	data_ = new Data{ other.data_->parent_, other.data_->iterator_ };
	data_->parent_->subscribe(data_); 
}
SymbolTable::Position::Position(SymbolTable* parent, Set<std::string>::Iterator iterator) : data_(new Data{ parent, iterator }) { parent->subscribe(data_); }

inline SymbolTable::Position::~Position() 
{ 
	if (data_ == nullptr) 
		return; 
	if (!data_->parent_->destructed_)
		data_->parent_->unsubscribe(data_);
	delete data_;
	data_ = nullptr;
}

#pragma endregion

SymbolTable& SymbolTable::Position::symbolTable() const
{
	return *(data_->parent_);
}

typename SymbolTable::Position& SymbolTable::Position::operator=(const Position& other)
{
	data_ = new Data{ other.data_->parent_, other.data_->iterator_ };
	data_->parent_->subscribe(data_);
	return *this;
}

typename SymbolTable::Position& SymbolTable::Position::operator=(Position&& other)
{
	data_ = other.data_;
	other.data_ = nullptr;
	return *this;
}

typename SymbolTable::Position& SymbolTable::Position::operator++()
{
	++data_->iterator_;
	return *this;
}

typename SymbolTable::Position SymbolTable::Position::operator++(int)
{
	Position retVal(*this);
	++data_->iterator_;
	return retVal;
}

bool SymbolTable::Position::operator==(Position other) const
{
	return data_->parent_ == other.data_->parent_ && data_->iterator_ == other.data_->iterator_;
}

bool SymbolTable::Position::operator!=(Position other) const
{
	return !(*this == other);
}

typename SymbolTable::Position::reference SymbolTable::Position::operator*() const
{
	return *(data_->iterator_);
}

typename SymbolTable::Position::pointer SymbolTable::Position::operator->() const
{
	return data_->iterator_.operator->();
}

size_t SymbolTable::Position::index() const
{
	return data_->iterator_.index();
}

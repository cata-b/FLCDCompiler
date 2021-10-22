#pragma once

#include <iterator>
#include <functional>
#include <algorithm>
#include <vector>
#include <stdexcept>

/// <summary>
/// Hashtable with coalesced chaining
/// </summary>
/// <typeparam name="KeyType">The type of the items held in this set</typeparam>
/// <typeparam name="Hash">Hashing functor for an item of type KeyType</typeparam>
/// <typeparam name="Equals">Equality functor for an item of type KeyType</typeparam>
template <class KeyType, class Hash = std::hash<KeyType>, class Equals = std::equal_to<KeyType>>
class Set
{
private:
	enum class State
	{
		EMPTY,
		OCCUPIED,
		DELETED
	};

	/// <summary>Array of positions -- each element points to the next one in the "bucket"</summary>
	int* next_;
	KeyType* data_;

	/// <summary> First empty (not deleted) position </summary>
	int64_t firstEmpty_;
	State* state_;

	/// <summary> The number of <code>State::OCCUPIED</code> cells </summary>
	size_t size_;

	/// <summary> The number of cells that are not of state <code>State::EMPTY</code></summary>
	size_t sizeWithDeleted_;
	size_t capacity_;

	Hash hash_;
	Equals equals_;

	void resize(size_t newCapacity);
public:
	/// <summary> Initializes an empty Set with initial capacity 1 </summary>
	Set() : next_(new int[1]{ -1 }), state_(new State[1]{ State::EMPTY }), data_(new KeyType[1]), firstEmpty_(0), size_(0), sizeWithDeleted_(0), capacity_(1) {}
	
	/// <summary> Initializes an empty Set with a custom initial capacity </summary>
	/// <param name="initial_size"> The initial capacity </param>
	Set(size_t initial_size);
	
	~Set()
	{
		delete[] next_;
		delete[] state_;
		delete[] data_;
	}
	/// <summary>
	/// Const iterator for a Set. May become invalid/give errors after inserting data into the set; to check if an insertion 
	/// would render the iterators invalid, use <see cref="willInvalidateIteratorsOnInsert">willInvalidateIteratorsOnInsert()</see>
	/// </summary>
	class Iterator
	{
	public:
		friend class Set;

		typedef std::input_iterator_tag iterator_category;
		typedef KeyType value_type;
		typedef std::ptrdiff_t difference_type;
		typedef const KeyType* pointer;
		typedef const KeyType& reference;

		explicit Iterator(const KeyType* elem, const State* state, const KeyType* end) : current_(elem), currentState_(state), end_(end) {};

		Iterator& operator++();
		Iterator operator++(int);

		bool operator==(Iterator other) const;
		bool operator!=(Iterator other) const;

		const reference operator*() const;
		const pointer operator->() const;
	private:
		const KeyType* end_;
		const KeyType* current_;
		const State* currentState_;
	};

	typedef Iterator const_iterator_type;

	/// <summary>Inserts a copy of an item into the set</summary>
	/// <param name="item"> The item to insert </param>
	/// <returns>
	/// An iterator to an item equal to the parameter and whether the insertion happened (true) 
	/// or a similar item was already in the set (false)
	/// </returns>
	std::pair<Iterator, bool> insert(const KeyType& item);
	
	/// <summary>Inserts an item into the set</summary>
	/// <param name="item"> The item to insert </param>
	/// <returns>
	/// An iterator to an item equal to the parameter and whether the insertion happened (true) 
	/// or a similar item was already in the set (false)
	/// </returns>
	std::pair<Iterator, bool> insert(KeyType&& item);

	/// <summary>Searches for an item in the set</summary>
	/// <param name="item">The item to search for</param>
	/// <returns>An iterator to the similar item in the set or <see cref="end">end()</see></returns>
	Iterator find(const KeyType& item) const;

	/// <summary>Removes the item at an iterator</summary>
	/// <param name="iterator">The position of the item to remove</param>
	/// <remarks>Throws std::runtime_error if the iterator points outside the container (e.g. is equal to end())</remarks>
	void erase(Iterator iterator);

	/// <returns>The number of items in the set</returns>
	size_t size() { return size_; };

	/// <returns>The capacity of the set</returns>
	/// <remarks>The number of insertions (without subtracting deletions) is compared with the capacity when resizing</remarks>
	size_t capacity() { return capacity_; };

	/// <returns>true if the next insert would trigger a resize, therefore invalidating the iterators</returns>
	bool willInvalidateIteratorsOnInsert() const;

	Iterator begin() const;
	Iterator end() const;

private:
	template <typename refType>
	std::pair<Iterator, bool> insert_(refType item);
};

#pragma region Set Methods

template<class KeyType, class Hash, class Equals>
inline Set<KeyType, Hash, Equals>::Set(size_t initial_size) :
	next_(new int[initial_size]),
	state_(new State[initial_size]),
	data_(new KeyType[initial_size]),
	firstEmpty_(0),
	size_(0),
	capacity_(initial_size)
{
	std::fill_n(next_, initial_size, -1);
	std::fill_n(state_, initial_size, State::EMPTY);
}

template<class KeyType, class Hash, class Equals>
void Set<KeyType, Hash, Equals>::resize(size_t newCapacity)
{
	KeyType* newData = new KeyType[newCapacity];
	State* newState = new State[newCapacity];
	int* newPointers = new int[newCapacity];

	std::fill_n(newState, newCapacity, State::EMPTY);
	std::fill_n(newPointers, newCapacity, -1);

	size_t newFirstEmpty = 0;

	for (int i = 0; i < capacity_; ++i)
	{
		if (state_[i] != State::OCCUPIED)
			continue;
		auto item = data_[i];
		auto hash = hash_(item);
		auto pos = hash % newCapacity;
		if (newState[pos] == State::EMPTY)
		{
			newData[pos] = std::move(item);
			newState[pos] = State::OCCUPIED;
			if (newFirstEmpty == pos)
				while (newState[newFirstEmpty] != State::EMPTY)
					newFirstEmpty = (newFirstEmpty + 1) % newCapacity;
		}
		else
		{
			while (newPointers[pos] != -1)
				pos = newPointers[pos];
			newData[newFirstEmpty] = std::move(item);
			newState[newFirstEmpty] = State::OCCUPIED;
			newPointers[pos] = newFirstEmpty;
			while (newState[newFirstEmpty] != State::EMPTY)
				newFirstEmpty = (newFirstEmpty + 1) % newCapacity;
		}
	}

	delete[] data_;
	delete[] state_;
	delete[] next_;

	data_ = newData;
	state_ = newState;
	next_ = newPointers;
	capacity_ = newCapacity;
	firstEmpty_ = newFirstEmpty;
	sizeWithDeleted_ = size_;
}

template<class KeyType, class Hash, class Equals>
template<typename refType>
inline std::pair<typename Set<KeyType, Hash, Equals>::Iterator, bool> Set<KeyType, Hash, Equals>::insert_(refType item)
{
	if (sizeWithDeleted_ >= capacity_)
		resize(capacity_ * 2);
	size_t hash = hash_(item);
	auto pos = hash % capacity_;
	if (state_[pos] != State::OCCUPIED)
	{
		data_[pos] = item;
		state_[pos] = State::OCCUPIED;
		if (firstEmpty_ == pos)
		{
			if (capacity_ == sizeWithDeleted_ + 1)
				firstEmpty_ = -1;
			else
				while (state_[firstEmpty_] != State::EMPTY)
					firstEmpty_ = (firstEmpty_ + 1) % capacity_;
		}
		++size_;
		++sizeWithDeleted_;
		return { Set<KeyType, Hash, Equals>::Iterator(data_ + pos, state_ + pos, data_ + capacity_), true };
	}
	while (next_[pos] != -1)
	{
		if (state_[pos] == State::OCCUPIED && equals_(data_[pos], item))
			return { Set<KeyType, Hash, Equals>::Iterator(data_ + pos, state_ + pos, data_ + capacity_), false };
		pos = next_[pos];
	}
	if (state_[pos] == State::OCCUPIED && equals_(data_[pos], item))
		return { Set<KeyType, Hash, Equals>::Iterator(data_ + pos, state_ + pos, data_ + capacity_), false };
	data_[firstEmpty_] = item;
	state_[firstEmpty_] = State::OCCUPIED;
	std::pair<Set<KeyType, Hash, Equals>::Iterator, bool> retVal = { Set<KeyType, Hash, Equals>::Iterator(data_ + firstEmpty_, state_ + firstEmpty_, data_ + capacity_), true };
	if (capacity_ == sizeWithDeleted_ + 1)
		firstEmpty_ = -1;
	else
		while (state_[firstEmpty_] != State::EMPTY)
			firstEmpty_ = (firstEmpty_ + 1) % capacity_;
	++sizeWithDeleted_;
	++size_;
	return retVal;
}

template<class KeyType, class Hash, class Equals>
std::pair<typename Set<KeyType, Hash, Equals>::Iterator, bool> Set<KeyType, Hash, Equals>::insert(KeyType&& item)
{
	return insert_<KeyType&&>(std::move(item));
}

template<class KeyType, class Hash, class Equals>
std::pair<typename Set<KeyType, Hash, Equals>::Iterator, bool> Set<KeyType, Hash, Equals>::insert(const KeyType& item)
{
	return insert_<const KeyType&>(item);
}

template<class KeyType, class Hash, class Equals>
inline typename Set<KeyType, Hash, Equals>::Iterator Set<KeyType, Hash, Equals>::find(const KeyType& item) const
{
	auto hash = hash_(item);
	auto pos = hash % capacity_;
	while (state_[pos] != State::EMPTY && next_[pos] != -1)
		if (data_[pos] == item)
			return Iterator(data_ + pos, state_ + pos, data_ + capacity_);
		else pos = next_[pos];
	if (data_[pos] == item)
		return Iterator(data_ + pos, state_ + pos, data_ + capacity_);
	return end();
}

template<class KeyType, class Hash, class Equals>
inline void Set<KeyType, Hash, Equals>::erase(Iterator iterator)
{
	if (iterator.current_ < data_ && iterator.current_ >= data_ + capacity_)
		throw std::runtime_error("Iterator is invalid");
	auto pos = iterator.current_ - data_;
	if (pos < firstEmpty_)
		firstEmpty_ = pos;
	state_[pos] = State::DELETED;
	--size_;
}

template<class KeyType, class Hash, class Equals>
inline bool Set<KeyType, Hash, Equals>::willInvalidateIteratorsOnInsert() const
{
	return sizeWithDeleted_ >= capacity_;
}

template<class KeyType, class Hash, class Equals>
inline typename Set<KeyType, Hash, Equals>::Iterator Set<KeyType, Hash, Equals>::begin() const
{
	if (state_[0] != State::OCCUPIED)
		return ++Iterator(data_, state_, data_ + capacity_);
	return Iterator(data_, state_, data_ + capacity_);
}

template<class KeyType, class Hash, class Equals>
inline typename Set<KeyType, Hash, Equals>::Iterator Set<KeyType, Hash, Equals>::end() const
{
	return Iterator(data_ + capacity_, state_ + capacity_, data_ + capacity_);
}

#pragma endregion


#pragma region Set Iterator Methods

template<class KeyType, class Hash, class Equals>
inline typename Set<KeyType, Hash, Equals>::Iterator& Set<KeyType, Hash, Equals>::Iterator::operator++()
{
	if (current_ >= end_)
		throw std::runtime_error("Reached the end of the container");
	do
	{
		++current_;
		++currentState_;
	} while (current_ < end_ && *currentState_ != State::OCCUPIED);

	return *this;
}

template<class KeyType, class Hash, class Equals>
inline typename Set<KeyType, Hash, Equals>::Iterator Set<KeyType, Hash, Equals>::Iterator::operator++(int)
{
	auto* retVal = this;
	++(*this);
	return retVal;
}

template<class KeyType, class Hash, class Equals>
inline bool Set<KeyType, Hash, Equals>::Iterator::operator==(Iterator other) const
{
	return current_ == other.current_ && end_ == other.end_;
}

template<class KeyType, class Hash, class Equals>
inline bool Set<KeyType, Hash, Equals>::Iterator::operator!=(Iterator other) const
{
	return !(*this == other);
}

template<class KeyType, class Hash, class Equals>
inline const Set<KeyType, Hash, Equals>::Iterator::reference Set<KeyType, Hash, Equals>::Iterator::operator*() const
{
	return *current_;
}

template<class KeyType, class Hash, class Equals>
inline const Set<KeyType, Hash, Equals>::Iterator::pointer Set<KeyType, Hash, Equals>::Iterator::operator->() const
{
	return current_;
}

#pragma endregion

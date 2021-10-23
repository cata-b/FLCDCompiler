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
	size_t capacity_;

	Hash hash_;
	Equals equals_;

	void resize(size_t newCapacity);
public:
	/// <summary> Initializes an empty Set with initial capacity 1 </summary>
	Set() : next_(new int[1]{ -1 }), state_(new State[1]{ State::EMPTY }), data_(new KeyType[1]), firstEmpty_(0), size_(0), capacity_(1) {}
	
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

		explicit Iterator(const KeyType* data_begin, const State* state_begin, size_t index, size_t max) : 
			data_begin_(data_begin), 
			state_begin_(state_begin),
			index_(index),
			max_(max)
		{};

		Iterator(const Iterator& other);
		Iterator(Iterator&& other);

		Iterator& operator++();
		Iterator operator++(int);

		Iterator& operator=(const Iterator& other);
		Iterator& operator=(Iterator&& other);

		bool operator==(Iterator other) const;
		bool operator!=(Iterator other) const;

		const reference operator*() const;
		const pointer operator->() const;

		size_t index() const;
	private:
		const KeyType* data_begin_;
		const State* state_begin_;
		size_t index_;
		size_t max_;
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
	size_t size() const { return size_; };

	/// <returns>The capacity of the set</returns>
	/// <remarks>The number of insertions (without subtracting deletions) is compared with the capacity when resizing</remarks>
	size_t capacity() const { return capacity_; };

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
	KeyType* oldData = data_;
	State* oldState = state_;
	size_t oldCapacity = capacity_;

	data_ = new KeyType[newCapacity];
	state_ = new State[newCapacity];
	delete[] next_;
	next_ = new int[newCapacity];
	capacity_ = newCapacity;
	size_ = 0;
	firstEmpty_ = 0;

	std::fill_n(state_, newCapacity, State::EMPTY);
	std::fill_n(next_, newCapacity, -1);

	for (int i = 0; i < oldCapacity; ++i)
		if (oldState[i] == State::OCCUPIED)
			insert(oldData[i]);

	delete[] oldData;
	delete[] oldState;
}

template<class KeyType, class Hash, class Equals>
template<typename refType>
inline std::pair<typename Set<KeyType, Hash, Equals>::Iterator, bool> Set<KeyType, Hash, Equals>::insert_(refType item)
{
	if (firstEmpty_ >= capacity_)
		resize(capacity_ * 2);
	size_t hash = hash_(item);

	auto pos = hash % capacity_;
	
	if (state_[pos] == State::EMPTY)
	{
		state_[pos] = State::OCCUPIED;
		data_[pos] = item;
		next_[pos] = -1;
		if (pos == firstEmpty_)
			while (firstEmpty_ < capacity_ && state_[firstEmpty_] != State::EMPTY)
				++firstEmpty_;
		++size_;
		return { Iterator{data_, state_, pos, capacity_}, true };
	}
	
	auto current = pos;
	while (next_[current] != -1)
	{
		if (equals_(data_[current], item))
			return { Iterator{data_, state_, current, capacity_}, false };
		current = next_[current];
	}
	if (equals_(data_[current], item))
		return { Iterator{data_, state_, current, capacity_}, false };
	data_[firstEmpty_] = item;
	state_[firstEmpty_] = State::OCCUPIED;
	next_[current] = firstEmpty_;
	next_[firstEmpty_] = -1;

	auto retVal = std::make_pair(Iterator{data_, state_, (size_t)firstEmpty_, capacity_}, true);

	while (firstEmpty_ < capacity_ && state_[firstEmpty_] != State::EMPTY)
		++firstEmpty_;
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
	while (next_[pos] != -1)
	{
		if (state_[pos] == State::OCCUPIED && equals_(data_[pos], item))
			return Iterator{ data_, state_, pos, capacity_ };
		pos = next_[pos];
	};
	if (state_[pos] == State::OCCUPIED && equals_(data_[pos], item))
		return Iterator{ data_, state_, pos, capacity_ };
	return end();
}

template<class KeyType, class Hash, class Equals>
inline void Set<KeyType, Hash, Equals>::erase(Iterator iterator)
{
	if (iterator.data_begin_ != data_ || iterator.state_begin_ != state_ || iterator.index_ >= capacity_ || iterator.max_ != capacity_)
		throw std::runtime_error("Iterator is invalid");
	state_[iterator.index_] = State::DELETED;
	--size_;
}

template<class KeyType, class Hash, class Equals>
inline bool Set<KeyType, Hash, Equals>::willInvalidateIteratorsOnInsert() const
{
	return firstEmpty_ >= capacity_;
}

template<class KeyType, class Hash, class Equals>
inline typename Set<KeyType, Hash, Equals>::Iterator Set<KeyType, Hash, Equals>::begin() const
{
	if (state_[0] != State::OCCUPIED)
		return ++Iterator(data_, state_, 0, capacity_);
	return Iterator(data_, state_, 0, capacity_);
}

template<class KeyType, class Hash, class Equals>
inline typename Set<KeyType, Hash, Equals>::Iterator Set<KeyType, Hash, Equals>::end() const
{
	return Iterator(data_, state_, capacity_, capacity_);
}

#pragma endregion


#pragma region Set Iterator Methods

template<class KeyType, class Hash, class Equals>
inline Set<KeyType, Hash, Equals>::Iterator::Iterator(const Iterator& other)
{
	data_begin_ = other.data_begin_;
	state_begin_ = other.state_begin_;
	index_ = other.index_;
	max_ = other.max_;
}

template<class KeyType, class Hash, class Equals>
inline Set<KeyType, Hash, Equals>::Iterator::Iterator(Iterator&& other)
{
	data_begin_ = other.data_begin_;
	state_begin_ = other.state_begin_;
	index_ = other.index_;
	max_ = other.max_;
	other.index_ = max_;
}

template<class KeyType, class Hash, class Equals>
inline typename Set<KeyType, Hash, Equals>::Iterator& Set<KeyType, Hash, Equals>::Iterator::operator++()
{
	if (index_ >= max_)
	{
		index_ = max_;
		throw std::runtime_error("Reached the end of the container");
	}
	do
	{
		++index_;
	} while (index_ < max_ && state_begin_[index_] != State::OCCUPIED);

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
inline typename Set<KeyType, Hash, Equals>::Iterator& Set<KeyType, Hash, Equals>::Iterator::operator=(const Iterator& other)
{
	data_begin_ = other.data_begin_;
	state_begin_ = other.state_begin_;
	index_ = other.index_;
	max_ = other.max_;
	return *this;
}

template<class KeyType, class Hash, class Equals>
inline typename Set<KeyType, Hash, Equals>::Iterator& Set<KeyType, Hash, Equals>::Iterator::operator=(Iterator&& other)
{
	data_begin_ = other.data_begin_;
	state_begin_ = other.state_begin_;
	index_ = other.index_;
	max_ = other.max_;
	other.index_ = max_; // turn other into end()
	return *this;
}

template<class KeyType, class Hash, class Equals>
inline bool Set<KeyType, Hash, Equals>::Iterator::operator==(Iterator other) const
{
	return 
		data_begin_ == other.data_begin_ &&
		state_begin_ == other.state_begin_ &&
		index_ == other.index_ &&
		max_ == other.max_;
}

template<class KeyType, class Hash, class Equals>
inline bool Set<KeyType, Hash, Equals>::Iterator::operator!=(Iterator other) const
{
	return !(*this == other);
}

template<class KeyType, class Hash, class Equals>
inline const Set<KeyType, Hash, Equals>::Iterator::reference Set<KeyType, Hash, Equals>::Iterator::operator*() const
{
	return data_begin_[index_];
}

template<class KeyType, class Hash, class Equals>
inline const Set<KeyType, Hash, Equals>::Iterator::pointer Set<KeyType, Hash, Equals>::Iterator::operator->() const
{
	return data_begin_ + index_;
}

template<class KeyType, class Hash, class Equals>
inline size_t Set<KeyType, Hash, Equals>::Iterator::index() const
{
	return index_;
}

#pragma endregion

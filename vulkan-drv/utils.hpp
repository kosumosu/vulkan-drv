#pragma once
#include <array>
#include <boost/range/iterator_range.hpp>

#include <unordered_set>
#include <optional>
#include <boost/optional/optional.hpp>

namespace utils
{
	template <class TStream>
	void output(TStream& stream)
	{
	}

	template <class TStream, class TArg, class ...TArgs>
	void output(TStream& stream, const TArg& arg, const TArgs& ...args)
	{
		stream << arg;
		output(stream, args...);
	}

	template <typename TElement, size_t _Size>
	constexpr std::array<TElement, _Size> make_array(TElement (& array)[_Size])
	{
		return std::array<TElement, _Size>{array};
	}

	template <typename TElement, typename ... TElements>
	constexpr std::array<TElement, sizeof...(TElements)> make_array(TElements&& ... elements)
	{
		static_assert(std::conjunction<std::is_constructible<TElement, TElements>...>::value,
			"Array element type must be constructible from each of the passed item types");
		return std::array<TElement, sizeof...(TElements)>{elements...};
	}

	template <typename TElement, size_t _Size>
	constexpr size_t array_size(TElement (&array)[_Size])
	{
		return _Size;
	}

	template <class TRange>
	auto to_unordered_set(const TRange& range)
	{
		return boost::copy_range<std::unordered_set<typename TRange::value_type>>(range);
	}

	template <class TIteratorBegin, class TIteratorEnd, class TPredicate>
	auto maybeFirst(
		TIteratorBegin begin,
		TIteratorEnd end,
		const TPredicate& predicate) -> boost::optional<typename TIteratorBegin::reference>
	// Have to use boost::optional because C++17 std::optional does not support references
	
	{
		const auto it = std::find_if(begin, end, predicate);
		if (it == end)
		{
			return boost::none;
		}
		else
		{
			return *it;
		}
	}

	template <class TCollection, class TPredicate>
	auto maybeFirst(TCollection collection, const TPredicate& predicate)
	{
		return maybeFirst(std::begin(collection), std::end(collection), predicate);
	}

	template <class TCollection, class TPredicate>
	bool contains(TCollection collection, const TPredicate& predicate)
	{
		return maybeFirst(std::begin(collection), std::end(collection), predicate);
	}
}

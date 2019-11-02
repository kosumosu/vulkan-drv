#pragma once
#include <array>


namespace utils
{
	template <typename TElement, size_t _Size>
	constexpr std::array<TElement, _Size> make_array(TElement (& array)[_Size])
	{
		return std::array<TElement, _Size>{array};
	}

	template <typename TElement, typename ... TElements, class = std::enable_if_t<std::conjunction<std::is_constructible<TElement, TElements>...>::value>>
	constexpr std::array<TElement, sizeof...(TElements)> make_array(TElements&& ... elements)
	{
		return std::array<TElement, sizeof...(TElements)>{elements...};
	}

	template <typename TElement, size_t _Size>
	constexpr size_t array_size(TElement (&array)[_Size])
	{
		return _Size;
	}
}

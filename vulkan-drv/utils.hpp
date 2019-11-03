#pragma once
#include <array>


namespace utils
{
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

	namespace detail
	{
		// An iterator that is analogous to boost ranges indexed iterator adapter
		// https://www.boost.org/doc/libs/1_68_0/libs/range/doc/html/range/reference/adaptors/reference/indexed.html
		// Though, it does not fully satisfy C++ Iterator named requirement yet
		// https://en.cppreference.com/w/cpp/named_req/Iterator
		template <class TBaseIterator>
		class IndexedCollectionWrapper
		{
		public:
			using base_reference = typename TBaseIterator::reference;
			using reference = std::pair<size_t, base_reference>;
			
			class iterator
			{
			public:
				iterator(TBaseIterator current, size_t index)
					: current_(std::move(current))
					, index_(index)
				{
				}

				iterator& operator ++() noexcept(noexcept(++current_))
				{
					++current_;
					++index_;
					return *this;
				}

				bool operator !=(const iterator& other)
				{
					return current_ != other.current_;
				}

				bool operator ==(const iterator& other)
				{
					return current_ == other.current_;
				}

				reference operator*()
				{
					return reference(index_, *current_);
				}

			private:
				TBaseIterator current_;
				size_t index_;
			};

			IndexedCollectionWrapper(TBaseIterator begin, TBaseIterator end)
				: begin_(std::move(begin))
				, end_(std::move(end))
			{
			}

			[[nodiscard]] iterator begin() const
			{
				return iterator(begin_, 0);
			}

			[[nodiscard]] iterator end() const
			{
				return iterator(end_, 0); // Index here makes no sense
			}

		private:
			TBaseIterator begin_;
			TBaseIterator end_;
		};
	}

	template <class TCollection>
	auto indexed(const TCollection& collection)
	{
		return detail::IndexedCollectionWrapper<typename TCollection::const_iterator>(collection.begin(), collection.end());
	}
}

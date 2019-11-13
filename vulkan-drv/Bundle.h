#pragma once

#include <boost/noncopyable.hpp>

#include <tuple>

// Since we cannot return structs that have pointers to other temporal structs from functions,
// we need a helper that will bundle up all the stuff at fix the addresses.
// So this is it.
template <class TMain, class ...TDependencies>
class Bundle
{
	const std::tuple<TDependencies...> dependencies_;
	const TMain main_;

public:
	template <class TMainFactory, class ...TDependencies2>
	Bundle(const TMainFactory& mainFactory, TDependencies2&& ... dependencies)
		: dependencies_(std::forward<TDependencies2>(dependencies)...)
		//, main_(callFactory(mainFactory, dependencies_, std::index_sequence_for<TDependencies...>()))
		, main_(std::apply(mainFactory, dependencies_))
	{
	}

	Bundle(const Bundle& other) = delete;
	Bundle(Bundle&& other) = delete;
	Bundle& operator=(const Bundle& other) = delete;
	Bundle& operator=(Bundle&& other) = delete;

	TMain& operator*()
	{
		return main_;
	}

	const TMain& operator*() const
	{
		return main_;
	}

	TMain* operator->()
	{
		return &main_;
	}

	const TMain* operator->() const
	{
		return &main_;
	}

	TMain* get()
	{
		return &main_;
	}

	const TMain* get() const
	{
		return &main_;
	}

private:
	template <class TMainFactory, class TTuple, std::size_t ...TIndices>
	auto callFactory(const TMainFactory& mainFactory, const TTuple& tuple, std::index_sequence<TIndices...>) const
	{
		return mainFactory(std::get<TIndices>(tuple)...);
	}
};


template <class TMainFactory, class ...TDependencies>
auto makeBundle(const TMainFactory& mainFactory, TDependencies&& ... dependencies)
{
	using TMain2 = decltype(mainFactory(std::forward<TDependencies>(dependencies)...));
	return Bundle<TMain2, std::decay_t<TDependencies>...>(mainFactory, std::forward<TDependencies>(dependencies)...);
}

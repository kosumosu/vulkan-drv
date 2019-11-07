#pragma once

#include <memory>

template <class TParent, class TDeleter>
class SelfDestroyable : public TParent
{
public:
	template <class ...TArgs>
	SelfDestroyable(TArgs&& ... args, TDeleter deleter)
		: TParent(args)
		, deleter_(std::move(deleter))
	{
	}

	SelfDestroyable(const TParent& parent, TDeleter deleter)
		: TParent(parent)
		, deleter_(std::move(deleter))
	{
	}

	SelfDestroyable(TParent&& parent, TDeleter deleter)
		: TParent(std::move(parent))
		, deleter_(std::move(deleter))
	{
	}

	~SelfDestroyable()
	{
		deleter_(static_cast<TParent&>(*this));
	}

	SelfDestroyable(const SelfDestroyable& other) = delete;
	SelfDestroyable(SelfDestroyable&& other) noexcept = default;
	SelfDestroyable& operator=(const SelfDestroyable& other) = delete;
	SelfDestroyable& operator=(SelfDestroyable&& other) noexcept = default;
private:
	TDeleter deleter_;
};


template <class TParent, class TDeleter>
SelfDestroyable<TParent, TDeleter> makeSelfDestroyable(TParent&& parent, TDeleter deleter)
{
	return SelfDestroyable<TParent, TDeleter>(std::forward<TParent>(parent), std::move(deleter));
}
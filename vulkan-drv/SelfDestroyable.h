#pragma once

#include <memory>

template <class TParent, class TDeleter>
class SelfDestroyable
{
	TParent parent_;
	TDeleter deleter_;
	bool isEmpty_ = true;

public:
	SelfDestroyable(const TParent& parent, TDeleter deleter)
		: parent_(parent)
		, deleter_(std::move(deleter))
		, isEmpty_(false)
	{
	}

	SelfDestroyable(TParent&& parent, TDeleter deleter)
		: parent_(std::move(parent))
		, deleter_(std::move(deleter))
		, isEmpty_(false)
	{
	}

	~SelfDestroyable()
	{
		if (!isEmpty_)
			deleter_(parent_);
	}

	TParent& operator *()
	{
		return parent_;
	}

	const TParent& operator *() const
	{
		return parent_;
	}

	TParent* operator ->()
	{
		return &parent_;
	}

	const TParent* operator ->() const
	{
		return &parent_;
	}

	SelfDestroyable(const SelfDestroyable& other) = delete;

	SelfDestroyable(SelfDestroyable&& other) noexcept
		: parent_(std::move(other.parent_))
		, deleter_(std::move(other.deleter_))
		, isEmpty_(false)
	{
		other.isEmpty_ = true;
	}

	SelfDestroyable& operator=(const SelfDestroyable& other) = delete;
	SelfDestroyable& operator=(SelfDestroyable&& other) noexcept = delete;
};


template <class TParent, class TDeleter>
SelfDestroyable<TParent, TDeleter> makeSelfDestroyable(TParent&& parent, TDeleter deleter)
{
	return SelfDestroyable<TParent, TDeleter>(std::forward<TParent>(parent), std::move(deleter));
}

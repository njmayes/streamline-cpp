#pragma once

#include "SL/Core/Common/Base.h"
#include "SL/Core/Logging/Log.h"

namespace sl {

	enum class ModalButtons
	{
		None,
		OK,
		OKCancel,
		YesNo,
		Custom
	};

	class IModal : public RefCounted
	{
	public:
		IModal();
		virtual ~IModal();

		virtual void OnOverlayRender() = 0;
		virtual void OnComplete()
		{}
		virtual void OnCustomButtonRender( bool& open )
		{
			throw std::logic_error( "You must provide an override for this function if using custom button behaviour!" );
		}

		void AddCompletionCallback( std::function< void() > const& function )
		{
			mCompletionCallbacks.emplace_back( function );
		}

	private:
		std::vector< std::function< void() > > mCompletionCallbacks;
		friend class ModalManager;
	};

	template < typename T >
	concept IsModal = DerivedFromOnly< T, IModal >;

	class InlineModal : public IModal
	{
	public:
		InlineModal( Action<>&& on_overlay_render, Action<>&& on_complete )
			: mOnOverlayRender( std::move( on_overlay_render ) ), mOnComplete( std::move( on_complete ) )
		{}

	private:
		void OnOverlayRender() override
		{
			mOnOverlayRender();
		}
		void OnComplete() override
		{
			mOnComplete();
		}

	private:
		Action<> mOnOverlayRender, mOnComplete;
	};
} // namespace sl
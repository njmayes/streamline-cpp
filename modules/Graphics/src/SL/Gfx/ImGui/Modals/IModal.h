#pragma once

#include "SL/Core/Common/Application.h"
#include "SL/Core/Logging/Log.h"
#include "SL/Core/Types/Math.h"

namespace sl {

	enum class ModalButtons
	{
		None,
		OK,
		OKCancel,
		YesNo,
		Custom
	};

	class IModal : public ApplicationEventListener
	{
	public:
		IModal() = default;
		virtual ~IModal() = default;

		virtual void OnOverlayRender() = 0;
		virtual void OnComplete()
		{}
		virtual void OnClose()
		{}

		virtual void OnCustomButtonRender( bool& open, Vec2f const& size )
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
		InlineModal( Action<> on_overlay_render, Action<> on_complete, Action<> on_close, Action<IModal*, bool&, Vec2f const& > on_button_render = {} )
			: mOnOverlayRender( std::move( on_overlay_render ) )
			, mOnComplete( std::move( on_complete ) )
			, mOnClose( std::move( on_close ) )
			, mOnButtonRender( std::move( on_button_render ) )
		{}

		void OnOverlayRender() override
		{
			mOnOverlayRender();
		}
		void OnComplete() override
		{
			mOnComplete();
		}
		void OnClose() override
		{
			mOnClose();
		}

		void OnCustomButtonRender( bool& open, Vec2f const& size ) override
		{
			if ( !mOnButtonRender )
				throw std::logic_error( "You must provide a button renderer if using custom button behaviour!" );

			mOnButtonRender( this, open, size );
		}

		void OnEvent( Event& e ) override
		{}

	private:
		Action<> mOnOverlayRender, mOnComplete, mOnClose;
		Action< IModal*, bool&, Vec2f const& > mOnButtonRender;
	};
} // namespace sl
#pragma once

#include "slc/Common/Base.h"
#include "slc/Logging/Log.h"

namespace slc {

	enum class ModalButtons
	{
		None,
		OK,
		OKCancel,
		YesNo,
		Custom
	};

	class IModalWindow : public RefCounted
	{
	public:
		IModalWindow();
		virtual ~IModalWindow();

	private:
		virtual void OnOverlayRender() = 0;
		virtual void OnComplete()
		{}
		virtual void OnCustomButtonRender( bool& open )
		{
			throw std::logic_error( "You must provide an override for this function if using custom button behaviour!" );
		}

	private:
		friend class ModalManager;
	};

	template < typename T >
	concept IsEditorModal = DerivedFromOnly< T, IModalWindow >;

	class WarningModal : public IModalWindow
	{
	public:
		WarningModal( const std::string& msg )
			: IModalWindow(), mMessage( msg )
		{}

	private:
		void OnOverlayRender() override;

	private:
		std::string mMessage;
	};

	class InlineModal : public IModalWindow
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
} // namespace slc
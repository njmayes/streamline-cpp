#pragma once

#include "IModalWindow.h"

namespace slc {

	class ModalManager
	{
	private:
		struct ModalEntry
		{
			std::string_view heading;
			Box< IModalWindow > modal = nullptr;
			ModalButtons type = ModalButtons::OKCancel;

			bool open = true;

			ModalEntry( std::string_view h, ModalButtons t )
				: heading( h ), type( t )
			{}

			template < typename T, typename... Args >
			void Init( Args&&... args )
			{
				modal = MakeBox< T >( std::forward< Args >( args )... );
			}
		};

	public:
		template < IsEditorModal T, typename... Args >
		void Open( std::string_view title, ModalButtons type, Args&&... args )
		{
			if ( Contains( title ) )
			{
				log::Error( "Modal already open!" );
				return;
			}

			ModalEntry& entry = mEditorModals.emplace_back( title, type );
			entry.Init< T >( std::forward< Args >( args )... );
			mLastAdded = entry.heading;
		}

		void OpenInline( std::string_view title, ModalButtons type, Action<>&& on_overlay_render, Action<>&& on_complete = [] {} )
		{
			Open< InlineModal >( title, type, std::move( on_overlay_render ), std::move( on_complete ) );
		}
		void OpenWarning( std::string_view title, const std::string& msg )
		{
			Open< WarningModal >( title, ModalButtons::OK, msg );
		}

		void AddCallback( std::function< void() > function )
		{
			mModalCallbacks[ mLastAdded ].emplace_back( function );
		}

		void Render();

	private:
		void RenderButtons( ModalEntry& modalData );

		auto Find( std::string_view key )
		{
			return std::ranges::find_if( mEditorModals, [ &key ]( const ModalEntry& panel ) { return key == panel.heading; } );
		}

		bool Contains( std::string_view key )
		{
			return Find( key ) != mEditorModals.end();
		}

		void Clear()
		{
			mEditorModals.clear();
		}

	private:
		std::vector< ModalEntry > mEditorModals;

		using CallbackMap = std::unordered_map< std::string_view, std::vector< std::function< void() > > >;
		CallbackMap mModalCallbacks;
		std::string_view mLastAdded;
	};
} // namespace slc
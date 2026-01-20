#pragma once

#include "IModal.h"

#include "SL/Core/Types/Math.h"

namespace sl {

	struct ModalConstructionData
	{
		static constexpr Vec2f DefaultSize = { 600, 400 };
		static constexpr Vec2f DefaultButtonSize = { 60, 40 };
		static constexpr float DefaultFontScale = 2.0f;

		std::string_view heading;

		Vec2f size = DefaultSize;
		Vec2f button_size = DefaultButtonSize;
		float font_scale = DefaultFontScale;

		ModalButtons button_type = ModalButtons::OKCancel;
	};

	class ModalManager
	{
	private:
		struct ModalEntry
		{
			ModalConstructionData init_data{};
			Ref< IModal > modal = nullptr;

			bool open = true;

			ModalEntry( ModalConstructionData data, Ref< IModal > modal )
				: init_data( data )
				, modal( modal )
			{}
		};

	public:
		template < IsModal T, typename... Args >
		Ref< IModal > Open( ModalConstructionData const& init_data, Args&&... args )
		{
			auto it = Find( init_data.heading );
			if ( it != mEditorModals.end() )
			{
				log::Warn( "Modal {} already open!", init_data.heading );
				return nullptr;
			}

			Ref< T > new_modal = Ref< T >::Create( std::forward< Args >( args )... );
			ModalEntry& entry = mEditorModals.emplace_back( init_data, new_modal );

			return new_modal;
		}

		void AddCallback( std::string_view heading, std::function< void() > function )
		{
			auto it = Find( heading );
			if ( it == mEditorModals.end() )
			{
				log::Error( "Could not find modal to add callback to." );
				return;
			}

			it->modal->AddCompletionCallback( function );
		}

		void Render();

	private:
		void RenderButtons( ModalEntry& modalData );

		auto Find( std::string_view key ) -> std::vector< ModalEntry >::iterator
		{
			return std::ranges::find_if( mEditorModals, [ &key ]( const ModalEntry& panel ) { return key == panel.init_data.heading; } );
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
	};
} // namespace sl
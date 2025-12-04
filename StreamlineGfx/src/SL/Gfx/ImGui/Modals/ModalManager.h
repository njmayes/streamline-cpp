#pragma once

#include "IModal.h"

#include "SL/Core/Types/Math.h"

namespace slc {

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
			Box< IModal > modal = nullptr;

			bool open = true;

			ModalEntry( ModalConstructionData data )
				: init_data( data )
			{}

			template < typename T, typename... Args >
			void Init( Args&&... args )
			{
				modal = MakeBox< T >( std::forward< Args >( args )... );
			}
		};

	public:
		template < IsModal T, typename... Args >
		void Open( ModalConstructionData const& init_data, Args&&... args )
		{
			if ( Contains( init_data.heading ) )
			{
				log::Error( "Modal already open!" );
				return;
			}

			ModalEntry& entry = mEditorModals.emplace_back( init_data );
			entry.Init< T >( std::forward< Args >( args )... );
			mLastAdded = entry.init_data.heading;
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

		using CallbackMap = std::unordered_map< std::string_view, std::vector< std::function< void() > > >;
		CallbackMap mModalCallbacks;
		std::string_view mLastAdded;
	};
} // namespace slc
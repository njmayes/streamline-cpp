#pragma once

#include "IPanel.h"

#include "SL/Core/Types/Math.h"

namespace sl {

	struct PanelConstructionData
	{
		std::string_view key;
		Vec2f size{};
		bool show = true;
	};

	class PanelManager
	{
	private:
		struct PanelEntry
		{
			PanelConstructionData init_data{};
			Ref< IPanel > panel;
			bool displayed = true;

			PanelEntry() = default;
			PanelEntry( PanelConstructionData const& data, Ref< IPanel > p )
				: init_data( data )
				, panel( std::move( p ) )
				, displayed{ data.show }
			{}
		};

	public:
		template < IsPanel T, typename... Args >
		Ref< T > Open( PanelConstructionData const& init_data, Args&&... args )
		{
			auto it = Find( init_data.key );
			if ( it != mEditorPanels.end() )
			{
				log::Warn( "Panel {} already open!", init_data.key );
				return nullptr;
			}

			Ref< T > new_panel = Ref< T >::Create( std::forward< Args >( args )... );
			mEditorPanels.emplace_back( init_data, new_panel );

			return new_panel;
		}

		void Render();

	private:
		auto Find( std::string_view key ) -> std::vector< PanelEntry >::iterator;
		bool Contains( std::string_view key );

		void Delete( std::string_view key );
		void Clear()
		{
			mEditorPanels.clear();
		}

	private:
		std::vector< PanelEntry > mEditorPanels;
	};
} // namespace Laby
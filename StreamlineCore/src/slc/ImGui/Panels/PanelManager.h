#pragma once

#include "IPanel.h"

#include "slc/Types/Math.h"

namespace slc {

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

	private:
		std::vector< PanelEntry >& GetPanels()
		{
			return mEditorPanels;
		}

		void Render();

		PanelEntry* Find( std::string_view key );
		bool Contains( std::string_view key );

		void Delete( std::string_view key );
		void Clear()
		{
			mEditorPanels.clear();
		}

		template < IsPanel T, typename... Args >
		Ref< T > Open( PanelConstructionData const& init_data, Args&&... args )
		{
			ASSERT( !Contains( init_data.key ), "Can't register panel that is already being managed! (Check name is not already in use)" );

			Ref< T > new_panel = Ref< T >::Create( std::forward< Args >( args )... );
			mEditorPanels.emplace_back( init_data, new_panel );

			return new_panel;
		}

	private:
		std::vector< PanelEntry > mEditorPanels;
	};
} // namespace Laby
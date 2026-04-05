#pragma once

#include "NetworkLayer.h"

namespace sl::net {

	class ServerLayer : public NetLayer
	{
	public:
		ServerLayer( ServerContextOptions const& opts );

		void OnAttach() override;
		void OnDetach() override;

		bool OnEvent( Event& e ) override;

	private:
		virtual bool ShouldSend( ConnectionPtr connection, Payload const& payload )
		{
			return true;
		};

	private:
		void AddPort( std::uint16_t );
		bool SendMessage( NetworkOutEvent& e );

	private:
		ServerContextOptions mOptions;

		std::vector< ListenerHandle > mListeners;
		std::set< ConnectionPtr > mConnections;
	};
} // namespace sl::net
#pragma once

#include "NetworkLayer.h"

namespace sl::net {

	class ClientLayer : public NetLayer
	{
	public:
		ClientLayer( ClientContextOptions const& opts );

		void OnAttach() override;
		void OnDetach() override;

		void OnEvent( Event& e ) override;

	private:
		void Connect( std::string const& host, std::uint16_t port );

		bool SendMessage( NetworkOutEvent& e );

	private:
		ClientContextOptions mOptions;
		ConnectionPtr mServerConnection;
	};
} // namespace sl::net
#pragma once

#include "Context.h"
#include "Connection.h"

#include "slc/Common/Application.h"
#include "slc/Events/EventTypes.h"

namespace slc::net {

	class NetLayer : public ApplicationLayer
	{
	public:
		LISTENING_EVENTS( NetworkOut );

		template < ContextOptionsType options_t >
		NetLayer( options_t const& options )
			: mContext( options )
		{
		}

		void OnUpdate( Timestep )
		{}
		void OnRender() override
		{}
		void OnOverlayRender() override
		{}

		virtual void OnConnect( ConnectionPtr )
		{}
		virtual void OnDisconnect( ConnectionPtr )
		{}

		virtual void OnMessage( Payload const& )
		{}

	protected:
		Context mContext;
	};

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


	class ServerLayer : public NetLayer
	{
	public:
		ServerLayer( ServerContextOptions const& opts );

		void OnAttach() override;
		void OnDetach() override;

		void OnEvent( Event& e ) override;

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
} // namespace slc::net
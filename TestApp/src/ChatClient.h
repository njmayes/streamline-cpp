#pragma once

#include "SL/Core.h"
#include "SL/Gfx.h"

#include <deque>


class ChatLayer : public sl::ApplicationLayer
{
public:
	SLC_LISTENING_EVENTS( NetworkIn );

	void OnAttach() override
	{}
	void OnDetach() override
	{}

	void OnRender() override
	{}
	void OnOverlayRender() override;

	void OnUpdate( sl::Timestep );
	void OnEvent( sl::Event& e ) override
	{
		e.Dispatch< sl::NetworkInEvent >( SLC_BIND_EVENT_FUNC( OnMessageReceived ) );
	}

private:
	bool OnMessageReceived( sl::NetworkInEvent& e );
	bool SendMessage();

	void OpenUsernameEntryIfNeeded();

private:
	std::string mUsername;
	std::string mCurrentText;

	bool mInUsernameModal{};

	enum
	{
		max_recent_msgs = 100
	};
	std::deque< sl::net::Payload > mRecentMessages;
};


class ClientLayer : public sl::net::ClientLayer
{
public:
	ClientLayer( sl::net::ClientContextOptions const& opts );

	void OnConnect( sl::net::ConnectionPtr connection )
	{
		sl::Application::Get()->PushLayer< ChatLayer >();
	}
	void OnDisconnect( sl::net::ConnectionPtr )
	{}
};

class ChatClient : public sl::GuiApplication
{
public:
	ChatClient( sl::Ref< sl::GuiApplicationSpecification > spec, sl::net::ClientContextOptions const& opts );
};
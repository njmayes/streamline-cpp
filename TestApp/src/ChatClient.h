#pragma once

#include "streamline.h"

#include <deque>
#include <thread>


class ChatLayer : public slc::ApplicationLayer
{
public:
	LISTENING_EVENTS( NetworkIn );

	void OnAttach() override
	{}
	void OnDetach() override
	{}

	void OnRender() override
	{}
	void OnOverlayRender() override;

	void OnUpdate( slc::Timestep );
	void OnEvent( slc::Event& e ) override
	{
		e.Dispatch< slc::NetworkInEvent >( SLC_BIND_EVENT_FUNC( OnMessageReceived ) );
	}

private:
	bool OnMessageReceived( slc::NetworkInEvent& e );
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
	std::deque< slc::net::Payload > mRecentMessages;
};


class ClientLayer : public slc::net::ClientLayer
{
public:
	ClientLayer( slc::net::ClientContextOptions const& opts );

	void OnConnect( slc::net::ConnectionPtr connection )
	{
		slc::Application::Get().PushLayer< ChatLayer >();
	}
	void OnDisconnect( slc::net::ConnectionPtr )
	{}
};

class ChatClient : public slc::Application
{
public:
	ChatClient( slc::Box< slc::ApplicationSpecification > spec, slc::net::ClientContextOptions const& opts );
};
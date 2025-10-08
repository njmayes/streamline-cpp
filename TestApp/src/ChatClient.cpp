#include "ChatClient.h"

#include "slc/Common/Time.h"
#include "slc/Logging/Log.h"
#include "slc/ImGui/Widgets.h"

#include <iostream>

static std::string GetTimestamp()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t( now );

	std::tm time = slc::GetLocalTime( &now_c );

	std::string timestamp( 20, '\0' );
	std::strftime( timestamp.data(), timestamp.size(), "%F %T", &time );
	return timestamp;
}

void ChatLayer::OnOverlayRender()
{
	using slc::Widgets;

	Widgets::BeginWindow( "Chat Client", ImGuiWindowFlags_NoTitleBar );

	Widgets::BeginChild( "Messages", slc::Vec2f{ 0, -34.f } );

	slc::Utils::SetWindowFontScale( 2.0f );

	for ( auto const& line : mRecentMessages )
	{
		auto message = std::string_view{ line.As< char >(), line.Size() };
		Widgets::Label( message );
	}

	Widgets::EndChild();

	Widgets::BeginChild( "Input" );

	slc::Utils::SetWindowFontScale( 2.0f );

	int flags = ImGuiInputTextFlags_EnterReturnsTrue;
	if ( mUsername.empty() or mInUsernameModal )
		flags |= ImGuiInputTextFlags_ReadOnly;

	Widgets::StringEdit( "Message", mCurrentText, flags, [ this ] { SendMessage(); } );

	Widgets::EndChild();


	Widgets::EndWindow();
}

void ChatLayer::OnUpdate( slc::Timestep )
{
	OpenUsernameEntryIfNeeded();
}

bool ChatLayer::OnMessageReceived( slc::NetworkInEvent& e )
{
	mRecentMessages.emplace_back( e.data );
	return true;
}

bool ChatLayer::SendMessage()
{
	if ( mUsername.empty() )
		return false;

	auto timestamp = GetTimestamp();
	auto timestamp_view = std::string_view{ timestamp.data(), timestamp.size() - 1 };

	auto text = std::format( "{}: [{}] {}", timestamp_view, mUsername, mCurrentText );

	slc::net::Payload msg{};
	msg.Reserve( text.size() + 1 );
	msg.Append( text );
	msg.Push( '\0' );

	slc::Application::PostEvent< slc::NetworkOutEvent >( msg );

	mCurrentText.clear();
	slc::Utils::ReloadCurrentBuffer();
	slc::Utils::SetKeyboardFocusHere();

	return false;
}

void ChatLayer::OpenUsernameEntryIfNeeded()
{
	if ( !mUsername.empty() or mInUsernameModal )
		return;

	using slc::Widgets;

	mInUsernameModal = true;

	auto on_render = [ this ] {
		Widgets::StringEdit( "Please enter your username...", mUsername );
	};

	auto on_complete = [ this ] {
		mInUsernameModal = false;
	};

	auto cd = slc::ModalConstructionData{};
	cd.heading = "No username";
	cd.button_type = slc::ModalButtons::OK;

	slc::Application::OpenModal< slc::InlineModal >( cd, on_render, on_complete );
}

ClientLayer::ClientLayer( slc::net::ClientContextOptions const& opts )
	: slc::net::ClientLayer( opts )
{
}

ChatClient::ChatClient( slc::Box< slc::ApplicationSpecification > spec, slc::net::ClientContextOptions const& opts )
	: Application( std::move( spec ) )
{
	PushLayer< ClientLayer >( opts );
	AddLogTarget< slc::ConsoleLogTarget >( slc::LogLevel::Info );
}

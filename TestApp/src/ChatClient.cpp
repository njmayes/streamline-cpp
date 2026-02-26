#include "ChatClient.h"

#include "SL/Core/Common/Time.h"
#include "SL/Gfx/ImGui/Widgets.h"

struct TestStruct
{
	int a{};
	float b{};

	SL_REFLECT_CLASS( TestStruct, a, b );
};

struct TestNestedStruct
{
	uint8_t c{};
	double d{};
	TestStruct e{};

	SL_REFLECT_CLASS( TestNestedStruct, c, d, e );
};

static TestNestedStruct sTestStruct{};

static std::string GetTimestamp()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t( now );

	std::tm time = sl::GetLocalTime( &now_c );

	std::string timestamp( 20, '\0' );
	std::strftime( timestamp.data(), timestamp.size(), "%F %T", &time );
	return timestamp;
}

void ChatLayer::OnOverlayRender()
{
	using sl::Utils;
	using sl::Widgets;

	Widgets::BeginWindow( "Chat Client", ImGuiWindowFlags_NoTitleBar );

	Widgets::BeginChild( "Messages", sl::Vec2f{ 0, -34.f } );

	Utils::SetWindowFontScale( 2.0f );

	for ( auto const& line : mRecentMessages )
	{
		auto message = std::string_view{ line.As< char >(), line.Size() };
		Widgets::Label( message );
	}

	Widgets::EndChild();

	Widgets::BeginChild( "Input" );

	Utils::SetWindowFontScale( 2.0f );

	int flags = ImGuiInputTextFlags_EnterReturnsTrue;
	if ( mUsername.empty() or mInUsernameModal )
		flags |= ImGuiInputTextFlags_ReadOnly;

	Widgets::StringEdit( "Message", mCurrentText, flags, [ this ] { SendMessage(); } );

	Widgets::EndChild();


	Widgets::EndWindow();
}

void ChatLayer::OnUpdate( sl::Timestep )
{
	OpenUsernameEntryIfNeeded();
}

void ChatLayer::OnEvent( sl::Event& e )
{
	e.Dispatch< sl::NetworkInEvent >( SL_BIND_EVENT_FUNC( OnMessageReceived ) );
}

bool ChatLayer::OnMessageReceived( sl::NetworkInEvent& e )
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

	sl::net::Payload msg{};
	msg.Reserve( text.size() + 1 );
	msg.Append( text );
	msg.Push( '\0' );

	sl::Application::PostEvent< sl::NetworkOutEvent >( msg );

	mCurrentText.clear();
	sl::Utils::ReloadCurrentBuffer();
	sl::Utils::SetKeyboardFocusHere();

	return false;
}

void ChatLayer::OpenUsernameEntryIfNeeded()
{
	if ( !mUsername.empty() or mInUsernameModal )
		return;

	using sl::Widgets;

	mInUsernameModal = true;

	auto on_render = [ this ] {
		Widgets::StringEdit( "Please enter your username...", mUsername );
	};

	auto on_complete = [ this ] {
		mInUsernameModal = false;
	};

	auto cd = sl::ModalConstructionData{};
	cd.heading = "No username";
	cd.button_type = sl::ModalButtons::OK;

	sl::GuiApplication::OpenModal< sl::InlineModal >( cd, on_render, on_complete );
}

ClientLayer::ClientLayer( sl::net::ClientContextOptions const& opts )
	: sl::net::ClientLayer( opts )
{
}

void ClientLayer::OnOverlayRender()
{
	using sl::Widgets;

	Widgets::BeginWindow( "TestWindow" );

	Widgets::EditGeneric( "TestNestedStruct", sTestStruct );

	Widgets::EndWindow();
}

ChatClient::ChatClient( sl::Ref< sl::GuiApplicationSpecification > spec, sl::net::ClientContextOptions const& opts )
	: GuiApplication( spec )
{
	PushLayer< ClientLayer >( opts );
	AddLogTarget< sl::ConsoleLogTarget >( sl::LogLevel::Info );
}

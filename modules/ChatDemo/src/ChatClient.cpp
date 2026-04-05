#include "ChatClient.h"

#include "SL/Core/Common/Time.h"
#include "SL/Gfx/ImGui/Widgets.h"

static std::string GetTimestamp()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t( now );

	std::tm time = sl::GetLocalTime( &now_c );

	std::string timestamp( 20, '\0' );
	std::strftime( timestamp.data(), timestamp.size(), "%F %T", &time );
	return timestamp;
}

class ChatDisplayPanel : public sl::IPanel
{
public:
	void OnOverlayRender() override
	{
		using sl::Utils;
		using sl::Widgets;

		Utils::SetWindowFontScale( 1.2f );

		if ( mConnected )
		{

			for ( auto const& line : mRecentMessages )
			{
				auto message = std::string_view{ line.As< char >(), line.Size() };
				Widgets::Label( message );
			}
		}
		else
		{
			Widgets::LabelWrapped( "Connecting to server..." );
		}
	}

	bool OnEvent( sl::Event& e )
	{
		return e.Dispatch( BindDispatch( this, &ChatDisplayPanel::OnMessageReceived ) );
	}

	bool OnMessageReceived( sl::NetworkInEvent& e )
	{
		mRecentMessages.emplace_back( e.data );
		return true;
	}

	void SetConnected( bool connected )
	{
		mConnected = connected;
	}

	SL_LISTENING_EVENTS( sl::NetworkInEvent );

private:
	enum
	{
		max_recent_msgs = 100
	};
	std::deque< sl::net::Payload > mRecentMessages;
	bool mConnected = false;
};

class ChatEntryPanel : public sl::IPanel
{
public:
	void OnOverlayRender() override
	{
		using sl::Utils;
		using sl::Widgets;

		OpenUsernameEntryIfNeeded();

		Utils::SetWindowFontScale( 2.0f );

		ImGui::BeginDisabled( mUsername.empty() or not mConnected );

		int flags = ImGuiInputTextFlags_EnterReturnsTrue;
		Widgets::StringEdit( "Message", mCurrentText, flags, [ this ] { SendMessage(); } );

		ImGui::EndDisabled();
	}

	bool SendMessage()
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

	void OpenUsernameEntryIfNeeded()
	{
		if ( !mUsername.empty() or mUsernameModalOpen or not mConnected )
			return;

		using sl::Widgets;

		mUsernameModalOpen = true;

		auto on_render = [ this ] {
			Widgets::StringEdit( "Please enter your username...", mUsernameEntry );
		};

		auto on_complete = [ this ] { mUsername = mUsernameEntry; };
		auto on_close = [ this ] { mUsernameModalOpen = false; };

		auto on_button_render = [ this ]( sl::IModal* modal, bool& open, sl::Vec2f const& size ) {
			ImGui::BeginDisabled( mUsernameEntry.empty() );

			Widgets::Button( "OK", size, [ & ]() {
				modal->OnComplete();
				open = false;
			} );

			ImGui::EndDisabled();
		};

		auto cd = sl::ModalConstructionData{};
		cd.heading = "No username";
		cd.button_type = sl::ModalButtons::Custom;

		sl::GuiApplication::OpenModal< sl::InlineModal >( cd, on_render, on_complete, on_close, on_button_render );
	}

	void SetConnected( bool connected )
	{
		mConnected = connected;
		if ( !connected and mUsernameModalOpen )
		{
			sl::GuiApplication::CloseModal( "No username" );
		}
	}

private:
	bool mConnected = false;

	std::string mCurrentText{};

	std::string mUsername{};
	std::string mUsernameEntry{};
	bool mUsernameModalOpen = false;
};

class ChatLayer : public sl::net::ClientLayer
{
public:
	ChatLayer( sl::net::ClientContextOptions const& opts )
		: ClientLayer( opts )
	{}

	void OnAttach() override
	{
		auto display_cd = sl::PanelConstructionData{
			.key = "Messages",
		};
		mDisplayPanel = sl::GuiApplication::OpenPanel< ChatDisplayPanel >( display_cd );

		auto entry_cd = sl::PanelConstructionData{
			.key = "Input",
		};
		mEntryPanel = sl::GuiApplication::OpenPanel< ChatEntryPanel >( entry_cd );

		ClientLayer::OnAttach();
	}

	void OnOverlayRender() override
	{
		using sl::Widgets;

		Widgets::BeginWindow( "Chat Client", ImGuiWindowFlags_NoTitleBar );
		Widgets::EndWindow();
	}

	void OnConnect( sl::net::ConnectionPtr ) override
	{
		mDisplayPanel->SetConnected( true );
		mEntryPanel->SetConnected( true );
	}

	void OnDisconnect( sl::net::ConnectionPtr ) override
	{
		mDisplayPanel->SetConnected( false );
		mEntryPanel->SetConnected( false );
	}


private:
	sl::Ref< ChatDisplayPanel > mDisplayPanel;
	sl::Ref< ChatEntryPanel > mEntryPanel;
};


ChatClient::ChatClient( sl::Ref< sl::GuiApplicationSpecification > spec, sl::net::ClientContextOptions const& opts )
	: GuiApplication( spec )
{
	PushLayer< ChatLayer >( opts );
	AddLogTarget< sl::ConsoleLogTarget >( sl::LogLevel::Info );
}

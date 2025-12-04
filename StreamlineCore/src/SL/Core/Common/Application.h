#pragma once

#include "SL/Core/Events/IEventListener.h"
#include "SL/Core/Events/EventRuntime.h"
#include "SL/Core/IO/Window.h"
#include "SL/Core/ImGui/Controller.h"
#include "SL/Core/Logging/Logger.h"
#include "SL/Core/Types/Timestep.h"

#include <string>
#include <mutex>

int main( int argc, char* argv[] );

struct GLFWwindow;

namespace slc {
	class Application;
}

// To be defined in client
extern slc::Application* CreateApplication( int argc, char** argv );

namespace slc {

	class ApplicationLayer : public IEventListener
	{
	public:
		virtual ~ApplicationLayer() = default;

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnUpdate( Timestep ts ) = 0;
		virtual void OnRender() = 0;
		virtual void OnOverlayRender() = 0;
	};

	namespace detail {

		template < typename T >
		concept IsLayer = DerivedFromOnly< T, ApplicationLayer >;

		using LayerStack = std::vector< Ref< ApplicationLayer > >;


		template < typename T, typename... Args >
		concept AppSystem = requires( Args&&... args ) { T::Init(std::forward<Args>(args)...); T::Shutdown(); };

		using AppSystemCleanups = std::vector< Action<> >;


		struct ApplicationState
		{
			bool running = true;
			bool minimised = false;
			bool block_exit = false;
			float last_frame_time = 0.0f;

			std::mutex main_thread_queue_mutex;
			std::vector< Action<> > main_thread_queue;
		};
	} // namespace detail

	struct ApplicationSpecification
	{
		std::string name = "Streamline Application";
		Resolution resolution = { 1600, 900 };
		std::filesystem::path working_dir;
		bool fullscreen = false;
		bool headless = false;

		virtual ~ApplicationSpecification()
		{}
	};

	class Application : public IEventListener
	{
	public:
		SLC_LISTENING_EVENTS( WindowClose, WindowResize )

	public:
		Application( Box< ApplicationSpecification > spec );
		~Application();

		void OnEvent( Event& e ) override;

		Window& GetWindow()
		{
			return *mWindow;
		}

		template < detail::IsLayer T, typename... Args >
		void PushLayer( Args&&... args )
		{
			auto layer = mEventRuntime->CreateListener< T >( std::forward< Args >( args )... );
			mLayerStack.emplace_back( layer );
			layer->OnAttach();
		}

		template < typename T, typename... Args >
			requires detail::AppSystem< T, Args... >
		void RegisterSystem( Args&&... args )
		{
			T::Init( std::forward< Args >( args )... );
			mAppSystems.emplace_back( T::Shutdown );
		}

		template < typename T, typename... Args >
		void AddLogTarget( Args&&... args )
		{
			Logger::GetGlobalLogger().AddLogTarget< T >( std::forward< Args >( args )... );
		}

	private:
		bool OnWindowClose( WindowCloseEvent& e );
		bool OnWindowResize( WindowResizeEvent& e );

	public:
		static void Close();
		static Application& Get()
		{
			return *sInstance;
		}

		static const ApplicationSpecification& GetSpec()
		{
			return *sInstance->mSpecification;
		}
		template < typename T >
			requires std::derived_from< T, ApplicationSpecification >
		static const T& GetSpec()
		{
			return static_cast< const T& >( *sInstance->mSpecification );
		}

		template < typename T >
			requires std::derived_from< T, ApplicationSpecification >
		static void SetSpec( const T& spec )
		{
			sInstance->mSpecification = MakeBox< T >( spec );
		}

		template < IsAction Func >
		static void SubmitActionToMainThread( Func&& function )
		{
			std::scoped_lock< std::mutex > lock( sInstance->mState.main_thread_queue_mutex );

			sInstance->mState.main_thread_queue.emplace_back( std::move( function ) );
		}

		static void ExecuteQueuedJobs();

		static void BlockEsc( bool block = true );
		static void BlockEvents( bool block );

		static GLFWwindow* GetNativeWindow()
		{
			return sInstance->mWindow->GetNativeWindow();
		}

		static float GetWindowWidth()
		{
			return static_cast< float >( sInstance->mWindow->GetWidth() );
		}
		static float GetWindowHeight()
		{
			return static_cast< float >( sInstance->mWindow->GetHeight() );
		}

		template < IsEventListener T, typename... Args >
		static Ref< T > CreateEventListener( Args&&... args )
		{
			if ( not sInstance )
				return nullptr;

			return sInstance->mEventRuntime->CreateListener< T >( std::forward< Args >( args )... );
		}

		template < IsEvent TEvent, typename... TArgs >
		static void PostEvent( TArgs&&... args )
		{
			if ( not sInstance )
				return;

			sInstance->mEventRuntime->Post< TEvent >( std::forward< TArgs >( args )... );
		}

		template < IsModal T, typename... Args >
		static void OpenModal( ModalConstructionData const& init_data, Args&&... args )
		{
			if ( !sInstance )
				return;

			if ( !sInstance->mImGuiController )
				return;

			sInstance->mImGuiController->OpenModal< T >( init_data, std::forward< Args >( args )... );
		}

		template < IsPanel T, typename... Args >
		static void OpenPanel( PanelConstructionData const& init_data, Args&&... args )
		{
			if ( !sInstance )
				return;

			if ( !sInstance->mImGuiController )
				return;

			sInstance->mImGuiController->OpenPanel< T >( init_data, std::forward< Args >( args )... );
		}

	private:
		static void Run( int argc, char** argv );

	protected:
		Box< ApplicationSpecification > mSpecification;

	private:
		detail::ApplicationState mState;
		Box< Window > mWindow;
		Ref< ImGuiController > mImGuiController;
		detail::LayerStack mLayerStack;
		detail::AppSystemCleanups mAppSystems;

		Box< EventRuntime > mEventRuntime;

	private:
		inline static Application* sInstance = nullptr;
		friend int ::main( int argc, char** argv );
	};
} // namespace slc
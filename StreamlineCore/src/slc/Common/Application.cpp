#include "Application.h"

#include "slc/Graphics/Renderer.h"
#include "slc/ImGui/Widgets.h"

namespace slc {

	Application::Application( Box< ApplicationSpecification > spec )
		: mSpecification( std::move( spec ) )
	{
		if ( sInstance )
		{
			ASSERT( false, "Application already exists" );
			return;
		}
		sInstance = this;

		// Manually call register again as there was no runtime instance to register with in IEventListener constructor as runtime was initialised after.
		EventRuntime::RegisterListener( this );

		if ( !mSpecification->workingDir.empty() )
			std::filesystem::current_path( mSpecification->workingDir );

		if ( mSpecification->headless )
			return;

		mWindow = Window::Create( WindowProperties( mSpecification->name, mSpecification->resolution, mSpecification->fullscreen ) );

		mImGuiController = ImGuiController::Create( mWindow->GetNativeWindow() );

		RegisterSystem< Renderer >();
	}

	Application::~Application()
	{
		for ( ApplicationLayer* layer : mLayerStack )
		{
			layer->OnDetach();
			delete layer;
		}

		mImGuiController.reset();
		mWindow.reset();

		for ( const auto& shutdownTask : mAppSystems | std::views::reverse )
			shutdownTask();
	}

	void Application::OnEvent( Event& e )
	{
		e.Dispatch< WindowCloseEvent >( SLC_BIND_EVENT_FUNC( OnWindowClose ) );
		e.Dispatch< WindowResizeEvent >( SLC_BIND_EVENT_FUNC( OnWindowResize ) );
	}

	bool Application::OnWindowClose( WindowCloseEvent& e )
	{
		Application::Close();
		return true;
	}

	bool Application::OnWindowResize( WindowResizeEvent& e )
	{
		if ( e.width == 0 || e.height == 0 )
		{
			mState.minimised = true;
			return false;
		}

		mState.minimised = false;
		Renderer::SetViewport( e.width, e.height );
		return false;
	}

	void Application::ExecuteQueuedJobs()
	{
		std::scoped_lock< std::mutex > lock( sInstance->mState.main_thread_queue_mutex );

		for ( auto& func : sInstance->mState.main_thread_queue )
			func();

		sInstance->mState.main_thread_queue.clear();
	}

	void Application::Run( int argc, char** argv )
	{
		Application* app = CreateApplication( argc, argv );
		if ( sInstance != app )
		{
			delete app;
			ASSERT( false, "There was already an app instance, could not create a new one" );
			return;
		}

		while ( sInstance->mState.running )
		{
			float time = Timestep::Now();
			Timestep timestep = time - sInstance->mState.last_frame_time;
			sInstance->mState.last_frame_time = time;

			// Process any queued tasks that could not be performed within main loop.
			sInstance->ExecuteQueuedJobs();

			// Process any events in the event queue
			sInstance->mState.event_runtime.Dispatch();

			// Run update and render method for each frame
			if ( !sInstance->mState.minimised )
			{
				for ( auto* layer : sInstance->mLayerStack )
					layer->OnUpdate( timestep );

				if ( not sInstance->mSpecification->headless )
				{
					for ( auto* layer : sInstance->mLayerStack )
						layer->OnRender();
				}
			}

			if ( not sInstance->mSpecification->headless )
			{
				// Begin ImGui rendering
				sInstance->mImGuiController->StartFrame();

				// Render each ImGui controls in each layer
				for ( ApplicationLayer* layer : sInstance->mLayerStack )
					layer->OnOverlayRender();

				// End ImGui rendering
				sInstance->mImGuiController->EndFrame();

				// Poll GLFW events to populate queue and swap buffers
				sInstance->mWindow->OnUpdate();
			}
		}

		delete sInstance;
	}

	void Application::Close()
	{
		if ( !sInstance )
			return;

		if ( !sInstance->mState.block_exit )
			sInstance->mState.running = false;
	}

	void Application::BlockEsc( bool block )
	{
		sInstance->mState.block_exit = block;
	}

	void Application::BlockEvents( bool block )
	{
		if ( sInstance->mSpecification->headless )
			return;

		sInstance->mImGuiController->BlockEvents( block );
	}
} // namespace slc
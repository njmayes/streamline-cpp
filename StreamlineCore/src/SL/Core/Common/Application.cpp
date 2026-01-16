#include "Application.h"

namespace sl {

	Application::Application( Ref< ApplicationSpecification > spec )
		: mSpecification( spec )
	{
		if ( sInstance )
			throw std::runtime_error( "Application instance already exists!" );

		sInstance = this;

		mEventRuntime = MakeBox< EventRuntime >();

		// Manually call RegisterListener for Application it is not created via EventRuntime::CreateListener
		mEventRuntime->RegisterListener( this );

		if ( !mSpecification->working_dir.empty() )
			std::filesystem::current_path( mSpecification->working_dir );
	}

	Application::~Application()
	{
		for ( auto& layer : mLayerStack )
		{
			layer->OnDetach();
			layer.Reset();
		}

		mEventRuntime.reset();

		for ( const auto& shutdownTask : mAppSystems | std::views::reverse )
			shutdownTask();
	}

	void Application::ExecuteQueuedJobs()
	{
		std::scoped_lock< std::mutex > lock( sInstance->mState.main_thread_queue_mutex );

		for ( auto& func : sInstance->mState.main_thread_queue )
			func();

		sInstance->mState.main_thread_queue.clear();
	}

	void Application::Run( ApplicationFactory make_app, CommandLineArgs args )
	{
		Application* app = make_app( std::move( args ) );
		if ( !app )
			throw std::runtime_error( "No application instance was created!" );

		while ( sInstance->mState.running )
		{
			float time = Timestep::Now();
			Timestep timestep = time - sInstance->mState.last_frame_time;
			sInstance->mState.last_frame_time = time;

			// Process any queued tasks that could not be performed within main loop.
			sInstance->ExecuteQueuedJobs();

			// Process any events in the event queue
			sInstance->mEventRuntime->Dispatch();

			// Run update and render method for each frame
			if ( !sInstance->mState.minimised )
			{
				for ( auto& layer : sInstance->mLayerStack )
					layer->OnUpdate( timestep );

				sInstance->OnRender();
			}

			sInstance->OnUpdate( timestep );
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
		if ( !sInstance )
			return;

		sInstance->mState.block_exit = block;
	}

	void Application::BlockEvents( bool block )
	{
		if ( !sInstance )
			return;

		sInstance->mState.block_events = block;
	}

	bool Application::AreEventsBlocked()
	{
		if ( !sInstance )
			return false;

		return sInstance->mState.block_events;
	}
} // namespace sl
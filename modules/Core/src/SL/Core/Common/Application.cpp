#include "Application.h"

namespace sl {

	Application::Application( Ref< ApplicationSpecification > spec )
		: mSpecification( spec )
	{
		sInstance = this;

		mEventRuntime = MakeBox< ApplicationEventRuntime >();

		// Manually call RegisterDevice for Application it is not created via EventRuntime::CreateEventDevice and thus does not automatically register itself as a device
		mEventRuntime->RegisterDevice( this );

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

		sInstance = nullptr;
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
		SL_VERIFY( !sInstance, "Application instance already exists!" );

		Box< Application > app = make_app( std::move( args ) );
		SL_VERIFY( sInstance, "No application instance was created!" );

		while ( sInstance->mState.running )
		{
			double time = Timestep::Now();
			Timestep timestep = time - sInstance->mState.last_frame_time;
			sInstance->mState.last_frame_time = time;

			// Process any queued tasks that could not be performed within main loop.
			sInstance->ExecuteQueuedJobs();

			// Poll for and process any events in the event queue
			sInstance->mEventRuntime->OnUpdate();

			// Run update for each frame
			for ( auto& layer : sInstance->mLayerStack )
				layer->OnUpdate( timestep );

			// Run derived application update logic
			sInstance->OnUpdate( timestep );
		}
	}

	void Application::Close()
	{
		if ( not sInstance )
			throw std::runtime_error( "No application instance" );

		if ( sInstance->mState.block_exit )
			return;

		sInstance->mState.running = false;
	}

	void Application::BlockEsc( bool block )
	{
		if ( not sInstance )
			throw std::runtime_error( "No application instance" );

		sInstance->mState.block_exit = block;
	}

	void Application::BlockEvents( bool block )
	{
		if ( not sInstance )
			throw std::runtime_error( "No application instance" );

		sInstance->mState.block_events = block;
	}

	bool Application::AreEventsBlocked()
	{
		if ( not sInstance )
			throw std::runtime_error( "No application instance" );

		return sInstance->mState.block_events;
	}
} // namespace sl
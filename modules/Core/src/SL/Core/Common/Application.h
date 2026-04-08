#pragma once

#include "CommandLine.h"

#include "SL/Core/Events/EventRuntime.h"
#include "SL/Core/Events/EventList.h"

#include "SL/Core/Logging/Logger.h"
#include "SL/Core/Types/Timestep.h"

#include <string>
#include <mutex>

namespace sl {

	class Application;

	using ApplicationFactory = std::function< Box< Application >( CommandLineArgs ) >;

	using ApplicationEventRuntime = EventRuntimeMTOrdered< CoreEventList >;

	using ApplicationEventEmitter = typename ApplicationEventRuntime::Emitter;
	using ApplicationEventListener = typename ApplicationEventRuntime::Listener;

	class ApplicationLayer : public ApplicationEventListener, public ApplicationEventEmitter
	{
	public:
		virtual ~ApplicationLayer() = default;

		virtual void OnAttach() {};
		virtual void OnDetach() {};
		virtual void OnUpdate( Timestep ts ) {};
		virtual void OnRender() {};
		virtual void OnOverlayRender() {};
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
			bool block_events = false;
			double last_frame_time = 0.0;

			std::mutex main_thread_queue_mutex;
			std::vector< Action<> > main_thread_queue;
		};
	} // namespace detail

	struct ApplicationSpecification : public RefCounted
	{
		std::string name = "Streamline Application";
		std::filesystem::path working_dir;

		virtual ~ApplicationSpecification()
		{}
	};

	class Application : public ApplicationEventListener
	{
	public:
		static void Run( ApplicationFactory factory, CommandLineArgs args );

	public:
		Application( Ref< ApplicationSpecification > spec );
		virtual ~Application();

		virtual void OnUpdate( Timestep ts )
		{}

		template < detail::IsLayer T, typename... Args >
		Ref< T > PushLayer( Args&&... args )
		{
			auto layer = mEventRuntime->CreateEventDevice< T >( std::forward< Args >( args )... );
			mLayerStack.emplace_back( layer );
			layer->OnAttach();

			return layer;
		}

		template < detail::IsLayer T >
		void PopLayer( Ref< T > layer )
		{
			auto it = std::find_if( mLayerStack.begin(), mLayerStack.end(), [ &layer ]( const auto& l ) { return l == layer; } );
			if ( it == mLayerStack.end() )
				throw std::runtime_error( "Layer not found in stack" );
			layer->OnDetach();
			mLayerStack.erase( it );
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

	public:
		static bool Exists()
		{
			return sInstance;
		}

		static void Close();

		static Application* Get()
		{
			return sInstance;
		}

		template < typename T >
			requires std::derived_from< T, Application >
		static T* Get()
		{
			return dynamic_cast< T* >( Get() );
		}

		template < IsAction Func >
		static void SubmitActionToMainThread( Func&& function )
		{
			if ( not sInstance )
				return;

			std::scoped_lock< std::mutex > lock( sInstance->mState.main_thread_queue_mutex );

			sInstance->mState.main_thread_queue.emplace_back( std::move( function ) );
		}

		static void BlockEsc( bool block = true );
		static void BlockEvents( bool block );

		static bool AreEventsBlocked();

		template < typename T, typename... Args >
		static Ref< T > CreateEventDevice( Args&&... args )
		{
			if ( not sInstance )
				throw std::runtime_error( "No application instance" );

			return sInstance->mEventRuntime->CreateEventDevice< T >( std::forward< Args >( args )... );
		}

	private:
		static void ExecuteQueuedJobs();

	protected:
		detail::LayerStack const& GetLayerStack()
		{
			return mLayerStack;
		}

	protected:
		detail::ApplicationState mState;

	private:
		Ref< ApplicationSpecification > mSpecification;

		detail::LayerStack mLayerStack;
		detail::AppSystemCleanups mAppSystems;

		Box< ApplicationEventRuntime > mEventRuntime;

	private:
		inline static Application* sInstance = nullptr;
	};
} // namespace sl
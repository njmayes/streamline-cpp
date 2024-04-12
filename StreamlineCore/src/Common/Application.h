#pragma once

#include "Containers/Vector.h"
#include "Events/IEventListener.h"
#include "IO/Window.h"
#include "ImGui/Controller.h"
#include "Types/Timestep.h"

#include <string>
#include <mutex>

int main(int argc, char* argv[]);

struct GLFWwindow;

namespace slc {

	class Application;

	//To be defined in client
	extern slc::Application* CreateApplication(int argc, char** argv);

	template<typename T, typename... Args>
	concept AppSystem = requires (Args&&... args) { T::Init(std::forward<Args>(args)...); T::Shutdown(); };

	class ApplicationLayer : public IEventListener
	{
	public:
		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnUpdate(Timestep ts) = 0;
		virtual void OnRender() = 0;
	};

	template<typename T>
	concept IsLayer = std::is_base_of_v<ApplicationLayer, T>;

	using LayerStack = Vector<ApplicationLayer*>;
	using ApplicationSystems = Vector<Action<>>;

	struct ApplicationSpecification
	{
		std::string name = "Streamline Application";
		Resolution resolution = { 1600, 900 };
		fs::path workingDir;
		bool fullscreen = false;

		virtual ~ApplicationSpecification() {}
	};

	struct ApplicationState
	{
		bool running = true;
		bool minimised = false;
		bool blockExit = false;
		float lastFrameTime = 0.0f;

		Vector<Action<>> mainThreadQueue;
		std::mutex mainThreadQueueMutex;
	};

	class Application : public IEventListener
	{
	public:
		LISTENING_EVENTS(EventType::WindowClose, EventType::WindowResize)

	public:
		Application(ApplicationSpecification* spec);
		~Application();

		void OnEvent(Event& e) override;

		Window& GetWindow() { return *mWindow; }

	protected:
		template<IsLayer T, typename... Args>
		void PushLayer(Args&&... args) 
		{ 
			T* layer = new T(std::forward<Args>(args)...);
			PushLayer(layer);
		}

		void PushLayer(IsLayer auto* layer)
		{
			mLayerStack.emplace_back(layer);
			layer->OnAttach();
		}

		template<typename T, typename... Args> requires AppSystem<T, Args...>
		void RegisterSystem(Args&&... args)
		{
			T::Init(std::forward<Args>(args)...);
			mAppSystems.emplace_back(T::Shutdown);
		}

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	public:
		static void Close();
		static Application& Get() { return *sInstance; }

		static const ApplicationSpecification& GetSpec() { return *sInstance->mSpecification; }
		template<typename T> requires std::derived_from<T, ApplicationSpecification>
		static const T& GetSpec() { return *dynamic_cast<const T*>(sInstance->mSpecification); }

		template<typename T> requires std::derived_from<T, ApplicationSpecification>
		static void SetSpec(const T& spec)
		{
			*sInstance->mSpecification = spec;
		}

		template<IsAction Func>
		static void SubmitActionToMainThread(Func&& function)
		{
			std::scoped_lock<std::mutex> lock(sInstance->mState.mainThreadQueueMutex);

			sInstance->mState.mainThreadQueue.emplace_back(std::move(function));
		}

		static void ExecuteMainThread();

		static void BlockEsc(bool block = true);
		static void BlockEvents(bool block);

		static GLFWwindow* GetNativeWindow() { return sInstance->mWindow->GetNativeWindow(); }

		static float GetWindowWidth() { return static_cast<float>(sInstance->mWindow->GetWidth()); }
		static float GetWindowHeight() { return static_cast<float>(sInstance->mWindow->GetHeight()); }

	private:
		static void Run(int argc, char** argv);

	protected:
		ApplicationSpecification* 	mSpecification;

	private:
		ApplicationState 			mState;
		Impl<Window> 				mWindow;
		Impl<ImGuiController> 		mImGuiController;
		LayerStack					mLayerStack;
		ApplicationSystems			mAppSystems;

	private:
		inline static Application* sInstance = nullptr;
		friend int ::main(int argc, char** argv);
	};
}
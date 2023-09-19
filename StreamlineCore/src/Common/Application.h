#pragma once

#include <string>

#include "Events/IEventListener.h"
#include "IO/Window.h"
#include "ImGui/ImGuiController.h"
#include "Types/ILayer.h"

int main(int argc, char* argv[]);

struct GLFWwindow;

namespace slc {

	class Application;

	//To be defined in client
	extern Impl<Application> CreateApplication(int argc, char** argv);

	struct ApplicationSpecification
	{
		std::string name = "Streamline Application";
		Resolution resolution = { 1600, 900 };
		fs::path workingDir;
		bool fullscreen = false;
	};

	struct ApplicationState
	{
		bool running = true;
		bool minimised = false;
		bool blockExit = false;

		std::vector<Action<>> mainThreadQueue;
		std::mutex mainThreadQueueMutex;
	};

	class Application : public IEventListener
	{
	public:
		LISTENING_EVENTS(WindowClose, WindowResize)

	public:
		Application(const ApplicationSpecification& spec);
		~Application();

	public:
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

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	public:		
		static void Close();

		static const ApplicationSpecification& GetSpec() { return sInstance->mSpecification; }

		static void SubmitActionToMainThread(Action<>&& function);
		static void ExecuteMainThread();

		static void BlockEsc(bool block = true);
		static void BlockEvents(bool block);

		static GLFWwindow* GetNativeWindow() { return sInstance->mWindow->getNativeWindow(); }

		static float GetWindowWidth() { return static_cast<float>(sInstance->mWindow->getWidth()); }
		static float GetWindowHeight() { return static_cast<float>(sInstance->mWindow->getHeight()); }

	private:
		static void Run(int argc, char** argv);
		static Application& Get() { return *sInstance; }

	private:
		ApplicationSpecification 	mSpecification;
		ApplicationState 			mState;
		Impl<Window> 				mWindow;
		Impl<ImGuiController> 		mImGuiController;
		std::vector<ILayer*>		mLayerStack;

	private:
		inline static Impl<Application> sInstance = nullptr;
		friend int ::main(int argc, char** argv);
	};
}
#pragma once

#include <string>

#include "IO/Window.h"
#include "Events/IEventListener.h"

int main(int argc, char* argv[]);

namespace slc {

	class Application;

	//To be defined in client
	extern Impl<Application> CreateApplication(int argc, char** argv);

	struct ApplicationSpecification
	{
		std::string name = "Labyrinth Application";
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

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	public:
		static void Run(int argc, char** argv);
		static Application& Get() { return *sInstance; }
		static void Close();

		static const ApplicationSpecification& GetSpec() { return sInstance->mSpecification; }

		static void SubmitActionToMainThread(Action<>&& function);
		static void ExecuteMainThread();

		static void BlockEsc(bool block = true);
		static void BlockEvents(bool block);

	private:
		ApplicationSpecification mSpecification;
		ApplicationState mState;
		Impl<Window> mWindow;

	private:
		inline static Impl<Application> sInstance = nullptr;
		friend int ::main(int argc, char** argv);
	};
}
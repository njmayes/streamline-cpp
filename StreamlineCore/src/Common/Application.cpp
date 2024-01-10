#include "pch.h"
#include "Application.h"

#include "Graphics/Renderer.h"
#include "ImGui/Widgets.h"

namespace slc {

	Application::Application(const ApplicationSpecification& spec)
		: IEventListener(ListenerType::App), mSpecification(spec)
	{
		if (!mSpecification.workingDir.empty())
			std::filesystem::current_path(mSpecification.workingDir);

		mWindow = Window::Create(WindowProperties(mSpecification.name, mSpecification.resolution, mSpecification.fullscreen));

		Renderer::Init();

		mImGuiController = ImGuiController::Create(mWindow->GetNativeWindow());
	}

	Application::~Application()
	{
		for (ApplicationLayer* layer : mLayerStack)
		{
			layer->OnDetach();
			delete layer;
		}

		mWindow.reset();
	}

	void Application::OnEvent(Event& e)
	{
		e.Dispatch<WindowCloseEvent>(SLC_BIND_EVENT_FUNC(OnWindowClose));
		e.Dispatch<WindowResizeEvent>(SLC_BIND_EVENT_FUNC(OnWindowResize));
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		Application::Close();
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.width == 0 || e.height == 0)
		{
			mState.minimised = true;
			return false;
		}

		mState.minimised = false;
		Renderer::SetViewport(e.width, e.height);
		return false;
	}

	void Application::ExecuteMainThread()
	{
		std::scoped_lock<std::mutex> lock(sInstance->mState.mainThreadQueueMutex);

		for (auto& func : sInstance->mState.mainThreadQueue)
			func();

		sInstance->mState.mainThreadQueue.clear();
	}

	void Application::Run(int argc, char** argv)
	{
		ASSERT(!sInstance, "Application already exists");
		sInstance = CreateApplication(argc, argv);

		while (sInstance->mState.running)
		{
			// Process any queued tasks that could not be performed within main loop.
			sInstance->ExecuteMainThread();

			// Process any events in the event queue
			EventManager::Dispatch();

			// Begin ImGui rendering
			sInstance->mImGuiController->StartFrame();

			// Render each ImGui controls in each layer
			for (ApplicationLayer* layer : sInstance->mLayerStack)
				layer->OnRender();

			// End ImGui rendering
			sInstance->mImGuiController->EndFrame();

			// Poll GLFW events to populate queue and swap buffers
			sInstance->mWindow->OnUpdate();
		}

		sInstance.reset();
	}

	void Application::Close()
	{
		if (!sInstance->mState.blockExit) 
			sInstance->mState.running = false;
	}

	void Application::BlockEsc(bool block)
	{
		sInstance->mState.blockExit = block;
	}

	void Application::BlockEvents(bool block)
	{
		sInstance->mImGuiController->BlockEvents(block);
	}
}
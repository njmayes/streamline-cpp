#include "pch.h"
#include "Application.h"

#include "Graphics/Renderer.h"

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
		for (ILayer* layer : mLayerStack)
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
			sInstance->ExecuteMainThread();

			EventManager::Dispatch();

			sInstance->mImGuiController->StartFrame();

			for (ILayer* layer : sInstance->mLayerStack)
				layer->OnRender();

			sInstance->mImGuiController->EndFrame();

			sInstance->mWindow->OnUpdate();
		}

		sInstance.reset();
	}

	void Application::Close()
	{
		if (!sInstance->mState.blockExit) 
			sInstance->mState.running = false;
	}

	void Application::SubmitActionToMainThread(Action<>&& function)
	{
		std::scoped_lock<std::mutex> lock(sInstance->mState.mainThreadQueueMutex);

		sInstance->mState.mainThreadQueue.emplace_back(std::move(function));
	}

	void Application::BlockEsc(bool block)
	{
		sInstance->mState.blockExit = block;
	}

	void Application::BlockEvents(bool block)
	{
        // TODO: Add imgui block events
	}
}
#include "streamline.h"
#include "Common/EntryPoint.h"

namespace slc {

    class TestLayer : public ILayer
    {
    public:
		virtual void OnAttach() override {}
		virtual void OnDetach() override {}
		virtual void OnRender() override {}
        void OnEvent(Event& e) override {}

		LISTENING_EVENTS(KeyPressed, MouseButtonPressed)
    };

	class TestApp : public Application
	{
	public:
		TestApp(const ApplicationSpecification& spec)
			: Application(spec)
		{
			PushLayer<TestLayer>();
		}

		~TestApp()
		{
		}
	};

	Impl<Application> CreateApplication(int argc, char** argv)
	{
		ApplicationSpecification spec;
		spec.name = "TestApp";

		return MakeImpl<TestApp>(spec);
	}

}
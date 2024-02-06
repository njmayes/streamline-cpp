#include "streamline.h"
#include "Common/EntryPoint.h"

#include "Common/Reflection.h"

namespace slc {

	enum class Error
	{
		InvalidChar,
		InvalidRandom
	};
	enum class Failure
	{
		RandomFail
	};
	using FooResult = Result<int, Error>;
	using BarResult = Result<float, Error>;
	using BazResult = Result<int, Failure>;

    class TestLayer : public ApplicationLayer
    {
	private:
		FooResult GetInput()
		{
			int input;
			if (!(std::cin >> input))
				return Err<FooResult>(Error::InvalidChar);

			return Ok<FooResult>(input);
		}

		FooResult GetRandom()
		{
			srand(time(NULL));
			int val = rand() % 2;

			if (val == 0)
				return Ok<FooResult>(1000);

			return Err<FooResult>(Error::InvalidRandom);
		}

		FooResult GetRandomTwo(int value)
		{
			if (value % 2 == 0)
				return Ok<FooResult>(500);

			return Err<FooResult>(Error::InvalidRandom);
		}

		BarResult CheckRandom(int val)
		{
			if (val % 2 == 0)
				return Ok<BarResult>(3.14159);

			return Err<BarResult>(Error::InvalidRandom);
		}

    public:
		virtual void OnAttach() override {}
		virtual void OnDetach() override {}
		virtual void OnUpdate(Timestep ts) 
		{
			auto a = Do<float>(
				GetRandom(),
				NEXT(CheckRandom)
			);

			FooResult b = a.map<int>([](float val) { return (int)val; })
				.map_err<Failure>([](Error error) { return Failure::RandomFail; })
				.map_err<Error>([](Failure f) { return Error::InvalidRandom; });

			MATCH_START(b)
				MATCH_OK(std::cout << "User entered value of " << value << "\n")
				MATCH(Error::InvalidChar, std::cout << "Invalid character entered\n")
				MATCH(Error::InvalidRandom, std::cout << "RNG not satisfied\n")
			MATCH_END;

			auto bVal = b.unwrap_or_default();

			auto c = Do<float>(
				GetRandom(),
				NEXT(CheckRandom)
			).map_or<std::string>("Error", [](float val) { return std::to_string(val); });


			auto d = GetRandom() |
				NEXT(GetRandomTwo) |
				CHAIN(GetRandom);

			MATCH_START(d)
				MATCH_OK(std::cout << "User entered value of " << value << "\n")
				MATCH(Error::InvalidChar, std::cout << "Invalid character entered\n")
				MATCH(Error::InvalidRandom, std::cout << "RNG not satisfied\n")
			MATCH_END;

			auto dVal = d.unwrap_or_else([]() { return 0; });
		}
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
}

slc::Impl<slc::Application> CreateApplication(int argc, char** argv)
{
	slc::ApplicationSpecification spec;
	spec.name = "TestApp";

	return slc::MakeImpl<slc::TestApp>(spec);
}
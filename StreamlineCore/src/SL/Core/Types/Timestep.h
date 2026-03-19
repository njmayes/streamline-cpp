#pragma once

namespace sl {

	class Timestep
	{
	public:
		Timestep(double time = 0.0)
			: mTime(time)
		{
		}

		operator double() const
		{
			return mTime;
		}

		double GetSeconds() const
		{
			return mTime;
		}
		double GetMilliseconds() const
		{
			return mTime * 1000.0;
		}

	private:
		double mTime;

	public:
		static double Now();
	};

}
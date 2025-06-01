#pragma once

namespace slc {

	class Timestep
	{
	public:
		Timestep(float time = 0.0f)
			: mTime(time)
		{
		}

		operator float() const { return mTime; }

		float GetSeconds() const { return mTime; }
		float GetMilliseconds() const { return mTime * 1000.f; }

	private:
		float mTime;

	public:
		static float Now();
	};

}
#pragma once

#include <atomic>

namespace sl::thread {

	enum class UnlockPolicy
	{
		NotifyOne,
		NotifyAll
	};

	class ScopedGate
	{
	public:
		ScopedGate( std::atomic_flag& flag, UnlockPolicy policy )
			: mFlag( flag ), mPolicy( policy )
		{
			mFlag.wait( true );
			mFlag.test_and_set();
		}

		~ScopedGate()
		{
			mFlag.clear();

			switch ( mPolicy )
			{
				case UnlockPolicy::NotifyOne:
					mFlag.notify_one();
					break;
				case UnlockPolicy::NotifyAll:
					mFlag.notify_all();
					break;
			}
		}

	private:
		std::atomic_flag& mFlag;
		UnlockPolicy mPolicy;
	};

	class Flag
	{
	public:
		Flag() = default;
		Flag( UnlockPolicy policy )
			: mFlag{}, mPolicy{ policy }
		{
		}

		[[nodiscard]] ScopedGate Lock()
		{
			return ScopedGate( mFlag, mPolicy );
		}

		void WaitUntil( bool expected ) const
		{
			mFlag.wait( not expected );
		}

	private:
		std::atomic_flag mFlag{};
		UnlockPolicy mPolicy = UnlockPolicy::NotifyOne;
	};
} // namespace sl::thread
#pragma once

#include "Event.h"

namespace sl {

	template < typename TRuntime >
	class BasicEventDevice : public RefCounted
	{
	public:
		using Runtime = TRuntime;
		using EventList = typename Runtime::EventList;
		using Event = typename Runtime::EventType;

		static_assert( IsTypeList< EventList >, "Runtime::EventList must satisfy IsTypeList" );

		virtual ~BasicEventDevice()
		{
			if ( mRuntime )
				mRuntime->DeregisterDevice( this );
		}

		virtual bool OnEvent( Event& )
		{
			return false;
		};

		virtual bool Accept( Event& ) const
		{
			return false;
		}

		virtual void PollEvents()
		{

		}

	protected:
		template < typename, EventRuntimeMode, EventOrdering >
		friend class BasicEventRuntime;

		Runtime* mRuntime = nullptr;
	};

} // namespace sl
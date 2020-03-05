#include "vstgui/lib/platform/linux/x11frame.h"
#include "pluginterfaces/gui/iplugview.h"
#include "base/source/fstring.h"

namespace VSTGUI {

// Map Steinberg Vst Interface to VSTGUI Interface
class RunLoop : public X11::IRunLoop, public AtomicReferenceCounted
{
public:
	struct EventHandler : Steinberg::Linux::IEventHandler, public Steinberg::FObject
	{
		X11::IEventHandler* handler {nullptr};

		void PLUGIN_API onFDIsSet (Steinberg::Linux::FileDescriptor) override
		{
			if (handler)
				handler->onEvent ();
		}
		DELEGATE_REFCOUNT (Steinberg::FObject)
		DEFINE_INTERFACES
			DEF_INTERFACE (Steinberg::Linux::IEventHandler)
		END_DEFINE_INTERFACES (Steinberg::FObject)
	};
	struct TimerHandler : Steinberg::Linux::ITimerHandler, public Steinberg::FObject
	{
		X11::ITimerHandler* handler {nullptr};

		void PLUGIN_API onTimer () final
		{
			if (handler)
				handler->onTimer ();
		}
		DELEGATE_REFCOUNT (Steinberg::FObject)
		DEFINE_INTERFACES
			DEF_INTERFACE (Steinberg::Linux::ITimerHandler)
		END_DEFINE_INTERFACES (Steinberg::FObject)
	};

	bool registerEventHandler (int fd, X11::IEventHandler* handler) final
	{
		if(!runLoop)
			return false;

		auto smtgHandler = Steinberg::owned (new EventHandler ());
		smtgHandler->handler = handler;
		if (runLoop->registerEventHandler (smtgHandler, fd) == Steinberg::kResultTrue)
		{
			eventHandlers.push_back (smtgHandler);
			return true;
		}
		return false;
	}
	bool unregisterEventHandler (X11::IEventHandler* handler) final
	{
		if(!runLoop)
			return false;

		for (auto it = eventHandlers.begin (), end = eventHandlers.end (); it != end; ++it)
		{
			if ((*it)->handler == handler)
			{
				runLoop->unregisterEventHandler ((*it));
				eventHandlers.erase (it);
				return true;
			}
		}
		return false;
	}
	bool registerTimer (uint64_t interval, X11::ITimerHandler* handler) final
	{
		if(!runLoop)
			return false;

		auto smtgHandler = Steinberg::owned (new TimerHandler ());
		smtgHandler->handler = handler;
		if (runLoop->registerTimer (smtgHandler, interval) == Steinberg::kResultTrue)
		{
			timerHandlers.push_back (smtgHandler);
			return true;
		}
		return false;
	}
	bool unregisterTimer (X11::ITimerHandler* handler) final
	{
		if(!runLoop)
			return false;

		for (auto it = timerHandlers.begin (), end = timerHandlers.end (); it != end; ++it)
		{
			if ((*it)->handler == handler)
			{
				runLoop->unregisterTimer ((*it));
				timerHandlers.erase (it);
				return true;
			}
		}
		return false;
	}

	RunLoop (Steinberg::FUnknown* runLoop) : runLoop (runLoop) {}
private:
	using EventHandlers = std::vector<Steinberg::IPtr<EventHandler>>;
	using TimerHandlers = std::vector<Steinberg::IPtr<TimerHandler>>;
	EventHandlers eventHandlers;
	TimerHandlers timerHandlers;
	Steinberg::FUnknownPtr<Steinberg::Linux::IRunLoop> runLoop;
};

} // namespace

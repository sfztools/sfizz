// This file is part of VSTGUI. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this
// distribution and at http://github.com/steinbergmedia/vstgui/LICENSE

#pragma once
#include "base/source/updatehandler.h"
#include <vstgui/lib/cvstguitimer.h>

/// @cond ignore
namespace Steinberg {

class IdleUpdateHandler
{
public:
	static void start ()
	{
		auto& instance = get ();
		if (++instance.users == 1)
		{
			instance.timer = VSTGUI::makeOwned<VSTGUI::CVSTGUITimer> (
			    [] (VSTGUI::CVSTGUITimer*) { return UpdateHandler::instance ()->triggerDeferedUpdates (); },
			    1000 / 30);
		}
	}

	static void stop ()
	{
		auto& instance = get ();
		if (--instance.users == 0)
		{
			instance.timer = nullptr;
		}
	}

protected:
	static IdleUpdateHandler& get ()
	{
		static IdleUpdateHandler gInstance;
		return gInstance;
	}

	VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> timer;
	std::atomic<uint32_t> users {0};
};

} // namespace Steinberg
/// @endcond ignore

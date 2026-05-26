// C-ABI exports for CounterStrikeSharp P/Invoke. quiet=true on all entries.

#include "dispatch.h"
#include "InputInjector.h"

#include <cstdint>

extern "C" __declspec(dllexport)
int BotLocker_Lock(int slot, int kind, int arg)
{
    return BotLocker::Dispatch::Lock(slot,
        static_cast<BotLocker::LockKind>(kind), arg, /*quiet=*/true);
}

extern "C" __declspec(dllexport)
int BotLocker_Unlock(int slot, int kind)
{
    return BotLocker::Dispatch::Unlock(slot,
        static_cast<BotLocker::LockKind>(kind), /*quiet=*/true);
}

extern "C" __declspec(dllexport)
int BotLocker_UnlockAll(int kind)
{
    return BotLocker::Dispatch::UnlockAll(
        static_cast<BotLocker::LockKind>(kind), /*quiet=*/true);
}

extern "C" __declspec(dllexport)
int BotLocker_IsLocked(int slot, int kind)
{
    return BotLocker::Dispatch::IsLocked(slot,
        static_cast<BotLocker::LockKind>(kind));
}

// Set per-slot injected input. Engine pmove runs with these values until cleared.
extern "C" __declspec(dllexport)
int BotLocker_InjectUserCmd(int      slot,
                            uint64_t buttons,
                            float    forwardMove,
                            float    sideMove,
                            float    upMove,
                            float    pitch,
                            float    yaw)
{
    BotLocker::InjectedInput in{buttons, forwardMove, sideMove, upMove, pitch, yaw};
    return BotLocker::InputInjector::SetInput(slot, in) ? 0 : -1;
}

// Stop injecting for one slot. Engine resumes its own UserCmd.
extern "C" __declspec(dllexport)
int BotLocker_ClearInjection(int slot)
{
    return BotLocker::InputInjector::ClearInput(slot) ? 0 : -1;
}

// Stop injecting for every slot at once.
extern "C" __declspec(dllexport)
int BotLocker_ClearAllInjections()
{
    BotLocker::InputInjector::ClearAll();
    return 0;
}

extern "C" __declspec(dllexport)
int BotLocker_GetVersion()
{
    return 6;
}

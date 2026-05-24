// Unified lock dispatch. Three orthogonal per-slot lock kinds.

#pragma once

#include "WeaponLockerState.h"

class IVEngineServer2;

namespace BotLocker
{
    // Lock category. Mirror BotLockerApi.LockKind on the C# side.
    enum class LockKind : int
    {
        All    = 0,
        Aim    = 1,
        Weapon = 2,
    };

    namespace Dispatch
    {
        extern IVEngineServer2 *g_pEngine;

        // Lock a slot. arg = LockTarget int for Weapon kind, ignored otherwise.
        int Lock(int slot, LockKind kind, int arg);

        // Unlock one slot for the given kind.
        int Unlock(int slot, LockKind kind);

        // Clear every slot for the given kind.
        int UnlockAll(LockKind kind);

        // 1 if All/Aim locked; for Weapon returns LockTarget int.
        int IsLocked(int slot, LockKind kind);
    }
}

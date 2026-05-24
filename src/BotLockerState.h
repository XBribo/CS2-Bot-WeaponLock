// Per-slot bot lock flags. All = freeze Update + Upkeep, Aim = Upkeep only.

#pragma once

namespace BotLocker
{
    namespace BotLockerState
    {
        constexpr int kMaxSlots = 64;

        // All lock: blocks Update + Upkeep.
        bool GetAll(int slot);
        void SetAll(int slot, bool locked);
        void ClearAllAll();
        int  CountAll();

        // Aim lock: blocks Upkeep only.
        bool GetAim(int slot);
        void SetAim(int slot, bool locked);
        void ClearAllAim();
        int  CountAim();
    }
}

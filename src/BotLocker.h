// MinHook install/remove for CCSBot::Update + CCSBot::Upkeep.

#pragma once

#include <string>

namespace BotLocker
{
    namespace BotLockerHooks
    {
        // Resolve sigs from gamedata and install both detours.
        bool Install(const std::string &gamedataPath,
                     void *serverIface,
                     char *errorOut, size_t errorOutLen);

        // Disable and remove both detours.
        void Remove();

        // "ok" / "not_attempted" / "failed: <reason>".
        const char *Status();

        // Resolved address (nullptr if not installed).
        void *UpdateAddress();
        void *UpkeepAddress();
    }
}

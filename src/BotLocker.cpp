// CCSBot::Update + CCSBot::Upkeep detours. Skip the body when locked so
// the bot stays passive; Update body keeps +21196 = 1 for the engine gate.

#include "BotLocker.h"
#include "BotLockerState.h"
#include "ccsbot_slot.h"
#include "sig_scan.h"

#include <Windows.h>
#include <MinHook.h>

#include <cstdint>
#include <cstdio>
#include <vector>

using Update_t = void (__fastcall *)(void *bot);
using Upkeep_t = void (__fastcall *)(void *bot);

// CCSBot AI-ran-this-tick byte flag at +21196.
static constexpr int kOffsetAiTickedFlag = 21196;

namespace BotLocker
{
    namespace BotLockerHooks
    {
        static Update_t g_origUpdate   = nullptr;
        static void    *g_addrUpdate   = nullptr;
        static Upkeep_t g_origUpkeep   = nullptr;
        static void    *g_addrUpkeep   = nullptr;
        static bool     g_installed    = false;
        static std::string g_status    = "not_attempted";

        // Skip the AI tick under All lock; still mark it as ran.
        static void __fastcall HookedUpdate(void *bot)
        {
            int slot = CCSBotToSlot(bot);
            if (slot >= 0 && BotLockerState::GetAll(slot))
            {
                *(reinterpret_cast<uint8_t *>(bot) + kOffsetAiTickedFlag) = 1;
                return;
            }
            g_origUpdate(bot);
        }

        // Skip the per-frame view tick under All or Aim lock.
        static void __fastcall HookedUpkeep(void *bot)
        {
            int slot = CCSBotToSlot(bot);
            if (slot >= 0 &&
                (BotLockerState::GetAll(slot) || BotLockerState::GetAim(slot)))
            {
                return;
            }
            g_origUpkeep(bot);
        }

        // Resolve a sig from gamedata against the loaded server.dll.
        static void *ResolveSig(const std::string &gd, HMODULE serverModule,
                                const char *name,
                                char *errorOut, size_t errorOutLen)
        {
            std::string sig = Sig::FindWindowsSig(gd, name);
            if (sig.empty())
            {
                std::snprintf(errorOut, errorOutLen,
                              "gamedata missing '%s.signatures.windows'", name);
                return nullptr;
            }
            std::vector<uint8_t> bytes;
            std::vector<bool>    wild;
            if (!Sig::ParseSigString(sig, bytes, wild))
            {
                std::snprintf(errorOut, errorOutLen,
                              "failed to parse '%s' sig: '%s'", name, sig.c_str());
                return nullptr;
            }
            void *addr = Sig::FindPatternIn(serverModule, bytes, wild);
            if (!addr)
            {
                std::snprintf(errorOut, errorOutLen,
                              "sig '%s' not found in server.dll", name);
                return nullptr;
            }
            return addr;
        }

        bool Install(const std::string &gamedataPath,
                     void *serverIface,
                     char *errorOut, size_t errorOutLen)
        {
            HMODULE serverModule = Sig::ModuleFromInterfacePtr(serverIface);
            if (!serverModule)
            {
                std::snprintf(errorOut, errorOutLen,
                              "ModuleFromInterfacePtr returned null");
                g_status = "failed: no server module";
                return false;
            }

            std::string gd = Sig::ReadFile(gamedataPath);
            if (gd.empty())
            {
                std::snprintf(errorOut, errorOutLen,
                              "failed to read gamedata: %s", gamedataPath.c_str());
                g_status = "failed: gamedata missing";
                return false;
            }

            g_addrUpdate = ResolveSig(gd, serverModule, "CCSBot::Update",
                                      errorOut, errorOutLen);
            if (!g_addrUpdate) { g_status = "failed: Update sig"; return false; }

            g_addrUpkeep = ResolveSig(gd, serverModule, "CCSBot::Upkeep",
                                      errorOut, errorOutLen);
            if (!g_addrUpkeep) { g_status = "failed: Upkeep sig"; return false; }

            // MinHook already initialized by WeaponLockerHooks.
            if (MH_CreateHook(g_addrUpdate,
                              reinterpret_cast<void *>(&HookedUpdate),
                              reinterpret_cast<void **>(&g_origUpdate)) != MH_OK)
            {
                std::snprintf(errorOut, errorOutLen,
                              "MH_CreateHook CCSBot::Update failed");
                g_status = "failed: MH_CreateHook Update";
                return false;
            }
            if (MH_EnableHook(g_addrUpdate) != MH_OK)
            {
                std::snprintf(errorOut, errorOutLen,
                              "MH_EnableHook CCSBot::Update failed");
                MH_RemoveHook(g_addrUpdate);
                g_origUpdate = nullptr;
                g_status = "failed: MH_EnableHook Update";
                return false;
            }

            if (MH_CreateHook(g_addrUpkeep,
                              reinterpret_cast<void *>(&HookedUpkeep),
                              reinterpret_cast<void **>(&g_origUpkeep)) != MH_OK)
            {
                std::snprintf(errorOut, errorOutLen,
                              "MH_CreateHook CCSBot::Upkeep failed");
                MH_DisableHook(g_addrUpdate);
                MH_RemoveHook(g_addrUpdate);
                g_origUpdate = nullptr;
                g_status = "failed: MH_CreateHook Upkeep";
                return false;
            }
            if (MH_EnableHook(g_addrUpkeep) != MH_OK)
            {
                std::snprintf(errorOut, errorOutLen,
                              "MH_EnableHook CCSBot::Upkeep failed");
                MH_RemoveHook(g_addrUpkeep);
                g_origUpkeep = nullptr;
                MH_DisableHook(g_addrUpdate);
                MH_RemoveHook(g_addrUpdate);
                g_origUpdate = nullptr;
                g_status = "failed: MH_EnableHook Upkeep";
                return false;
            }

            g_installed = true;
            g_status = "ok";

            char dbg[256];
            std::snprintf(dbg, sizeof(dbg),
                          "[BotLocker] CCSBot::Update hooked @ %p, "
                          "CCSBot::Upkeep hooked @ %p\n",
                          g_addrUpdate, g_addrUpkeep);
            OutputDebugStringA(dbg);
            return true;
        }

        void Remove()
        {
            if (!g_installed) return;
            MH_DisableHook(g_addrUpkeep);
            MH_RemoveHook(g_addrUpkeep);
            g_origUpkeep = nullptr;
            MH_DisableHook(g_addrUpdate);
            MH_RemoveHook(g_addrUpdate);
            g_origUpdate = nullptr;
            g_installed  = false;
            g_status     = "not_attempted";
        }

        const char *Status()        { return g_status.c_str(); }
        void *UpdateAddress()       { return g_addrUpdate; }
        void *UpkeepAddress()       { return g_addrUpkeep; }
    }
}

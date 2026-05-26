// ProcessUsercmd hook + 64-slot input override table.
// On hook entry, write injected buttons to MovementServices+88 and overwrite
// the cmd's move/view fields, then let the original ProcessUsercmd run.
// I don't know if it works or not this **** funcion.
// SO I HIGHLY RECOMMOND NOT USING THIS ON YOUR PROJECT!!!

#include "InputInjector.h"
#include "ccsbot_slot.h"
#include "sig_scan.h"

#include <Windows.h>
#include <MinHook.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <vector>

// CUserCmd field offsets.
static constexpr int kOffCmdForwardMove    = 44;
static constexpr int kOffCmdSideMove       = 48;
static constexpr int kOffCmdUpMove         = 52;
static constexpr int kOffCmdViewPitch      = 68;
static constexpr int kOffCmdViewYaw        = 72;
static constexpr int kOffCmdViewRoll       = 76;
static constexpr int kOffCmdViewPitchInput = 244;
static constexpr int kOffCmdViewYawInput   = 248;
static constexpr int kOffCmdViewRollInput  = 252;

// CCSPlayer_MovementServices field offsets.
static constexpr int kOffServicesPawn      = 56;
static constexpr int kOffServicesButtons   = 88;

using ProcessUsercmd_t = void (__fastcall *)(void *services, void *cmd);

namespace BotLocker
{
    namespace InputInjector
    {
        struct SlotState
        {
            std::atomic<bool> active{false};
            InjectedInput     input{};
        };

        static std::array<SlotState, kMaxSlots> g_slots;
        static ProcessUsercmd_t g_origProcessUsercmd = nullptr;
        static void            *g_addrProcessUsercmd = nullptr;
        static bool             g_installed          = false;
        static std::string      g_status             = "not_attempted";

        // services -> player slot via pawn ptr at services+56, then m_hController.
        static int ServicesToSlot(void *services)
        {
            if (!services) return -1;
            void *pawn = *reinterpret_cast<void **>(
                reinterpret_cast<char *>(services) + kOffServicesPawn);
            if (!pawn) return -1;
            return ControllerSlotForPawn(pawn);
        }

        // Overwrite buttons/move/view fields.
        static void __fastcall HookedProcessUsercmd(void *services, void *cmd)
        {
            int slot = ServicesToSlot(services);
            if (slot >= 0 && slot < kMaxSlots && g_slots[slot].active.load(std::memory_order_acquire))
            {
                const InjectedInput &p = g_slots[slot].input;
                auto *s = reinterpret_cast<char *>(services);

                // buttons live on MovementServices, not on cmd (CS2 specific).
                *reinterpret_cast<uint64_t *>(s + kOffServicesButtons) = p.buttons;

                if (cmd)
                {
                    auto *c = reinterpret_cast<char *>(cmd);
                    *reinterpret_cast<float *>(c + kOffCmdForwardMove)    = p.forwardMove;
                    *reinterpret_cast<float *>(c + kOffCmdSideMove)       = p.sideMove;
                    *reinterpret_cast<float *>(c + kOffCmdUpMove)         = p.upMove;
                    *reinterpret_cast<float *>(c + kOffCmdViewPitch)      = p.pitch;
                    *reinterpret_cast<float *>(c + kOffCmdViewYaw)        = p.yaw;
                    *reinterpret_cast<float *>(c + kOffCmdViewRoll)       = 0.0f;
                    *reinterpret_cast<float *>(c + kOffCmdViewPitchInput) = p.pitch;
                    *reinterpret_cast<float *>(c + kOffCmdViewYawInput)   = p.yaw;
                    *reinterpret_cast<float *>(c + kOffCmdViewRollInput)  = 0.0f;
                }
            }
            g_origProcessUsercmd(services, cmd);
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

            g_addrProcessUsercmd = ResolveSig(
                gd, serverModule,
                "CCSPlayer_MovementServices::ProcessUsercmd",
                errorOut, errorOutLen);
            if (!g_addrProcessUsercmd)
            {
                g_status = "failed: ProcessUsercmd sig";
                return false;
            }

            if (MH_CreateHook(g_addrProcessUsercmd,
                              reinterpret_cast<void *>(&HookedProcessUsercmd),
                              reinterpret_cast<void **>(&g_origProcessUsercmd)) != MH_OK)
            {
                std::snprintf(errorOut, errorOutLen,
                              "MH_CreateHook ProcessUsercmd failed");
                g_status = "failed: MH_CreateHook";
                return false;
            }
            if (MH_EnableHook(g_addrProcessUsercmd) != MH_OK)
            {
                std::snprintf(errorOut, errorOutLen,
                              "MH_EnableHook ProcessUsercmd failed");
                MH_RemoveHook(g_addrProcessUsercmd);
                g_origProcessUsercmd = nullptr;
                g_status = "failed: MH_EnableHook";
                return false;
            }

            g_installed = true;
            g_status    = "ok";

            char dbg[160];
            std::snprintf(dbg, sizeof(dbg),
                          "[BotLocker] ProcessUsercmd hooked @ %p\n",
                          g_addrProcessUsercmd);
            OutputDebugStringA(dbg);
            return true;
        }

        void Remove()
        {
            if (!g_installed) return;
            MH_DisableHook(g_addrProcessUsercmd);
            MH_RemoveHook(g_addrProcessUsercmd);
            g_origProcessUsercmd = nullptr;
            g_addrProcessUsercmd = nullptr;
            g_installed          = false;
            g_status             = "not_attempted";
            ClearAll();
        }

        const char *Status() { return g_status.c_str(); }

        void *ProcessUsercmdAddress() { return g_addrProcessUsercmd; }

        bool SetInput(int slot, const InjectedInput &input)
        {
            if (slot < 0 || slot >= kMaxSlots) return false;
            g_slots[slot].input = input;
            g_slots[slot].active.store(true, std::memory_order_release);
            return true;
        }

        bool ClearInput(int slot)
        {
            if (slot < 0 || slot >= kMaxSlots) return false;
            g_slots[slot].active.store(false, std::memory_order_release);
            return true;
        }

        void ClearAll()
        {
            for (auto &s : g_slots)
                s.active.store(false, std::memory_order_release);
        }

        bool IsActive(int slot)
        {
            if (slot < 0 || slot >= kMaxSlots) return false;
            return g_slots[slot].active.load(std::memory_order_acquire);
        }

        int CountActive()
        {
            int n = 0;
            for (auto &s : g_slots)
                if (s.active.load(std::memory_order_acquire)) ++n;
            return n;
        }
    }
}

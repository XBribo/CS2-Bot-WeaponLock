// MinHook for CCSPlayer_MovementServices::ProcessUsercmd.
// Lets the host plugin override per-slot UserCmd input (buttons/move/view)
// I don't know if it works or not this **** funcion.
// SO I HIGHLY RECOMMOND NOT USING THIS ON YOUR PROJECT!!!

#pragma once

#include <cstdint>
#include <string>

namespace BotLocker
{
    // Injected input applied for one slot until ClearInput.
    struct InjectedInput
    {
        uint64_t buttons;      // m_nButtonDownMaskPrev bitmask
        float    forwardMove;  // -1..+1, engine scales by maxspeed
        float    sideMove;     // -1..+1
        float    upMove;       // -1..+1 (swim/ladder)
        float    pitch;        // viewangle x
        float    yaw;          // viewangle y
    };

    namespace InputInjector
    {
        // Max bots we track per-slot input for. BotLockerState::kMaxSlots.
        static constexpr int kMaxSlots = 64;

        // Resolve sig and install the ProcessUsercmd hook.
        bool Install(const std::string &gamedataPath,
                     void *serverIface,
                     char *errorOut, size_t errorOutLen);

        // Disable + remove the hook; also wipes all per-slot inputs.
        void Remove();

        const char *Status();

        // Resolved address of the hooked function.
        void *ProcessUsercmdAddress();

        // Set the input applied each tick for this slot. Persists until ClearInput.
        // Returns false if slot is out of range.
        bool SetInput(int slot, const InjectedInput &input);

        // Stop injecting for this slot. Engine resumes its own input.
        bool ClearInput(int slot);

        // Stop injecting for every slot.
        void ClearAll();

        // bl_status queries.
        bool IsActive(int slot);
        int  CountActive();
    }
}

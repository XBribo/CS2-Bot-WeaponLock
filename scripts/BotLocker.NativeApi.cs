// CSS wrapper for BotLocker.dll. DLL is loaded by Metamod, exports
// resolvable. Always check IsCompatible() before use. Main-thread only.

using System.Runtime.InteropServices;

namespace BotLockerApi
{
    // Lock category. Three orthogonal per-slot lock states:
    //   All    - freezes both CCSBot::Update and CCSBot::Upkeep
    //   Aim    - freezes CCSBot::Upkeep only (view fixed, AI keeps ticking)
    //   Weapon - locks the bot's weapon to a specific engine slot
    public enum LockKind
    {
        All    = 0,
        Aim    = 1,
        Weapon = 2,
    }

    // Engine weapon slots. Slot1=primary, Slot2=pistol, Slot3=knife/taser,
    // Slot4=grenades (he/flash/smoke/molotov/decoy share this slot),
    // Slot5=C4.
    public enum LockTarget
    {
        None  = 0,
        Slot1 = 1,
        Slot2 = 2,
        Slot3 = 3,
        Slot4 = 4,
        Slot5 = 5,
    }

    public static class BotLocker
    {
        private const int ExpectedAbiVersion = 4;

        [DllImport("BotLocker", CallingConvention = CallingConvention.Cdecl)]
        private static extern int BotLocker_Lock(int slot, int kind, int arg);

        [DllImport("BotLocker", CallingConvention = CallingConvention.Cdecl)]
        private static extern int BotLocker_Unlock(int slot, int kind);

        [DllImport("BotLocker", CallingConvention = CallingConvention.Cdecl)]
        private static extern int BotLocker_UnlockAll(int kind);

        [DllImport("BotLocker", CallingConvention = CallingConvention.Cdecl)]
        private static extern int BotLocker_IsLocked(int slot, int kind);

        [DllImport("BotLocker", CallingConvention = CallingConvention.Cdecl)]
        private static extern int BotLocker_GetVersion();

        public static bool IsCompatible() => BotLocker_GetVersion() == ExpectedAbiVersion;

        // ---- All / Aim ----
        public static bool Lock(int slot, LockKind kind)
            => BotLocker_Lock(slot, (int)kind, 0) == 0;

        // ---- Weapon: arg is the engine slot to lock onto ----
        public static bool Lock(int slot, LockTarget target)
            => BotLocker_Lock(slot, (int)LockKind.Weapon, (int)target) == 0;

        public static bool Unlock(int slot, LockKind kind)
            => BotLocker_Unlock(slot, (int)kind) == 0;

        public static bool UnlockAll(LockKind kind)
            => BotLocker_UnlockAll((int)kind) == 0;

        // Query. For All/Aim returns true if locked; for Weapon use GetWeaponLock.
        public static bool IsLocked(int slot, LockKind kind)
            => BotLocker_IsLocked(slot, (int)kind) != 0;

        // Weapon-only query: returns the locked weapon slot, or None.
        public static LockTarget GetWeaponLock(int slot)
            => (LockTarget)BotLocker_IsLocked(slot, (int)LockKind.Weapon);
    }
}

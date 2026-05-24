# CS2-Bot-Locker

Metamod:Source 原生插件,提供 CS2 bot 的三种正交锁:

- **武器锁(Weapon)** — 将 bot 钉在指定武器槽位,屏蔽 AI 切枪。
- **视角锁(Aim)** — 冻结 `CCSBot::Upkeep`,bot 视角彻底静止;AI 决策、移动、射击仍正常运行。
- **整体锁(All)** — 同时冻结 `CCSBot::Update` 和 `CCSBot::Upkeep`,让外部代码(如 demo replay)完全接管。

三种锁互相独立,可叠加,可分别清除。

**版本**:0.3.0
**Native ABI**:4

## 槽位对照

| Target  | Engine | 武器                                         |
| ------- | ------ | -------------------------------------------- |
| `Slot1` | 0      | 主武器                                       |
| `Slot2` | 1      | 手枪 / Zeus                                  |
| `Slot3` | 2      | 刀                                           |
| `Slot4` | 3      | 投掷物(HE / 闪 / 烟 / 燃烧 / 诱饵 共槽)    |
| `Slot5` | 4      | C4                                           |

## 安装

- `BotLocker.dll` -> `csgo/addons/BotLocker/bin/win64/`
- `gamedata.json` -> `csgo/addons/BotLocker/`
- `BotLocker.vdf`  -> `csgo/addons/metamod/`

## 编译

环境变量:`HL2SDKCS2`、`MMSOURCE_DEV`、`CSGO_PROTO`,以及 `protoc` 在 PATH 中。

```
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

## 控制台命令

统一命令族 `bl_*`,以 kind(`all` / `aim` / `weapon`)区分锁类型。

```
bl_lock <all|aim|weapon> <slot> [slot1..slot5]
bl_unlock <all|aim|weapon> <slot>
bl_unlock_all <all|aim|weapon>
bl_status
```

`weapon` 模式额外要求第三个参数指定要锁的武器槽位。示例:

```
bl_lock aim 1                # 只冻结 1 号 bot 视角,AI 仍运行
bl_lock all 1                # 完全冻结 1 号 bot
bl_lock weapon 1 slot3       # 把 1 号 bot 切到刀且锁住
bl_unlock aim 1              # 解开 1 号 bot 的视角锁
bl_unlock_all weapon         # 清除所有武器锁
```

## CounterStrikeSharp API

把 `scripts/BotLocker.NativeApi.cs` 拖进项目,在加载时先做兼容性检查。

```csharp
using BotLockerApi;

if (!BotLocker.IsCompatible()) return;          // 需要 ABI 4

// 视角锁 / 整体锁
BotLocker.Lock(slot, LockKind.Aim);             // bool
BotLocker.Lock(slot, LockKind.All);             // bool
BotLocker.Unlock(slot, LockKind.Aim);           // bool
BotLocker.UnlockAll(LockKind.Aim);              // bool
BotLocker.IsLocked(slot, LockKind.Aim);         // bool

// 武器锁
BotLocker.Lock(slot, LockTarget.Slot3);         // bool
BotLocker.Unlock(slot, LockKind.Weapon);        // bool
BotLocker.UnlockAll(LockKind.Weapon);           // bool
BotLocker.GetWeaponLock(slot);                  // LockTarget
```

仅主线程调用。

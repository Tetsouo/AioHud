# AioHud

**A full HUD overlay for FINAL FANTASY XI** — a **Windower 4** plugin drawn live over the game. It replaces and extends the native interface with a cohesive, fully in-game-configurable set of modules.

---

## Features

- **Party & Alliance** — XivParty-style roster (your party + both alliance parties), liquid HP/MP/TP bars, jobs, leader/quartermaster markers, low-HP danger blink, per-member buff icons.
- **Target** — HP + cast/ability bars, distance, TH indicator, debuffs with timers, sub-target box.
- **Player Hub** — your vitals, job/level, equipment, buffs, speed.
- **Timers** — your active **buffs** (exact server durations) and **recasts** on one panel, plus a per-job **track list** with a 4-state focus system (Tracked / Tracked+focus / Hidden / Hidden+focus) and red loss alerts.
- **Minimap** — live radar with player/mob markers, target line, zone map, and a Vana'diel clock (day / moon).
- **Skillchains** — the open skillchain on your target: property, window countdown, and the weapon skills that continue it.
- **Hate List** · **PointWatch** · **Treasure Pool** · **Grimoire (SCH)** · **Zone Tracker** (Dynamis / Abyssea / Omen / Nyzul / Sheol).
- **Profiles** — character-bound: auto-named `Name Main-Sub` and auto-loaded on login / job change (custom profiles stay manual).

Everything is tuned from one full-screen config window with a live preview, in **English or French**.

## Install

1. Download **`AioHud-x.y.z.zip`** from the [latest release](../../releases/latest).
2. **Extract it into your Windower folder** — the one that already contains `plugins\` and `addons\` (e.g. `D:\Windower\`). Everything lands in place automatically.
3. Enable **AioHud** in the Windower launcher (Plugins tab), then start the game — or type `//load AioHud` in game.

That's it. The one-click updater loads itself with Windower from now on.

## Everyday use

| Command | What it does |
|---|---|
| `//aio config` | open the full-screen configuration window (theme, modules, positions…) |
| `//aio edit` | move / resize the boxes directly on screen |
| `//aioupdate` | update to the latest version, in game, with **no window** (the HUD blinks for a moment while it reloads) |

Your settings and profiles live in `plugins\AioHud\data\` and are kept safe across updates.

### Dual-boxing

Windower keeps the plugin locked while **any** client has it loaded. To update on a dual-box setup, `//unload AioHud` on the **other** client first, then run `//aioupdate`.

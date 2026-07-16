# AioUpdate — in-game updater for AioHud

A tiny Windower **Lua addon** that updates the AioHud **plugin** from its latest GitHub release, in game.

## Why a separate addon?
AioHud is a compiled C++ plugin. A loaded DLL is **file-locked** by Windows and a plugin **cannot unload itself** (it would crash mid-call). So this small Lua companion does what the plugin can't: check the latest release, download it, `//unload` the plugin, extract the new files (the DLL is unlocked once unloaded), then `//load` it again.

Your settings are safe: the release zip contains only `AioHud.dll` + `AioHud\assets\` + `AioHud\design\` — your `AioHud\data\` (config, profiles) is never touched.

## Install (once)
1. Copy the **`aioupdate`** folder into `<Windower>\addons\`.
2. In game: `//lua load aioupdate` (or add `lua load aioupdate` to your `<Windower>\scripts\init.txt` to auto-load it).

## Use
```
//aioupdate      check for a newer release and, if any, update AioHud (the HUD blinks off ~3s during the reload)
```
- If you're already on the latest version, it just says so.
- The installed version comes from `plugins\AioHud\data\version.txt`, which the plugin writes on each launch.

## Dual-box note
Windower keeps the DLL locked while **any** client has AioHud loaded. On a dual-box setup, `//unload AioHud` on the **other** client first, then run `//aioupdate` — otherwise the extract can't overwrite the locked DLL and the update is skipped (the addon just reloads the current build).

## How it works
`aioupdate.lua` drives `update.ps1` (PowerShell): `prepare` reads `GET /repos/Tetsouo/AioHud/releases/latest`, compares the tag to `version.txt`, and downloads the `AioHud-*.zip` asset to `data\cache\` if newer; `apply` runs after the unload and `Expand-Archive`s that zip over `plugins\`.

# AioHUD — architecture & developer guide

How the plugin is wired and **how to add a new module fast**. For the reverse-engineered
ABI, memory offsets and D3D rules, see **`REFERENCE.md`** (the source of truth); this file
is the map *above* it — the data flow, the conventions, and the recipes we used to reverse
the game.

---

## 1. Layers (`src/`)

```
gfx/     D3D8 backend, zero game knowledge : textures, fonts, primitive submission (draw.h),
         procedural noise. Everything draws as pre-transformed quads (FVF XYZRHW).
model/   GAME DATA only, zero rendering : memory reads (game_mem), the live party/alliance
         roster + packet handlers (party_state), the per-frame snapshot (gamestate), the
         layout descriptor parser (layout), generated tables (spells/abilities/weapon_skills).
ui/      WIDGETS + the HUD compositor. A widget owns its GPU resources and draws itself from
         the snapshot. Never reads game memory in draw().
plugin/  IPlugin ABI glue (aiohud.cpp) : init/unload, the render hook, the packet hook, and
         the //aio command dispatch (incl. the RE probes).
include/ reusable ABI framework : windower.h (typed host-interface wrappers, all SEH-guarded),
         windower_debug.h (file logging + hexdump + RTTI/vtable helpers for RE).
```

Dependency rule: `ui` → `model` + `gfx`. `model` → nothing in `ui`/`gfx`. `gfx` → nothing.
Keep it that way — it's what lets a widget be tested/reasoned about in isolation.

---

## 2. The per-frame data flow (THE important diagram)

The render hook (`aio_plugin_render6` → `Hud::render`) runs once per game frame:

```
Hud::render(dev) {
    party().load_from_memory();     // 1. roster : self + party + alliance, slots 0..17 (model)
    poll_game_state(state_);        // 2. snapshot : player vitals/jobs, target, leaders, menu (model)
    Frame f { dev, fonts, t, game=&state_ };
    for (w : widgets_) w->draw(f);  // 3. every widget renders FROM the snapshot, never polls memory
}
```

**The seam** (`gamestate.h`): `GameState` is the single per-frame snapshot. `poll_game_state()`
reads each memory pointer-chain **once**; widgets read `f.game->…`. This is what makes the HUD
scale to many widgets — N widgets that need the target read it once, not N times — and keeps the
fragile memory offsets in **one** place (the poller) instead of scattered across every `draw()`.

> The party ROSTER is the one exception: it's bulky and shared, so it lives in the
> `party()` singleton (also refreshed once per frame by `load_from_memory`), reached through
> the same once-per-frame discipline.

`GameState` today: `inGame`, player `me` + hp/mp/tp fractions, `targetId`/`subTargetId`,
alliance/party leader ids, action-menu `menuType`/`menuAction`, and `partyMenuSel` (the
party-window picker cursor). Add a field here when a new widget needs a new datum.

---

## 3. Widgets (`ui/widget.h`)

A widget is a subclass of `Widget`:

| method | when | does |
|---|---|---|
| `type_name()` | — | the layout key (factory + descriptor), e.g. `"PartyList"` |
| `configure(cfg)` | on layout (re)load | reads its keys from the descriptor JSON |
| `measure(w,h)` | on placement | reports content size for right/bottom anchoring (or -1) |
| `ensure(dev)` | every frame, pre-draw | lazily (re)creates GPU resources, no-op once built |
| `draw(f)` | every frame | renders from `f.game` ; leaves no state that corrupts the next widget |
| `on_device_lost()` | on zoning (device recreated) | **forget** GPU handles (don't Release — old device may be dead) |
| `dispose()` | on `//unload` | Release GPU resources (device still alive) |

Placement (`px_/py_/z_/visible_/scale_`) is set by the HUD from `design/exports/layout.json`
(see `EXPORT.md`). The widget draws relative to `px_/py_` and multiplies geometry by `scale_`.

### Add a new module — checklist
1. Add the data it needs to `GameState` + fill it in `poll_game_state()` (model layer).
   Reverse the offset first if it's new (see §5) and document it in `REFERENCE.md §9`.
2. New file `ui/<name>.{h,cpp}` subclassing `Widget`. Read `f.game`, draw via `gfx/draw.h`,
   text via `f.fonts`. Mirror the look from `design/src/panels/<name>` (the mockup is the spec).
3. Register the type in `ui/factory.cpp` (`if (type == "<Name>") return new <Name>();`).
4. Add it to the layout descriptor so the HUD places it.
5. Add the `.cpp` to `build.bat`'s source list. Build (see §6), `//unload`, `deploy`, `//load`.

Conventions: D3D state hygiene (§REFERENCE 10f) — set what you need, the HUD wraps all
widgets in one save/restore state block, but don't leave a bound texture for the next widget.
Snap geometry to whole pixels (`snap()`); draw at the half-pixel offset (REFERENCE §10a).

---

## 4. Reading the live roster (`party_state.cpp`)

`load_from_memory()` (every frame) is the live party+alliance table:
- Anchor: `g = *(LuaCore+0x1C8400)`, `pp = *(g+0x248)` (4 bytes into member[0]), `base = pp-4`,
  self-validated against the player id.
- One member = `0x7C` bytes ; **18 slots** : 0..5 = your party, 6..11 = alliance party 2,
  12..17 = alliance party 3. Counts at `allianceinfo +0x13/+0x14/+0x15`, gated by the party
  leader ids `+0x04/+0x08/+0x0C` (so a wrong count can't spawn phantom boxes).
- `read_member()` does ONE SEH-guarded 0x7C block copy then parses the fields from the local
  buffer (name `+0x0A`, id `+0x1C`, hp/mp/tp `+0x28/+0x2C/+0x30`, %s `+0x34/+0x35`, zone `+0x36`,
  flags `+0x3C`, jobs `+0x71/+0x73`). One source of truth for the offsets, shared by the party
  and alliance loops. Full field table in `REFERENCE.md §9d`.

Packets top up what the array lacks or lags: `0x0DD/0x0DF` (vitals), `0x028` (cast bar),
`0x076` (other members' buffs — self buffs come from `player+0x1C`). See `REFERENCE.md §9b/§9f/§9h`.

---

## 5. Reverse-engineering recipe (how we found the offsets)

Two complementary tools. Use the Ghidra route for *structure*, the live probe for *which field*.

### 5a. Reverse a LuaCore lua-binding (authoritative)
Windower exposes game data as Lua functions (`get_player`, `get_mob_by_target`, …). Reversing
the C function behind a binding tells you the exact pointer chain Windower itself uses.
1. The binding is registered as `lua_pushcclosure(L, FUN_xxxxxxxx, 0); lua_setfield(L,-2,"<name>")`.
   Find `FUN_xxxxxxxx` (the closure pushed right before the `setfield "<name>"`).
2. Decompile it headless:
   ```
   analyzeHeadless <proj> aiohud_re -process LuaCore.dll -noanalysis \
       -scriptPath scripts -postScript DecompOne.java <FUN addr-without-0x>
   ```
   (Ghidra 12, project `re/ghidra_proj/aiohud_re`, LuaCore base `0x10000000`.)
3. Read the field accesses off `DAT_101c8400` (= our `g`). Examples found this way:
   - `get_player` (`FUN_10072040`): self buffs loop over `u16[32]` at `player+0x1C` (0xFF=empty);
     jobs were NOT here (they're in the member array `+0x71/+0x73` — found via the live probe).
   - `get_mob_by_target` (`FUN_100750c0` → resolver `FUN_1008b7d0`): a string→target dispatch that
     indexes the entity array `*(g+0x24)` by a per-type index in the target structs.
> Caveat: the decompiler output is messy for string-heavy functions; use it to find the *struct*
> and *which g+offset*, then confirm the exact field with a live probe.

### 5b. Live differential probe (which field / confirm)
A `//aio` toggle in `aio_plugin_render6` that watches a memory window every few frames and logs
on change (to `aiohud_debug.log`, read it directly). The discipline that cracked hard cases:
- **Scan for a known id** (your server id is distinctive) across a struct → the offset that holds it.
- **Differential / transition**: capture state OFF vs ON (e.g. a menu cursor closed vs open) and
  diff — the field that changes is the one. KEY LESSON (the quartermaster cursor): a value that is
  `0` solo (an index of the lone member) is indistinguishable from zeroed memory — you need **≥2
  members** so the field takes a *non-zero, distinctive* value. Don't burn cycles probing a `0`.
- Resolve indices: `entity = *(*(g+0x24) + index*4)`, server id at `entity+0x00`.

Probes live in `plugin/aiohud.cpp` and are kept (off by default) for re-locating after a client
patch: `//aio tgt2`/`sub` (target struct), `pcur` (party-window picker), `pkt`/`menu`/`dump`/`grabmod`.

### 5c. Worked example — the party-window picker (`REFERENCE.md §9e`)
Goal: frame the member the *Menu→Party→Distribution→Quartermaster* cursor is on. We ruled out
`target_t` and the LuaCore target list (both stay "nothing" → it's NOT a sub-target), then with
2 party members the `//aio pcur` dump showed the focused menu is named **`partywin`** with a
**1-based cursor index at `menu+0x4C`** → member = `rows[index-1]`. One field, covers
quartermaster / lottery / remove / leader.

---

## 6. Build / deploy / iterate

```
build.bat     -> build\AioTest.dll   (32-bit MSVC ; MUST regen the DLL — see gotcha)
# in game:  //unload AioTest
deploy.bat    -> copies into plugins\AioTest.dll
# in game:  //load AioTest
```
- Windower **file-locks** the loaded DLL → always `//unload` before `deploy`.
- **GOTCHA (cost us real time):** `build.bat` now fails on `cl`'s nonzero exit. Before, it only
  checked the DLL *existed*, so a failed compile silently kept the **stale** DLL and reported
  `[build] OK`. If you change the build, keep that exit-code check — and verify the DLL timestamp
  actually moved after a build.
- Demo without the game: `//aio party demo` (+ `alliance1|alliance2 demo`) drives the layout from
  baked rows so you can tune visuals out of game.

---

## 7. Notable conventions in this codebase
- **One source of truth for offsets**: memory layout lives in the poller / `read_member`, never
  duplicated. Document every offset in `REFERENCE.md §9` with how it was found and the date.
- **SEH everywhere we touch game memory**: `safe_read` (4 bytes) or one guarded block copy; a bad
  pointer degrades to a no-op, never a crash. Validate pointers with `valid_ptr`.
- **Snapshot, don't poll-in-draw**: widgets read `f.game`; only the poller (model) reads memory.
- **Pixel-snapped, half-pixel-offset 2D** (REFERENCE §10) — non-negotiable for crisp text/borders.
- Generated tables (`*_gen.h`) come from `scripts/` — don't hand-edit; regenerate.

---

## 8. Edit-layout GUIDES & ZONES  (2026-07-01)

`//aio edit` (`ui_config().editLayout`) lets you arrange the boxes on the live game. The config
overlay (`config_page.cpp`, the `editLayout` branch) draws a floating toolbar; the party/alliance
boxes handle their own drag/resize in `party.cpp`. The **"Rules"** toggle (`editShowLines_`) hides
the whole HUD (`hud.cpp` skips the widget draw) so only the guides + toolbar show.

**Zones (`GuideGroup`, `ui_config.h`).** A zone is a named **rectangle** the user draws by drag
(rubber-band) in Rules mode, stored as **screen fractions** (`x/y/w/h` in 0..1 — resolution
independent). Fixed-capacity array (`guideGroup[GUIDE_GROUPS_MAX]`, no heap), serialized as
`zone=x,y,w,h,allowP,allowA,allowH,role,name` in the config file.
- `allow[ZPERM_PARTY|ALLIANCE|HUB]` — which HUD box may sit on the zone. A box being dragged is
  pushed OUT of any zone that forbids its type (`guide_push_out`, called from `party.cpp`'s drag).
- `role` — `0` = plain keep-out/permission zone ; `1..6` = the **party box for that member count**
  (its rectangle TOP drives the party grow-up) ; `7/8` = **Alliance 1 / 2** (their top/bottom place
  the alliance boxes). This replaces the old full-span reference-LINE handles.

**How zones drive the boxes (`party.cpp`).**
- Party sizing: `guide_party_top(count, sh)` returns the role-`count` zone's top; `party.cpp` grows
  the party box up to it. Falls back to the legacy `partyRef[]` line if that zone is absent.
- Alliance placement: `guide_alliance_refs(ar)` fills `{A1 top, A1 bot, A2 top, A2 bot}` from the
  role 7/8 zones; `party.cpp` uses the bottoms to anchor the alliance boxes. Falls back to `allyRefY[]`.
- Only the party zone's TOP and the alliance zones' TOP/BOTTOM are functional; left/right/other edges
  are visual (like the old L/R/`0` markers, which were visual-only).

The legacy fraction fields (`partyRef[6]`, `partyRefX[2]`, `partyBottomY`, `allyRefY[4]`) are kept as
the **fallback** and as the source values the `+ Party` / `+ Ally` buttons seed the zones from.

**Panel.** Draggable, auto-sized (width fits the title/hint text, height fits the zone list + the
action row), scrollable (mouse wheel or the scrollbar thumb). Rename uses the shared keyboard field
(`nameBuf_`/`nameFocus_`, the same slot-14 hook as the Profile tab) with an **OK** button — no Enter
needed. Input layering: the panel rect (`overPanel`) is computed FIRST so nothing underneath (zone
handles) grabs a click that lands on the panel.

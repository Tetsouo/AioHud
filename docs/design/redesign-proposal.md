---
title: Redesign proposal — organise the interface clean
summary: An opinionated plan to make AioHUD better WITHOUT cutting any capability — the plugin targets thousands of users so every option stays. Reorganise the config around Global vs Per-box, give the live HUD a real reading hierarchy, discipline the palette to "calm + alarms", ship great defaults + preset profiles, and migrate in safe phases. Companion to interface-map.md; decisions marked as "recommend" vs "your call".
source: proposal, 2026-07-02 — validate before mockup/code
---

# Redesign proposal

> Premise (from reading [interface-map.md](interface-map.md)): AioHUD doesn't lack features — it lacks
> **hierarchy, discipline, and opinionated defaults**. **The plugin is for thousands of users, so every
> capability stays** (all 8 gauges, all fonts, the full zones system, every toggle). "Better" here means
> *the good setup is easy to reach and beautiful by default*, with the full depth still one step away —
> **never removing a knob** a power-user relies on. Levers: clearer grouping, great defaults, presets.
>
> Legend: **[Rec]** = my recommendation, do it. **[You]** = your taste decides, I need a call.
> **[Safe]** = no visual change / no data risk. **[Visual]** = needs your in-game eye to validate.

---

## 1. Config: Global vs Per-box

**Problem today.** The Configuration tab mixes scopes behind the box selector. Some controls next to
"Alliance 1" are secretly **Party-only** (Buff Size, Max Buffs, Cursor Size) or **global** (Theme,
Font, Animations, all Typography). You can't tell what a change will hit.

**Proposal.** Split the tab into three honest zones. The box selector then governs **only** genuinely
per-box things.

```
CONFIGURATION
├─ GLOBAL  (one setting, affects everything)
│    Box Theme · Font · Animations (HP blink / TP glow) · Cursor Size · Buffs (Size + Max)
│    Text — quick: one size / outline slider for ALL text
│
├─ PER BOX   [ Party ][ Alliance 1 ][ Alliance 2 ]      ← selector only matters here
│    Size · Bar Height/Width · Gauge Style · Job Badge (+ Size) · Casts · Distance · Border
│    (Party adds: Cost-box border)
│
└─ ADVANCED  (collapsed by default)
     Typography per element (9 × face/size/outline/bold/italic/caps/colour)
     Edit Layout · Zones/Rules · Reset (boxes / all)
```

- **[Rec]** Move Cursor Size + Buffs into **Global** — there is only one cursor and buffs only exist
  on Party, so they were never really "per-box". That alone kills most of the confusion.
- **[Rec]** Add a **Text quick control** (global size + outline for all elements) up top; keep the
  9-element deep editor under Advanced. 90% of "make text bigger" needs one slider, not 9.
- **[Rec]** Put **Edit Layout / Zones / Reset** under Advanced — they're powerful but rare.
- **[Safe]** This whole section is a **relayout of existing controls** — same fields, same behaviour,
  just regrouped. No pixels in-game change; lowest-risk phase.

---

## 2. HUD: a real reading hierarchy

**Problem today.** A member row shows name + badge + 3 gauges + buffs (2×16) + distance + 3 pips +
cast — all at roughly the **same visual weight**. In combat your eye has to hunt.

**Proposal.** Four explicit tiers. Each element is assigned one, and its size/contrast follows.

| Tier | Elements | Treatment |
|---|---|---|
| **1 — Vital** (read at a glance) | Name, **HP** | HP is the row's spine: tallest bar, highest contrast. Name bold. |
| **2 — Combat** (read when you look) | MP, TP, job badge, target cursor | present but quieter than HP; TP only *pops* when WS-ready |
| **3 — Ambient** (peripheral) | distance, leader/QM pips, sub-job, buffs | small, muted, low contrast — there when you want it, never shouting |
| **4 — Alarm** (overrides everything) | low-HP blink, WS-ready, out-of-range veil, **lock red** | the only place saturated colour/motion is allowed |

Concrete moves:
- **[Rec/Visual]** Make **HP the visual spine** — MP/TP thinner or shorter, so HP dominates the row.
- **[Rec/Visual]** Demote tier-3 (distance, pips, sub-job) to a **muted tone** and smaller size, so
  they stop competing with vitals.
- **[Rec]** Reserve **motion** (blink/glow/sweep) strictly for tier-4. Steady state should be still.
- **[You]** How far to push HP dominance vs keeping MP/TP legible for healers/casters — your call;
  we tune it live.

---

## 3. Palette: calm + alarms

**Problem today.** Too many competing semantic colours on screen at once: 5 role hues + HP gradient +
MP blue + TP pink/purple + distance blue/yellow/red + pips white/yellow/green + cursor white/blue/red
+ cast gold. It reads a bit rainbow, and it **buries the colours that matter** (low-HP red, WS pink,
lock red) among decorative ones.

**Proposal.** One rule: **at most ~2 saturated accents visible in steady state; saturation is a
signal, not decoration.** Two palettes:

**Steady palette (calm, low-saturation)** — structure, text, in-range/full states:
- Text / names: off-white and a single muted grey for secondary.
- MP: a calm steady blue (unchanged intent, lower saturation).
- TP (below 1000): a quiet neutral (desaturated), so it doesn't shout while charging.
- Role tints on the badge: keep the 5 roles but **slightly desaturated**, so they don't fight alarms.
- Distance / pips: muted (tier-3) — the *threshold* meaning (near/far) can stay via brightness, not
  loud hues.

**Alarm palette (reserved, high-saturation)** — only states that need your eyes NOW:
- Low HP: red blink (keep).
- WS ready (TP ≥ 1000): the pink pop (keep — and it'll read far better once TP is calm below 1000).
- Out of cast range: the dark veil / red distance (keep).
- Target lock: red hand + frame (keep).

- **[Rec]** Introduce a small **named-colour palette module** (one source of truth) so retinting is a
  data change, not scattered hex literals across `party.cpp`. Makes future theming trivial.
- **[You/Visual]** Exact hues — I'll propose a concrete table in the mockup and we tune it on your
  screen; palette is the most taste-driven part.

---

## 4. Keep ALL the depth — tame it, don't cut it

> **Guiding decision (2026-07-02):** this plugin is meant for **thousands of users**, so **everyone must
> find their fit**. We keep every capability — all 8 gauge styles, all fonts, the full zones system,
> every toggle. The redesign serves newcomers through **great defaults, presets and clearer
> organisation**, never by removing a knob a power-user relies on. "Better" = *easier to reach the good
> setup*, with the full depth still one step away.

- **Gauge styles: keep all 8.** No cut. We just make the **default** one excellent; the other 7 stay as
  the palette a user switches to.
- **Fonts: keep the full list.** No cut. **[Rec]** Only **order it best-first** and pick a strong
  **default** so the common choice is at hand, while every face stays selectable.
- **Zones: keep the system exactly as it is.** *(Decided — it is load-bearing: it guarantees our
  party/alliance boxes always fully **occlude** the game's native party/alliance windows, covering the
  gaps — e.g. the Cost/MP slot when it's empty, which otherwise leaks native fragments. See the Zones
  explainer in [interface-map.md](interface-map.md#part-d--zones--rules-system).)* The redesign only
  aims to make zones **clearer to discover and use**, not simpler-by-removal.
- The lever for "not overwhelming" is therefore **defaults + presets + grouping** (§1, §5), not deletion.

---

## 5. Opinionated defaults (the biggest single win)

A great HUD looks great **before** you open config. Today the defaults are functional but neutral.

- **[Rec]** Ship one **curated default look**: one chosen gauge style, one chosen font, tuned sizes,
  animations on, badge = Main+Sub, distance on for Party / off for Alliance, borders on, the calm
  palette from §3. A fresh install should need **zero** config to look intentional.
- **[Rec]** Ship 2–3 **named preset profiles** ("Clean", "Dense", "Minimal") using the profile system
  that already exists — one click to a whole coherent look, instead of 20 sliders.

---

## 6. Migration plan (safe, phased)

Each phase is its own commit(s) on a savepoint tag, **built + validated in game before the next**
(same rhythm as the gfx work: tag → change → build → you test → revert if bad).

| Phase | What | Risk | Validates |
|---|---|---|---|
| **0** | Config relayout: Global / Per-box / Advanced groupings (§1). No field/behaviour change. | **[Safe]** build-only | pixels in-game unchanged; config just clearer |
| **1** | Palette module + retint steady state; alarms untouched (§3). | **[Visual]** | your eye, over bright/dark zones |
| **2** | HUD hierarchy: HP spine, quiet tier-3, motion only on alarms (§2). | **[Visual]** | in-game, real party |
| **3** | Curated **default look** + named **preset profiles** (§5). Nothing removed. | **[Visual + data]** | fresh install looks intentional |
| **4** | Make **zones easier to discover/use** (clearer entry + explainer in the Help), system unchanged. | **[Visual]** | occlusion still guaranteed |

**Back-compat note.** Nothing is renumbered — all 8 gauge styles and all fonts stay, so existing
`aio_config.txt` / profile indices keep meaning. New defaults/presets are additive (new profile files),
so no saved layout breaks. (If we ever *reorder* the font list for "best-first", keep the stored value
keyed by **name**, not position, so old configs still resolve.)

---

## Open decisions for you
1. ~~Gauge survivors~~ — **decided: keep all 8.**
2. ~~Zones~~ — **decided: keep the whole system as-is** (it guarantees native-window occlusion; broad
   audience needs the flexibility). Redesign only makes it easier to discover/use.
3. ~~Cut anything?~~ — **decided: cut nothing.** Thousands of users → keep every option; tame with
   defaults/presets/grouping.
4. **HP dominance** — how hard do we push HP over MP/TP in the reading hierarchy? (§2)
5. **Default look + presets** — do you want me to propose "Clean / Dense / Minimal", or you name them? (§5)
6. **Start at Phase 0** (safe config relayout — pure organisation, no pixels change) or jump straight to
   the palette/hierarchy visual work?

Answer these and I'll turn §1–§3 into a concrete visual mockup (Artifact) to react to before any code.

## See also
- [Interface map](interface-map.md) — the full current-state inventory this builds on
- [Party visual system](party-visual-system.md) — where the current pixel values live
- [Brief Design — Party / Alliance](brief-party-alliance.md) — the original constraints

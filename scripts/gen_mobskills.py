#!/usr/bin/env python
# Generate src/model/mobskills_gen.h from Windower res/monster_abilities.lua.
# The 0x028 "readies" (category 7) from a MOB carries a monster-ability id (>= 257).
# We only need id -> English name for the target action bar.
import re, io

RES = r"D:\Windower Tetsouo\res\monster_abilities.lua"
OUT = r"D:\Windower Tetsouo\plugins\_aiohud_re\src\model\mobskills_gen.h"

rows = []
with io.open(RES, "r", encoding="utf-8") as f:
    for line in f:
        m = re.search(r'\[(\d+)\]\s*=\s*\{[^}]*?\bid=(\d+)\b[^}]*?\ben="((?:[^"\\]|\\.)*)"', line)
        if not m:
            continue
        mid = int(m.group(2))
        en = m.group(3).replace('\\', '\\\\').replace('"', '\\"')
        rows.append((mid, en))

rows.sort(key=lambda r: r[0])

with io.open(OUT, "w", encoding="utf-8", newline="\n") as o:
    o.write("// mobskills_gen.h -- AUTO-GENERATED from Windower res/monster_abilities.lua (do not edit).\n")
    o.write("// Regenerate: python scripts/gen_mobskills.py\n")
    o.write("#pragma once\n\nnamespace aio {\n\n")   # no include : this table uses only `unsigned`/`const char*` (model must not pull gfx/)
    o.write("struct MobSkillRow { unsigned id; const char* en; };\n")
    o.write("static const MobSkillRow MOB_SKILLS[] = {\n")
    for mid, en in rows:
        o.write('    {%d, "%s"},\n' % (mid, en))
    o.write("};\n")
    o.write("static const int MOB_SKILLS_N = %d;\n\n" % len(rows))
    o.write("inline const MobSkillRow* mobskill_info(unsigned id) {\n")
    o.write("    int lo = 0, hi = MOB_SKILLS_N - 1;\n")
    o.write("    while (lo <= hi) { int mid = (lo + hi) >> 1;\n")
    o.write("        if (MOB_SKILLS[mid].id == id) return &MOB_SKILLS[mid];\n")
    o.write("        if (MOB_SKILLS[mid].id < id) lo = mid + 1; else hi = mid - 1; }\n")
    o.write("    return 0;\n}\n\n")
    o.write("} // namespace aio\n")

print("wrote %d mob skills -> %s" % (len(rows), OUT))

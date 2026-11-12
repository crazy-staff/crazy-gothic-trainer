#!/usr/bin/env python3
"""Extract trainer options (MkRow calls) from src/ into docs/options.json.
Category / subcategory come from the nearest `const char* C = "..."; ... W = "..."` above the row.
Hidden rows are skipped. Names are the in-game (Polish) ones; English text comes from docs/options.en.json."""
import json, os, re, glob, sys

ROOT = os.path.join(os.path.dirname(__file__), "..", "..", "src")
OUTDIR = os.path.join(os.path.dirname(__file__), "..", "..", "docs")
STR = r'"((?:[^"\\]|\\.)*)"'
S_ = r'"(?:[^"\\]|\\.)*"'
MK = re.compile(r'MkRow\(\s*(?P<id>' + S_ + r'|[A-Za-z_][\w\[\]]*)\s*,\s*"(?P<name>(?:[^"\\]|\\.)*)"\s*,\s*"(?P<note>(?:[^"\\]|\\.)*)"\s*,\s*(?P<c>\w+)\s*,\s*(?P<w>\w+)\s*,\s*(?P<kind>K_\w+)(?P<rest>.*)')
CW = re.compile(r'const char\*\s*(\w+)\s*=\s*' + STR)
ASSIGN = re.compile(r'\b(\w+)\s*=\s*' + STR + r'\s*;')
GROUP = re.compile(r'Group\(\s*' + STR)
ST = re.compile(r'St\(\s*(ST_\w+)\s*(?:,\s*' + STR + r')?')
OPTS = re.compile(r'\{\s*(-?\d+)\s*,\s*' + STR + r'\s*\}')
NUMS = re.compile(r'^\s*,\s*(-?[\d.]+)f?\s*,\s*(-?[\d.]+)f?\s*,\s*(-?[\d.]+)f?')

rows = []
for f in sorted(glob.glob(os.path.join(ROOT, "*.cpp")) + glob.glob(os.path.join(ROOT, "mods", "*.cpp"))):
    vars_ = {}
    group = ""
    for ln, line in enumerate(open(f, encoding="utf-8", errors="replace"), 1):
        for m in CW.finditer(line): vars_[m.group(1)] = m.group(2)
        for m in ASSIGN.finditer(line):
            if m.group(1) in vars_: vars_[m.group(1)] = m.group(2)
        g = GROUP.search(line)
        if g: group = g.group(1)
        m = MK.search(line)
        if not m: continue
        rid = m.group("id").strip('"')
        if not re.match(r'^[\w.-]+$', rid): continue   # ids built in loops - skip
        rest = m.group("rest")
        if "Hidden()" in rest: continue
        row = {"id": rid, "name": m.group("name"), "note": m.group("note"), "cat": vars_.get(m.group("c"), m.group("c")),
               "sub": vars_.get(m.group("w"), m.group("w")), "group": group, "kind": m.group("kind")[2:].lower(), "file": os.path.relpath(f, ROOT), "line": ln}
        n = NUMS.match(rest)
        if n and row["kind"] in ("slider", "value"):
            row["def"], row["min"], row["max"] = (float(x) for x in n.groups())
        s = ST.search(rest)
        row["status"] = s.group(1)[3:].lower() if s else "new"
        if s and s.group(2): row["status_note"] = s.group(2)
        o = OPTS.findall(rest.split("Opts(")[1]) if "Opts(" in rest else []
        if o: row["options"] = [{"v": int(v), "label": l} for v, l in o]
        r = re.search(r'Req\(\{([^}]*)\}', rest)
        if r: row["requires"] = re.findall(STR, r.group(1))
        e = re.search(r'Excl\(\{([^}]*)\}', rest)
        if e: row["excludes"] = re.findall(STR, e.group(1))
        rows.append(row)

out = os.path.join(OUTDIR, "options.json")
json.dump(rows, open(out, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
cats = {}
for r in rows: cats.setdefault(r["cat"], {}).setdefault(r["sub"], 0); cats[r["cat"]][r["sub"]] += 1
print(len(rows), "options"); print(json.dumps(cats, ensure_ascii=False, indent=1))

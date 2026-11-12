#!/usr/bin/env python3
"""Build docs/pages/features/** from docs/options.json (extracted from the trainer source)
and docs/options.en.json (English names). Only subsections whose options are all translated are written."""
import json, os, re, shutil

HERE = os.path.dirname(__file__)
DOCS = os.path.join(HERE, "..", "..", "docs")
rows = json.load(open(os.path.join(DOCS, "options.json"), encoding="utf-8"))
en = json.load(open(os.path.join(DOCS, "options.en.json"), encoding="utf-8"))
OUT = os.path.join(DOCS, "pages", "features")
shutil.rmtree(OUT, ignore_errors=True); os.makedirs(OUT)
slug = lambda s: re.sub(r"[^a-z0-9]+", "-", s.lower()).strip("-")
KIND = {"toggle": "On / off", "slider": "Slider", "value": "Value", "button": "Button"}
fmt = lambda v: ("%g" % v) if v is not None else ""

subs = {}
for r in rows: subs.setdefault((r["cat"], r["sub"]), []).append(r)
written = {}
for (cat, sub), rs in subs.items():
    if not all(r["id"] in en["options"] for r in rs): continue
    c_en, s_en = en["categories"].get(cat, cat), en["categories"].get(sub, sub)
    d = os.path.join(OUT, slug(c_en)); os.makedirs(d, exist_ok=True)
    o = [f"---\ntitle: {s_en}\ndescription: {c_en} › {s_en} — every option of this tab.\n---\n"]
    o.append(en.get("intro", {}).get(f"{cat}/{sub}", "") + "\n")
    o.append(f"> **Note:** In the menu: **{cat} › {sub}** (the in-game menu is in Polish for now).\n")
    for r in rs:
        e = en["options"][r["id"]]; name, note = e[0], e[1]
        o.append(f"## {name}\n")
        if note: o.append(note[0].upper() + note[1:] + ".\n" if not re.fullmatch(r"[\w/%. ()+-]{1,12}", note) else f"Unit: {note}.\n")
        facts = [f"**Type:** {KIND.get(r['kind'], r['kind'])}", f"**ID:** `{r['id']}`"]
        if r.get("def") is not None: facts.append(f"**Default:** {fmt(r['def'])} (range {fmt(r['min'])} – {fmt(r['max'])})")
        o.append(" · ".join(facts) + "\n")
        labels = e[2] if len(e) > 2 else None
        if labels: o.append("Choices: " + ", ".join(f"*{l}*" for l in labels) + ".\n")
        if r.get("requires"): o.append("Requires: " + ", ".join(f"**{en['options'].get(x, [x])[0]}**" for x in r["requires"]) + ".\n")
        if r.get("excludes"): o.append("Cannot be on together with: " + ", ".join(en["options"].get(x, [x])[0] for x in r["excludes"]) + " (turning this on turns it off).\n")
    open(os.path.join(d, slug(s_en) + ".md"), "w", encoding="utf-8").write("\n".join(o))
    written.setdefault(c_en, []).append(slug(s_en))
# one sidebar section per menu category ("Features: Character", ...)
json.dump([{"title": f"Features: {c}", "pages": [f"features/{slug(c)}/{p}" for p in sorted(pages)]} for c, pages in sorted(written.items())],
          open(os.path.join(OUT, "sections.json"), "w"), indent=1)
total = sum(len(v) for v in subs.values())
done = sum(len(rs) for (c, s), rs in subs.items() if all(r["id"] in en["options"] for r in rs))
print(f"features: {done}/{total} options documented, pages: {written}")

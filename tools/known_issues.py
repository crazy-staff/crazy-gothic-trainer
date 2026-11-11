#!/usr/bin/env python3
"""Write G1CrazyTrainer_znane.txt (shipped with each release) from GitHub issues labelled opcja:<id>.
Format: target \t state \t number \t build \t title   (the trainer shows these next to each option)."""
import json, subprocess, sys
REPO = "dawidseipold/gothic1-crazy-trainer"
out = sys.argv[1] if len(sys.argv) > 1 else "G1CrazyTrainer_znane.txt"
r = subprocess.run(["gh", "api", f"repos/{REPO}/issues?state=all&per_page=100", "--paginate"], capture_output=True, text=True)
issues = []
try:
    data = json.loads("[" + r.stdout.replace("][", "],[") + "]") if r.stdout.strip() else []
    for page in data: issues += page if isinstance(page, list) else [page]
except json.JSONDecodeError:
    issues = []
lines = ["# Znane problemy - generowane z GitHuba przy wydaniu (target, stan, numer, build, tytuł)"]
for i in issues:
    if "pull_request" in i: continue
    labels = [l["name"] for l in i.get("labels", [])]
    targets = [l[6:] for l in labels if l.startswith("opcja:")] or [""]
    build = next((l[6:] for l in labels if l.startswith("build-")), "0")
    if "propozycja" in labels: continue
    for t in targets:
        lines.append("\t".join([t, i["state"], str(i["number"]), build, i["title"].replace("\t", " ")]))
open(out, "w", encoding="utf-8").write("\n".join(lines) + "\n")
print(f"{len(lines) - 1} wpisów -> {out}")

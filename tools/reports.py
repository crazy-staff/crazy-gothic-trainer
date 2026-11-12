#!/usr/bin/env python3
"""Merge tester report files (G1CrazyTrainer_raporty/raport_*.txt) into one overview, optionally file GitHub issues.

  python3 tools/reports.py raporty/*.txt                 # markdown summary on stdout
  python3 tools/reports.py raporty/*.txt --json out.json # all tickets as JSON
  python3 tools/reports.py raporty/*.txt --github        # create issues for new bug/crash/idea tickets (dedupe by uid)

Also accepts raw G1CrazyTrainer_zgloszenia.txt files and pasted text with @@TICKET blocks.
"""
import argparse, json, os, re, subprocess, sys, zipfile
from collections import defaultdict

REPO = "dawidseipold/gothic1-crazy-trainer"

def unesc(v):
    out, i = [], 0
    while i < len(v):
        if v[i] == "\\" and i + 1 < len(v):
            i += 1; out.append("\n" if v[i] == "n" else v[i])
        else:
            out.append(v[i])
        i += 1
    return "".join(out)

def parse(text, source):
    reports, tickets, cur, section = {}, [], None, None
    meta = {"source": source}
    for line in text.splitlines():
        if line.startswith("@@"):
            section = line[2:].split()[0] if len(line) > 2 else ""
            if line == "@@TICKET": cur = {"_source": source, "_tester": meta.get("tester", "")}; continue
            if line == "@@END" and cur is not None: tickets.append(cur); cur = None; continue
            continue
        if cur is not None:
            if "=" in line:
                k, v = line.split("=", 1); cur[k] = unesc(v)
        elif section == "G1CT-REPORT" and "=" in line:
            k, v = line.split("=", 1); meta[k] = v
        elif section in ("SYSTEM", "ERRORS", "CRASHES"):
            meta.setdefault(section.lower(), []).append(line)
    reports[source] = meta
    return reports, tickets

def gh(*args):
    return subprocess.run(["gh", "api", *args], capture_output=True, text=True)

def existing_uids():
    r = gh(f"repos/{REPO}/issues?state=all&per_page=100", "--paginate", "--jq", ".[].body")
    return set(re.findall(r"[A-Za-z0-9_\-ąćęłńóśźżĄĆĘŁŃÓŚŹŻ]+-\d{6}-\d{6}(?:-\d+)?", r.stdout or ""))

def issue_body(t):
    f = lambda label, v: f"### {label}\n\n{v or '_No response_'}\n\n"
    tgt = f"{t.get('targetName','')} ({t['target']})" if t.get("target") else "ogólne"
    if t.get("type") == "Propozycja":
        return (f("Czego dotyczy", tgt) + f("Opis pomysłu", t.get("steps") or t.get("notes")) + f("Po co / dla kogo", t.get("expected")) +
                f("Build trainera", t.get("build")) + f("Twój nick", t.get("nick")) + f("Kontekst z gry (automatycznie)", "```text\n" + t.get("context", "") + "\n```") +
                f("ID zgłoszenia (z gry)", t.get("uid")))
    return (f("Czego dotyczy", tgt) + f("Kroki — co robiłeś po kolei", t.get("steps")) + f("Co powinno się stać", t.get("expected")) +
            f("Co się stało", (t.get("actual", "") + ("\n\n" + t["notes"] if t.get("notes") else "")).strip()) +
            f("Waga", t.get("severity")) + f("Jak często", t.get("repro")) + f("Build trainera", t.get("build")) + f("Twój nick", t.get("nick")) +
            f("Kontekst z gry (automatycznie)", "```text\n" + t.get("context", "") + "\n```") + f("ID zgłoszenia (z gry)", t.get("uid")) +
            "\n_Utworzone automatycznie z raportu testera._\n")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--json")
    ap.add_argument("--github", action="store_true")
    a = ap.parse_args()
    reports, tickets = {}, {}
    for fn in a.files:
        if fn.lower().endswith(".zip"):   # report with attachments: raport.txt + zalaczniki/*
            z = zipfile.ZipFile(fn)
            text = z.read("raport.txt").decode("utf-8", "replace")
            outdir = os.path.splitext(fn)[0] + "_zalaczniki"
            for n in z.namelist():
                if n.startswith("zalaczniki/") and not n.endswith("/"):
                    os.makedirs(outdir, exist_ok=True); open(os.path.join(outdir, os.path.basename(n)), "wb").write(z.read(n))
        else:
            text = open(fn, encoding="utf-8", errors="replace").read()
        r, ts = parse(text, fn)
        reports.update(r)
        for t in ts: tickets[t.get("uid") or f"{fn}:{len(tickets)}"] = t   # same uid in many files -> one ticket
    ts = list(tickets.values())
    if a.json:
        json.dump({"reports": reports, "tickets": ts}, open(a.json, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
    by = defaultdict(list)
    for t in ts: by[t.get("target") or "ogólne"].append(t)
    print(f"# Raporty testerów: {len(reports)} plików, {len(ts)} wpisów, {len({t.get('nick') for t in ts})} testerów\n")
    for meta in reports.values():
        errs = meta.get("errors", [])
        if errs: print(f"- {meta.get('tester','?')} (build {meta.get('build','?')}): {len(errs)} wyjątków w logu")
    print()
    for target, items in sorted(by.items(), key=lambda kv: -sum(1 for t in kv[1] if t.get("type") in ("Błąd", "Crash", "Częściowo"))):
        name = items[0].get("targetName") or target
        verd = defaultdict(int)
        for t in items: verd[t.get("type")] += 1
        print(f"## {name} (`{target}`) — " + ", ".join(f"{k}: {v}" for k, v in verd.items()))
        for t in items:
            if t.get("verdict") == "1" and not (t.get("actual") or t.get("steps")): continue
            att = f" [załączniki: {t['files']}]" if t.get("files") else ""
            print(f"- **{t.get('type')}** b{t.get('build')} {t.get('nick')}: {t.get('title') or ''} — {(t.get('actual') or t.get('steps') or '').replace(chr(10), ' ')[:200]}{att}")
        print()
    if a.github:
        known = existing_uids()
        made = 0
        for t in ts:
            if t.get("type") not in ("Błąd", "Crash", "Propozycja", "Częściowo") or t.get("uid") in known: continue
            if t.get("verdict") == "1" and not (t.get("actual") or t.get("steps") or t.get("notes")): continue
            prefix = {"Crash": "[Crash] ", "Propozycja": "[Suggestion] "}.get(t["type"], "")
            labels = {"Crash": ["type: crash"], "Propozycja": ["type: enhancement"]}.get(t["type"], ["type: bug"]) + ["source: in-game", "status: needs verification"]
            if t.get("target"): labels.append("option: " + t["target"])
            if t.get("build"): labels.append("build: " + t["build"])
            args = [f"repos/{REPO}/issues", "-f", f"title={prefix}{t.get('title') or t.get('targetName') or 'zgłoszenie'}", "-f", f"body={issue_body(t)}"]
            for l in labels: args += ["-f", f"labels[]={l}"]
            r = gh(*args)
            print(("utworzono: " if r.returncode == 0 else "BŁĄD: ") + (t.get("title") or "") + ("" if r.returncode == 0 else r.stderr), file=sys.stderr)
            made += r.returncode == 0
        print(f"Nowe issues: {made}", file=sys.stderr)

if __name__ == "__main__":
    main()

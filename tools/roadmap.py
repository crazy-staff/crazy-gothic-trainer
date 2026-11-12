#!/usr/bin/env python3
"""Public roadmap from GitHub issues: ROADMAP.md (rendered by GitHub) + site/index.html (GitHub Pages).
Votes = 👍 reactions on an issue. Run by .github/workflows/roadmap.yml; locally: roadmap.py --from issues.json"""
import json, os, sys, html, re, urllib.request, datetime

REPO = os.environ.get("GITHUB_REPOSITORY", "dawidseipold/gothic1-crazy-trainer")

def fetch():
    out, page = [], 1
    tok = os.environ.get("GITHUB_TOKEN", "")
    while True:
        req = urllib.request.Request(f"https://api.github.com/repos/{REPO}/issues?state=all&per_page=100&page={page}",
                                     headers={"Accept": "application/vnd.github+json", **({"Authorization": f"Bearer {tok}"} if tok else {})})
        batch = json.load(urllib.request.urlopen(req))
        if not batch: break
        out += batch; page += 1
    return out

def fetch_ideas():
    """Ideas = comments in the Discussions "Ideas" category (tools/discussions.py). Vote = 👍 on the comment."""
    tok = os.environ.get("GITHUB_TOKEN", "")
    if not tok: return []
    o, n = REPO.split("/")
    q = """query($o:String!,$n:String!){repository(owner:$o,name:$n){discussions(first:100){nodes{title url closed category{slug name}
      comments(first:100){nodes{body url isMinimized reactionGroups{content reactors{totalCount}}}}}}}}"""
    req = urllib.request.Request("https://api.github.com/graphql", json.dumps({"query": q, "variables": {"o": o, "n": n}}).encode(),
                                 {"Authorization": f"Bearer {tok}"})
    try: d = json.load(urllib.request.urlopen(req))["data"]["repository"]["discussions"]["nodes"]
    except Exception as e: print("dyskusje niedostępne:", e); return []
    out = []
    for t in d:
        if t["closed"] or t["category"]["slug"] != "ideas": continue
        theme = t["title"].split(":", 1)[-1].strip()
        for c in t["comments"]["nodes"]:
            if c["isMinimized"]: continue
            body = c["body"]
            lines = [l.strip("# ").strip() for l in re.sub(r"<!--.*?-->", "", body, flags=re.S).strip().splitlines() if l.strip()]
            if not lines: continue
            up = sum(g["reactors"]["totalCount"] for g in c["reactionGroups"] if g["content"] == "THUMBS_UP")
            out.append({"idea": True, "ref": theme, "number": 0, "title": lines[0][:120], "html_url": c["url"], "body": body,
                        "state": "open", "labels": [{"name": "pomysł"}], "reactions": {"+1": up}})
    return out

def ref(i): return i.get("ref") or f"#{i['number']}"

def section(i):
    if i.get("idea"): return "plans" if order(i) < 9999 else "ideas"
    labels = {l["name"] for l in i.get("labels", [])}
    if i["state"] == "closed":
        return "done" if i.get("state_reason") != "not_planned" else None
    if "epik" in labels: return None
    if "w-trakcie" in labels: return "doing"
    if "do-weryfikacji" in labels: return "verify"
    if labels & {"błąd", "bug", "crash"}: return "bugs"
    if "plan" in labels: return "plans"
    if labels & {"pomysł", "propozycja", "sugestia", "enhancement"}: return "ideas"
    return "other"

SECTIONS = [
    ("bugs", "Błędy", "Znane problemy, które jeszcze nie są naprawione."),
    ("doing", "W trakcie", "Nad tym właśnie pracujemy."),
    ("verify", "Do sprawdzenia w grze", "Poprawione w ostatnim buildzie - czeka na potwierdzenie testerów."),
    ("plans", "Plany", "Zaplanowane na kolejne buildy - w kolejności priorytetu (od góry)."),
    ("ideas", "Pomysły - głosuj", "Pomysły są w Dyskusjach: daj 👍 pod komentarzem z pomysłem - najwyżej ocenione robimy najpierw."),
    ("done", "Zrobione", "Ostatnio zamknięte."),
    ("other", "Inne", ""),
]

def votes(i): return (i.get("reactions") or {}).get("+1", 0)
def order(i):   # "Kolejność: N" in the body = priority set by the author (plans)
    m = re.search(r"Kolejno\S*:\s*(\d+)", i.get("body") or "")
    return int(m.group(1)) if m else 9999

def build(issues):
    issues = [i for i in issues if "pull_request" not in i]
    groups = {k: [] for k, _, _ in SECTIONS}
    for i in issues:
        s = section(i)
        if s: groups[s].append(i)
    for k in groups:
        if k == "done": groups[k].sort(key=lambda i: i.get("closed_at") or "", reverse=True); groups[k] = groups[k][:60]
        elif k == "plans": groups[k].sort(key=lambda i: (order(i), -votes(i), i["number"]))
        else: groups[k].sort(key=lambda i: (-votes(i), -i["number"]))
    return groups

def md(groups, now):
    o = [f"# Gothic 1 Crazy Trainer — mapa drogowa\n",
         f"Lista generuje się automatycznie z [GitHub Issues](https://github.com/{REPO}/issues) (ostatnio: {now} UTC).  ",
         f"**Głosowanie:** pomysły są w [Dyskusjach](https://github.com/{REPO}/discussions/categories/ideas) — daj 👍 pod komentarzem z pomysłem.  ",
         f"Błąd: [zgłoś tutaj](https://github.com/{REPO}/issues/new/choose). Nowy pomysł: napisz komentarz w pasującym temacie w Dyskusjach.\n"]
    for k, title, desc in SECTIONS:
        g = groups[k]
        if not g: continue
        o.append(f"\n## {title} ({len(g)})\n")
        if desc: o.append(f"_{desc}_\n")
        o.append("| 👍 | # | Zgłoszenie | Etykiety |\n|---:|---:|---|---|")
        for i in g:
            labs = ", ".join(l["name"] for l in i.get("labels", []) if not l["name"].startswith("build-"))
            t = i["title"].replace("|", "\\|")
            o.append(f"| {votes(i)} | [{ref(i)}]({i['html_url']}) | {t} | {labs} |")
    return "\n".join(o) + "\n"

def page(groups, now):
    def card(i):
        labs = "".join(f'<span class="lab">{html.escape(l["name"])}</span>' for l in i.get("labels", []) if not l["name"].startswith("build-"))
        ms = f'<span class="ms">{html.escape(i["milestone"]["title"])}</span>' if i.get("milestone") else ""
        return (f'<a class="card" href="{i["html_url"]}" target="_blank" data-q="{html.escape((i["title"] + " " + " ".join(l["name"] for l in i.get("labels", []))).lower())}">'
                f'<div class="v"><b>{votes(i)}</b><small>👍</small></div><div class="b"><div class="t">{html.escape(ref(i))} · {html.escape(i["title"])}</div>'
                f'<div class="l">{ms}{labs}</div></div></a>')
    tabs, bodies = [], []
    for n, (k, title, desc) in enumerate(SECTIONS):
        g = groups[k]
        if not g: continue
        tabs.append(f'<button data-t="{k}">{title} <i>{len(g)}</i></button>')
        bodies.append(f'<section id="{k}"><p class="d">{desc}</p>' + "".join(card(i) for i in g) + "</section>")
    return f"""<!DOCTYPE html><html lang="pl"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Gothic 1 Crazy Trainer — mapa drogowa</title><style>
body{{margin:0;font:15px/1.45 system-ui,Segoe UI,sans-serif;background:#13111c;color:#e8e6f0}}
header{{padding:28px 24px 8px;max-width:980px;margin:auto}}h1{{margin:0 0 6px;font-size:26px}}header p{{margin:4px 0;color:#a9a4bd}}
a{{color:#f272d9}}nav{{max-width:980px;margin:12px auto;padding:0 24px;display:flex;flex-wrap:wrap;gap:8px}}
nav button{{background:#221e30;color:#cfcbe0;border:1px solid #332d47;border-radius:20px;padding:7px 14px;cursor:pointer;font:inherit}}
nav button.on{{background:#f272d9;color:#14111b;border-color:#f272d9}}nav i{{font-style:normal;opacity:.7;margin-left:4px}}
#q{{max-width:932px;margin:4px auto 10px;display:block;width:calc(100% - 48px);padding:10px 14px;border-radius:10px;border:1px solid #332d47;background:#1b1828;color:#fff;font:inherit}}
main{{max-width:980px;margin:auto;padding:0 24px 40px}}section{{display:none}}section.on{{display:block}}.d{{color:#a9a4bd}}
.card{{display:flex;gap:14px;align-items:center;background:#1d1a2a;border:1px solid #2b263c;border-radius:12px;padding:12px 14px;margin:8px 0;text-decoration:none;color:inherit}}
.card:hover{{border-color:#f272d9}}.v{{min-width:44px;text-align:center}}.v b{{display:block;font-size:20px}}.v small{{opacity:.7}}
.t{{font-weight:600}}.l{{margin-top:4px}}.lab,.ms{{display:inline-block;font-size:12px;background:#2a2540;border-radius:6px;padding:1px 7px;margin:2px 4px 0 0;color:#c9c3dd}}.ms{{background:#3a2140;color:#f6b3ea}}
</style></head><body><header><h1>Gothic 1 Crazy Trainer — mapa drogowa</h1>
<p>Błędy, plany i pomysły prosto z <a href="https://github.com/{REPO}/issues">GitHub Issues</a>. Aktualizacja: {now} UTC.</p>
<p><b>Jak głosować:</b> pomysły są w <a href="https://github.com/{REPO}/discussions/categories/ideas">Dyskusjach</a> - daj 👍 pod komentarzem z pomysłem. <a href="https://github.com/{REPO}/issues/new/choose">Zgłoś błąd lub pomysł</a> · <a href="https://github.com/{REPO}/releases">Pobierz trener</a></p></header>
<nav>{"".join(tabs)}</nav><input id="q" placeholder="Szukaj (np. drużyna, ślizg, radar)..."><main>{"".join(bodies)}</main>
<script>
const bs=[...document.querySelectorAll('nav button')],ss=[...document.querySelectorAll('section')];
function show(k){{bs.forEach(b=>b.classList.toggle('on',b.dataset.t==k));ss.forEach(s=>s.classList.toggle('on',s.id==k));location.hash=k}}
bs.forEach(b=>b.onclick=()=>show(b.dataset.t));show((location.hash||'').slice(1)||bs[0].dataset.t);
document.getElementById('q').oninput=e=>{{const q=e.target.value.toLowerCase();document.querySelectorAll('.card').forEach(c=>c.style.display=c.dataset.q.includes(q)?'':'none');
if(q)ss.forEach(s=>s.classList.add('on'));else show((location.hash||'').slice(1)||bs[0].dataset.t)}};
</script></body></html>"""

if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[1] == "--from": issues = json.load(open(sys.argv[2], encoding="utf-8"))
    else: issues = fetch() + fetch_ideas()
    g = build(issues)
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M")
    open("ROADMAP.md", "w", encoding="utf-8").write(md(g, now))
    os.makedirs("site", exist_ok=True)
    open("site/index.html", "w", encoding="utf-8").write(page(g, now))
    print({k: len(v) for k, v in g.items()})

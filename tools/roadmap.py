#!/usr/bin/env python3
"""Public roadmap from GitHub issues: ROADMAP.md (rendered by GitHub) + site/index.html (GitHub Pages).
Votes = 👍 reactions on an issue. Run by .github/workflows/roadmap.yml; locally: roadmap.py --from issues.json"""
import json, os, sys, html, re, urllib.request, datetime

REPO = os.environ.get("GITHUB_REPOSITORY", "dawidseipold/gothic-crazy-trainer")

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
    """Ideas = discussions in the "Ideas" category, one per idea; category = label "category: …". Vote = upvote + 👍."""
    tok = os.environ.get("GITHUB_TOKEN", "")
    if not tok: return []
    o, n = REPO.split("/")
    q = """query($o:String!,$n:String!,$c:String){repository(owner:$o,name:$n){discussions(first:100,after:$c){pageInfo{hasNextPage endCursor}
      nodes{number title url body closed upvoteCount category{slug} labels(first:20){nodes{name}} reactionGroups{content reactors{totalCount}}}}}}"""
    out, cur = [], None
    while True:
        req = urllib.request.Request("https://api.github.com/graphql", json.dumps({"query": q, "variables": {"o": o, "n": n, "c": cur}}).encode(),
                                     {"Authorization": f"Bearer {tok}"})
        try: d = json.load(urllib.request.urlopen(req))["data"]["repository"]["discussions"]
        except Exception as e: print("discussions unavailable:", e); return out
        for t in d["nodes"]:
            labs = [l["name"] for l in t["labels"]["nodes"]]
            if t["category"]["slug"] != "ideas" or (t["closed"] and "status: done" not in labs): continue
            kat = next((l.split(":", 1)[1].strip() for l in labs if l.startswith("category:")), None)
            up = t["upvoteCount"] + sum(g["reactors"]["totalCount"] for g in t["reactionGroups"] if g["content"] == "THUMBS_UP")
            out.append({"idea": True, "ref": kat or "to review", "number": t["number"], "title": t["title"][:120], "html_url": t["url"],
                        "body": t["body"] or "", "state": "open", "labels": [{"name": l} for l in labs if not l.startswith("category:")],
                        "reactions": {"+1": up}})
        if not d["pageInfo"]["hasNextPage"]: return out
        cur = d["pageInfo"]["endCursor"]

def prio(i):
    names = [l["name"] for l in i.get("labels", [])]
    return 0 if "priority: high" in names else 1 if "priority: medium" in names else 2

def ref(i): return i.get("ref") or f"#{i['number']}"

def section(i):
    if i.get("idea"):
        st = next((l["name"][8:] for l in i.get("labels", []) if l["name"].startswith("status: ")), "pending")
        return {"rejected": None, "done": "done", "in progress": "doing", "planned": "plans", "approved": "plans"}.get(st, "ideas")
    labels = {l["name"] for l in i.get("labels", [])}
    if "<!-- idea-discussion:" in (i.get("body") or ""): return None   # shown through its idea thread
    if i["state"] == "closed":
        return "done" if i.get("state_reason") != "not_planned" else None
    if "type: epic" in labels or "moderation: spam" in labels: return None
    if "status: in progress" in labels: return "doing"
    if "status: needs verification" in labels: return "verify"
    if labels & {"type: bug", "type: crash"}: return "bugs"
    if labels & {"status: planned", "status: approved"}: return "plans"
    if labels & {"type: idea", "type: enhancement"}: return "ideas"
    return "other"

SECTIONS = [
    ("bugs", "Bugs", "Known problems that are not fixed yet."),
    ("doing", "In progress", "Being worked on right now."),
    ("verify", "To verify in game", "Fixed in the latest build - waiting for testers to confirm."),
    ("plans", "Planned", "Approved ideas and planned work - highest priority first."),
    ("ideas", "Ideas - vote", "Every idea is its own thread in Discussions: vote with the up arrow or 👍 - the most voted get reviewed first."),
    ("done", "Done", "Recently closed."),
    ("other", "Other", ""),
]

def votes(i): return (i.get("reactions") or {}).get("+1", 0)
def order(i):   # "Order: N" (old: "Kolejność: N") in the body = priority set by the author (plans)
    m = re.search(r"(?:Order|Kolejno\S*):\s*(\d+)", i.get("body") or "")
    return int(m.group(1)) if m else 9999

def build(issues):
    issues = [i for i in issues if "pull_request" not in i]
    groups = {k: [] for k, _, _ in SECTIONS}
    for i in issues:
        s = section(i)
        if s: groups[s].append(i)
    for k in groups:
        if k == "done": groups[k].sort(key=lambda i: i.get("closed_at") or "", reverse=True); groups[k] = groups[k][:60]
        elif k == "plans": groups[k].sort(key=lambda i: (prio(i), order(i), -votes(i), i["number"]))
        else: groups[k].sort(key=lambda i: (-votes(i), -i["number"]))
    return groups

def md(groups, now):
    o = [f"# Gothic 1 Crazy Trainer — roadmap\n",
         f"Generated automatically from [GitHub Issues](https://github.com/{REPO}/issues) and [Discussions](https://github.com/{REPO}/discussions) (last update: {now} UTC).  ",
         f"**Voting:** every idea is its own thread in [Discussions](https://github.com/{REPO}/discussions/categories/ideas) — vote with the up arrow or 👍.  ",
         f"Bug: [report it here](https://github.com/{REPO}/issues/new/choose). New idea: [start a thread in Discussions](https://github.com/{REPO}/discussions/new?category=ideas).\n"]
    for k, title, desc in SECTIONS:
        g = groups[k]
        if not g: continue
        o.append(f"\n## {title} ({len(g)})\n")
        if desc: o.append(f"_{desc}_\n")
        o.append("| 👍 | # | Item | Labels |\n|---:|---:|---|---|")
        for i in g:
            labs = ", ".join(l["name"] for l in i.get("labels", []) if not l["name"].startswith("build:"))
            t = i["title"].replace("|", "\\|")
            o.append(f"| {votes(i)} | [{ref(i)}]({i['html_url']}) | {t} | {labs} |")
    return "\n".join(o) + "\n"

def page(groups, now):
    def card(i):
        labs = "".join(f'<span class="lab">{html.escape(l["name"])}</span>' for l in i.get("labels", []) if not l["name"].startswith("build:"))
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
    return f"""<!DOCTYPE html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Gothic 1 Crazy Trainer — roadmap</title><style>
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
</style></head><body><header><h1>Gothic 1 Crazy Trainer — roadmap</h1>
<p>Bugs, plans and ideas straight from <a href="https://github.com/{REPO}/issues">GitHub Issues</a> and <a href="https://github.com/{REPO}/discussions">Discussions</a>. Updated: {now} UTC.</p>
<p><b>How to vote:</b> every idea is its own thread in <a href="https://github.com/{REPO}/discussions/categories/ideas">Discussions</a> - vote with the up arrow or 👍. <a href="https://github.com/{REPO}/issues/new/choose">Report a bug</a> · <a href="https://github.com/{REPO}/discussions/new?category=ideas">Suggest an idea</a> · <a href="https://github.com/{REPO}/releases">Download the trainer</a></p></header>
<nav>{"".join(tabs)}</nav><input id="q" placeholder="Search (e.g. party, glide, radar)..."><main>{"".join(bodies)}</main>
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

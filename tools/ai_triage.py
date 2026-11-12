#!/usr/bin/env python3
"""AI triage + moderation for new issues and discussions (GitHub Models, free tier, GITHUB_TOKEN with models: read).
For every new issue / discussion it:
  - detects spam -> label "moderation: spam", close and lock (no comment; the author can reopen)
  - adds labels: type / area for issues ("status: triage" until the author looks at it), category for ideas
  - finds possible duplicates among open issues and ideas and links them (never closes on its own)
  - points out the wrong place (bug posted as idea, question posted as bug...)
  - asks for English (the only language of the repo) and adds an English summary when the post is in another language
  - welcomes people (not the repo owner) with a short note
The model only suggests: labels it returns are filtered to the ones that exist; the author has the final word.
Manual test without changes:  workflow_dispatch  kind=issue|discussion  number=N  dry_run=true
Run by .github/workflows/ai-triage.yml."""
import json, os, re, sys, urllib.request, urllib.parse, urllib.error

REPO = os.environ["GITHUB_REPOSITORY"]
OWNER, NAME = REPO.split("/")
TOK = os.environ["GITHUB_TOKEN"]
HDR = {"Authorization": f"Bearer {TOK}", "Accept": "application/vnd.github+json"}
MODEL = os.environ.get("AI_MODEL", "openai/gpt-4.1-mini")
DRY = os.environ.get("DRY_RUN", "false") == "true"
BASE = f"https://github.com/{REPO}"

def http(url, body=None, method=None, headers=HDR):
    r = urllib.request.Request(url, json.dumps(body).encode() if body is not None else None, headers, method=method)
    return json.load(urllib.request.urlopen(r))

def gql(q, **v):
    d = http("https://api.github.com/graphql", {"query": q, "variables": v})
    if d.get("errors"): raise RuntimeError(d["errors"])
    return d["data"]

def rest(method, path, body=None):
    try: return http(f"https://api.github.com/repos/{REPO}/{path}", body, method)
    except urllib.error.HTTPError as e:
        if e.code == 404 and method == "GET": return None
        raise

def all_labels():
    out, page = [], 1
    while True:
        b = rest("GET", f"labels?per_page=100&page={page}")
        if not b: return out
        out += [l["name"] for l in b]; page += 1

def context_items():
    iss, page = [], 1
    while page < 4:
        b = rest("GET", f"issues?state=open&per_page=100&page={page}")
        if not b: break
        iss += [f"issue #{i['number']}: {i['title']}" for i in b if "pull_request" not in i]; page += 1
    d = gql("""query($o:String!,$n:String!){repository(owner:$o,name:$n){discussions(first:100,states:OPEN){nodes{number title category{slug}}}}}""",
            o=OWNER, n=NAME)["repository"]["discussions"]["nodes"]
    return iss + [f"{x['category']['slug']} discussion #{x['number']}: {x['title']}" for x in d]

def load_item(kind, number=None, ev=None):
    if kind == "issue":
        i = ev["issue"] if ev else rest("GET", f"issues/{number}")
        return {"kind": "issue", "number": i["number"], "title": i["title"], "body": i.get("body") or "", "author": i["user"]["login"],
                "author_type": i["user"]["type"], "labels": [l["name"] for l in i.get("labels", [])], "category": None, "node_id": i["node_id"]}
    d = gql("""query($o:String!,$n:String!,$x:Int!){repository(owner:$o,name:$n){discussion(number:$x){id number title body
        author{login __typename} category{slug name} labels(first:20){nodes{name}}}}}""", o=OWNER, n=NAME,
            x=number or ev["discussion"]["number"])["repository"]["discussion"]
    return {"kind": "discussion", "number": d["number"], "title": d["title"], "body": d["body"] or "", "author": (d["author"] or {}).get("login", ""),
            "author_type": (d["author"] or {}).get("__typename", ""), "labels": [l["name"] for l in d["labels"]["nodes"]],
            "category": d["category"]["slug"], "node_id": d["id"]}

PROMPT = """You triage posts for "Gothic 1 Crazy Trainer", an in-game trainer / mod menu (DLL, Union + GD3D11) for the PC game Gothic 1.
Posts come from players and testers; the repo is English-only but some posts may be in Polish, German or Russian. Be fair and conservative: when unsure, do less.

Post type: {where}
Title: {title}
Body:
{body}

Allowed labels (use ONLY these, exact text):
- issue type: {types}
- issue area (code area, max 2): {areas}
- idea category (only for discussions in "ideas", max 2): {cats}

Open items (for duplicate detection):
{items}

Return JSON only:
{{"spam": bool,                      // ads, links unrelated to the trainer, gibberish, abuse. A short or low-quality but genuine post is NOT spam.
 "spam_reason": str,
 "language": "en"|"pl"|"other",
 "type": one issue type label or null,   // for issues only
 "areas": [area labels],                 // for issues only
 "categories": [category labels],        // for ideas only
 "duplicates": [numbers],                // only clear matches of the SAME request/problem, max 3
 "wrong_place": "none"|"bug_should_be_issue"|"idea_should_be_discussion"|"question_should_be_qa",
 "english_title": str,                   // English title (same as the original if already English)
 "english_summary": str                  // 1-3 sentences in English; empty if the post is already in English
}}"""

def ask(item, labels, items):
    where = f"GitHub issue" if item["kind"] == "issue" else f"GitHub discussion in category '{item['category']}'"
    p = PROMPT.format(where=where, title=item["title"], body=item["body"][:6000],
                      types=", ".join(l for l in labels if l.startswith("type:") and l not in ("type: epic", "type: idea")),
                      areas=", ".join(l for l in labels if l.startswith("area:")),
                      cats=", ".join(l for l in labels if l.startswith("category:")),
                      items="\n".join(items[:250]))
    r = http("https://models.github.ai/inference/chat/completions",
             {"model": MODEL, "temperature": 0, "response_format": {"type": "json_object"},
              "messages": [{"role": "system", "content": "You are a careful GitHub triage assistant. Output JSON only."},
                           {"role": "user", "content": p}]},
             headers={"Authorization": f"Bearer {TOK}", "Content-Type": "application/json", "Accept": "application/json"})
    return json.loads(r["choices"][0]["message"]["content"])

def add_labels(item, names):
    names = [n for n in names if n not in item["labels"]]
    if not names: return
    if item["kind"] == "issue": rest("POST", f"issues/{item['number']}/labels", {"labels": names})
    else:
        ids = [rest("GET", "labels/" + urllib.parse.quote(n))["node_id"] for n in names]
        gql("mutation($i:ID!,$l:[ID!]!){addLabelsToLabelable(input:{labelableId:$i,labelIds:$l}){clientMutationId}}", i=item["node_id"], l=ids)

def comment(item, text):
    if item["kind"] == "issue": rest("POST", f"issues/{item['number']}/comments", {"body": text})
    else: gql("mutation($d:ID!,$b:String!){addDiscussionComment(input:{discussionId:$d,body:$b}){comment{id}}}", d=item["node_id"], b=text)

def moderate_spam(item):
    add_labels(item, ["moderation: spam"])
    if item["kind"] == "issue":
        rest("PATCH", f"issues/{item['number']}", {"state": "closed", "state_reason": "not_planned"})
        rest("PUT", f"issues/{item['number']}/lock", {"lock_reason": "spam"})
    else:
        gql("mutation($d:ID!){closeDiscussion(input:{discussionId:$d,reason:OUTDATED}){discussion{id}}}", d=item["node_id"])
        gql("mutation($d:ID!){lockLockable(input:{lockableId:$d,lockReason:SPAM}){clientMutationId}}", d=item["node_id"])

WRONG = {
    "bug_should_be_issue": "This looks like a bug report. Bugs are tracked in [Issues]({b}/issues/new/choose) (or send a report from the in-game Tester hub).",
    "idea_should_be_discussion": "This looks like a new feature idea. Ideas live in [Discussions → Ideas]({b}/discussions/categories/ideas), where people can vote for them.",
    "question_should_be_qa": "This looks like a question — [Q&A]({b}/discussions/categories/q-a) is the best place for it.",
}

def main():
    kind = os.environ.get("ITEM_KIND") or ("issue" if os.environ["GITHUB_EVENT_NAME"] == "issues" else "discussion")
    ev = None if os.environ.get("ITEM_NUMBER") else json.load(open(os.environ["GITHUB_EVENT_PATH"], encoding="utf-8"))
    item = load_item(kind, int(os.environ["ITEM_NUMBER"]) if os.environ.get("ITEM_NUMBER") else None, ev)
    if not DRY and (item["author_type"] == "Bot" or item["author"].endswith("[bot]")): print("bot post - skip"); return
    owner = item["author"].lower() == OWNER.lower()
    labels = all_labels()
    try: res = ask(item, labels, context_items())
    except Exception as e:
        print("AI unavailable:", e); res = None
    print(json.dumps(res, ensure_ascii=False, indent=1))
    if DRY: return
    if res and res.get("spam") and not owner:
        moderate_spam(item); print("spam:", res.get("spam_reason")); return
    want = []
    if item["kind"] == "issue":
        if res:
            t = res.get("type")
            if t in labels and not any(l.startswith("type:") for l in item["labels"]): want.append(t)
            want += [a for a in (res.get("areas") or [])[:2] if a in labels and a.startswith("area:")]
        if not owner and not any(l.startswith("status:") for l in item["labels"]): want.append("status: triage")
    elif item["category"] == "ideas":
        if res: want += [c for c in (res.get("categories") or [])[:2] if c in labels and c.startswith("category:")]
        if not any(l.startswith("status:") for l in item["labels"]): want.append("status: pending")
    add_labels(item, want)
    if owner: return   # the author does not need a welcome note
    en = []
    if item["kind"] == "discussion" and item["category"] == "ideas":
        en.append(f"Thanks for the idea! It is on the [roadmap]({BASE}/blob/main/ROADMAP.md) — others can vote for it with the up arrow or 👍. "
                  "The author reviews new ideas; approved ones become issues on the project board.")
    elif item["kind"] == "issue":
        en.append("Thanks for the report! The author will check it — please keep an eye on this thread in case of questions.")
    else:
        en.append("Thanks for posting!")
    if res:
        dups = [n for n in (res.get("duplicates") or []) if isinstance(n, int) and n != item["number"]][:3]
        if dups: en.append("Possibly the same as: " + ", ".join(f"#{n}" for n in dups) + " — if so, please add your details and vote there.")
        w = WRONG.get(res.get("wrong_place") or "none")
        if w: en.append(w.format(b=BASE))
        if res.get("language") != "en":
            en.append(f"Please write in **English** — it is the only language of this repository ([rules]({BASE}/blob/main/CONTRIBUTING.md)), "
                      "so everyone can follow and help. You can edit your post.")
            if res.get("english_summary"):
                en.append(f"**English summary:** _{res.get('english_title', '').strip()}_ — {res['english_summary'].strip()}")
    text = "\n\n".join(en) + "\n\n<sub>🤖 Checked automatically (labels, duplicates). The author has the final word.</sub>"
    comment(item, text)

if __name__ == "__main__":
    main()

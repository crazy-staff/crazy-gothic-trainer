#!/usr/bin/env python3
"""Ideas live in GitHub Discussions (category "Ideas"): one discussion per theme, one comment per idea.
Vote = 👍 on the idea's comment. Source of truth: docs/discussions/*.json. Idempotent - safe to rerun.
Each idea comment carries a hidden marker <!-- idea:KEY -->; issues listed in "issues" are closed with a link.
Run by .github/workflows/discussions.yml (GraphQL needs the Actions GITHUB_TOKEN)."""
import json, os, glob, urllib.request

REPO = os.environ.get("GITHUB_REPOSITORY", "dawidseipold/gothic1-crazy-trainer")
OWNER, NAME = REPO.split("/")
TOK = os.environ["GITHUB_TOKEN"]
HDR = {"Authorization": f"Bearer {TOK}", "Accept": "application/vnd.github+json"}

def gql(q, **v):
    r = urllib.request.Request("https://api.github.com/graphql", json.dumps({"query": q, "variables": v}).encode(), HDR)
    d = json.load(urllib.request.urlopen(r))
    if d.get("errors"): raise SystemExit(d["errors"])
    return d["data"]

def rest(method, path, body=None):
    r = urllib.request.Request(f"https://api.github.com/repos/{REPO}/{path}", json.dumps(body).encode() if body else None, HDR, method=method)
    return json.load(urllib.request.urlopen(r))

def repo_info():
    d = gql("""query($o:String!,$n:String!){repository(owner:$o,name:$n){id
      discussionCategories(first:25){nodes{id name slug}}
      discussions(first:100){nodes{id title url comments(first:100){nodes{id body url}}}}}}""", o=OWNER, n=NAME)["repository"]
    return d

def idea_body(i):
    o = f"<!-- idea:{i['key']} -->\n### {i['title']}\n\n{i['desc']}\n"
    if i.get("order"): o += f"\nKolejność: {i['order']} _(priorytet autora)_\n"
    prev = i.get("issues", []) + i.get("discussions", [])
    if prev: o += "\n_Wcześniej: " + ", ".join(f"#{n}" for n in prev) + (f" — zgłosił(a) @{i['author']}" if i.get("author") else "") + "_\n"
    return o + "\n👍 pod tym komentarzem = głos na ten pomysł. Odpowiadaj w wątku tego komentarza."

def main():
    info = repo_info()
    cats = {c["slug"]: c for c in info["discussionCategories"]["nodes"]}
    cat = cats.get("ideas") or next((c for c in cats.values() if c["name"].lower() in ("ideas", "pomysły", "pomysly")), None)
    if not cat: raise SystemExit(f"Brak kategorii Ideas: {list(cats)}")
    existing = {d["title"]: d for d in info["discussions"]["nodes"]}
    for f in sorted(glob.glob("docs/discussions/*.json")):
        t = json.load(open(f, encoding="utf-8"))
        d = existing.get(t["title"])
        if not d:
            body = (f"{t['intro']}\n\n**Jak to działa:** każdy pomysł to osobny komentarz poniżej. "
                    "Daj 👍 pod komentarzem, żeby na niego zagłosować, i dyskutuj w jego wątku. "
                    "Nowy pomysł z tego tematu? Napisz komentarz. Najwyżej ocenione trafiają do planu "
                    f"([mapa drogowa](https://github.com/{REPO}/blob/main/ROADMAP.md)).")
            d = gql("""mutation($r:ID!,$c:ID!,$t:String!,$b:String!){createDiscussion(input:{repositoryId:$r,categoryId:$c,title:$t,body:$b})
                       {discussion{id title url comments(first:1){nodes{id body url}}}}}""",
                    r=info["id"], c=cat["id"], t=t["title"], b=body)["createDiscussion"]["discussion"]
            print("utworzono", d["url"])
        for i in t["ideas"]:
            mark = f"<!-- idea:{i['key']} "
            url = next((c["url"] for c in d["comments"]["nodes"] if c["body"].startswith(mark)), None)
            if not url:
                url = gql("""mutation($d:ID!,$b:String!){addDiscussionComment(input:{discussionId:$d,body:$b}){comment{url}}}""",
                          d=d["id"], b=idea_body(i))["addDiscussionComment"]["comment"]["url"]
                print("  +", i["title"])
            for n in i.get("discussions", []):   # loose idea someone posted on their own -> merged into this theme
                dd = gql("""query($o:String!,$n:String!,$x:Int!){repository(owner:$o,name:$n){discussion(number:$x){id closed}}}""",
                         o=OWNER, n=NAME, x=n)["repository"]["discussion"]
                if dd and not dd["closed"]:
                    gql("""mutation($d:ID!,$b:String!){addDiscussionComment(input:{discussionId:$d,body:$b}){comment{id}}}""", d=dd["id"],
                        b=f"Dzięki! Dopisałem ten pomysł do tematu, gdzie zbieramy podobne: {url}\n\nTam można na niego głosować 👍 i dalej o nim rozmawiać. Zamykam ten wątek, żeby głosy się nie rozdzielały.")
                    gql("""mutation($d:ID!){closeDiscussion(input:{discussionId:$d,reason:DUPLICATE}){discussion{id}}}""", d=dd["id"])
                    print("    zamknięto dyskusję #", n)
            for n in i.get("issues", []):
                iss = rest("GET", f"issues/{n}")
                if iss["state"] == "open":
                    rest("POST", f"issues/{n}/comments", {"body": f"Pomysł przeniesiony do Dyskusji, gdzie łatwiej głosować i rozmawiać: {url}\n\nZamykam zgłoszenie - dalej śledzimy go tam."})
                    rest("PATCH", f"issues/{n}", {"state": "closed", "state_reason": "not_planned"})
                    print("    zamknięto #", n)

if __name__ == "__main__":
    main()

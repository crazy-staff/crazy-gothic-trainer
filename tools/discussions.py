#!/usr/bin/env python3
"""Ideas live in GitHub Discussions (category "Ideas"): ONE discussion per idea, sorted by labels "kategoria: …".
Vote = upvote / 👍 on the discussion. Source of truth: docs/discussions/<kategoria>.json:
  {"label": "kategoria: Teleport", "label_desc": "...", "ideas": [{"key", "title", "desc", "order"?, "issues"?: [n], "discussion"?: n}]}
- idea without "discussion": a discussion is created (hidden marker <!-- idea:KEY --> makes reruns safe)
- idea with "discussion": n  -> someone's own thread, only labelled (nothing is moved or closed)
- "issues": old issues are closed with a link. Old theme threads ("Pomysły: …") are closed with links to the new ones.
Run by .github/workflows/discussions.yml (GraphQL needs the Actions GITHUB_TOKEN)."""
import json, os, glob, urllib.request, urllib.parse

REPO = os.environ.get("GITHUB_REPOSITORY", "dawidseipold/gothic1-crazy-trainer")
OWNER, NAME = REPO.split("/")
HDR = {"Authorization": f"Bearer {os.environ['GITHUB_TOKEN']}", "Accept": "application/vnd.github+json"}

def gql(q, **v):
    r = urllib.request.Request("https://api.github.com/graphql", json.dumps({"query": q, "variables": v}).encode(), HDR)
    d = json.load(urllib.request.urlopen(r))
    if d.get("errors"): raise SystemExit(d["errors"])
    return d["data"]

def rest(method, path, body=None):
    r = urllib.request.Request(f"https://api.github.com/repos/{REPO}/{path}", json.dumps(body).encode() if body else None, HDR, method=method)
    try: return json.load(urllib.request.urlopen(r))
    except urllib.error.HTTPError as e:
        if e.code == 404 and method == "GET": return None
        raise

def all_discussions():
    out, cur = [], None
    while True:
        d = gql("""query($o:String!,$n:String!,$c:String){repository(owner:$o,name:$n){id
          discussionCategories(first:25){nodes{id name slug}}
          discussions(first:100,after:$c){pageInfo{hasNextPage endCursor}nodes{id number title url body closed labels(first:20){nodes{name}}}}}}""",
                o=OWNER, n=NAME, c=cur)["repository"]
        out += d["discussions"]["nodes"]
        if not d["discussions"]["pageInfo"]["hasNextPage"]: return d, out
        cur = d["discussions"]["pageInfo"]["endCursor"]

def label_id(name, desc):
    l = rest("GET", "labels/" + urllib.parse.quote(name))
    if not l: l = rest("POST", "labels", {"name": name, "color": "8a63d2", "description": desc[:100]})
    return l["node_id"]

def idea_body(i):
    o = f"<!-- idea:{i['key']} -->\n{i['desc']}\n"
    if i.get("order"): o += f"\nKolejność: {i['order']} _(priorytet autora)_\n"
    if i.get("issues"): o += "\n_Wcześniej: " + ", ".join(f"#{n}" for n in i["issues"]) + "_\n"
    return o + "\n**Głosuj:** strzałka w górę albo 👍 pod tym wątkiem. Uwagi i szczegóły piszcie w komentarzach."

def comment(did, body):
    gql("mutation($d:ID!,$b:String!){addDiscussionComment(input:{discussionId:$d,body:$b}){comment{id}}}", d=did, b=body)

def main():
    repo, discs = all_discussions()
    cats = {c["slug"]: c for c in repo["discussionCategories"]["nodes"]}
    cat = cats.get("ideas") or next((c for c in cats.values() if c["name"].lower() in ("ideas", "pomysły", "pomysly")), None)
    if not cat: raise SystemExit(f"Brak kategorii Ideas: {list(cats)}")
    by_mark = {d["body"].split("-->")[0] + "-->": d for d in discs if d["body"].startswith("<!-- idea:")}
    by_num = {d["number"]: d for d in discs}
    links = {}   # key -> url, for closing old theme threads
    for f in sorted(glob.glob("docs/discussions/*.json")):
        t = json.load(open(f, encoding="utf-8"))
        lid = label_id(t["label"], t.get("label_desc", ""))
        for i in t["ideas"]:
            if i.get("discussion"): d = by_num[i["discussion"]]
            else:
                d = by_mark.get(f"<!-- idea:{i['key']} -->")
                if not d:
                    d = gql("""mutation($r:ID!,$c:ID!,$t:String!,$b:String!){createDiscussion(input:{repositoryId:$r,categoryId:$c,title:$t,body:$b})
                               {discussion{id number title url body closed labels(first:20){nodes{name}}}}}""",
                            r=repo["id"], c=cat["id"], t=i["title"], b=idea_body(i))["createDiscussion"]["discussion"]
                    print("utworzono", d["url"], i["title"])
            links[i["title"]] = d["url"]
            if t["label"] not in {l["name"] for l in d["labels"]["nodes"]}:
                gql("mutation($i:ID!,$l:[ID!]!){addLabelsToLabelable(input:{labelableId:$i,labelIds:$l}){clientMutationId}}", i=d["id"], l=[lid])
                print("  etykieta", t["label"], "->", d["number"])
            for n in i.get("issues", []):
                iss = rest("GET", f"issues/{n}")
                if iss and iss["state"] == "open":
                    rest("POST", f"issues/{n}/comments", {"body": f"Pomysł przeniesiony do Dyskusji, gdzie łatwiej głosować i rozmawiać: {d['url']}"})
                    rest("PATCH", f"issues/{n}", {"state": "closed", "state_reason": "not_planned"})
                    print("  zamknięto #", n)
    for d in discs:   # old layout: one thread per theme
        if d["title"].startswith("Pomysły:") and not d["closed"]:
            old = theme_titles(d["number"])
            comment(d["id"], "Każdy pomysł ma teraz własny wątek (kategorie = etykiety). Głosujcie tam:\n" +
                    "\n".join(f"- [{t}]({u})" for t, u in links.items() if f"### {t}" in old))
            gql("mutation($d:ID!){closeDiscussion(input:{discussionId:$d,reason:OUTDATED}){discussion{id}}}", d=d["id"])
            print("zamknięto stary temat", d["number"])

def theme_titles(num):
    d = gql("""query($o:String!,$n:String!,$x:Int!){repository(owner:$o,name:$n){discussion(number:$x){comments(first:100){nodes{body}}}}}""",
            o=OWNER, n=NAME, x=num)["repository"]["discussion"]
    return "\n".join(c["body"] for c in d["comments"]["nodes"])

if __name__ == "__main__":
    main()

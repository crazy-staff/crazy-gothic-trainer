#!/usr/bin/env python3
"""Ideas live in GitHub Discussions (category "Ideas"): ONE discussion per idea, sorted by labels "category: ...".
Vote = upvote / 👍 on the discussion. Source of truth: docs/discussions/<category>.json:
  {"label": "category: Teleport", "ideas": [{"key", "title", "desc", "pl": {"title", "desc"}, "priority": "high|medium|low",
    "status": "pending", "order"?, "note"?, "related"?: [..], "credit"?, "issues"?: [n], "discussion"?: n, "addenda"?: [{"key", "text", "pl"}]}]}
- idea without "discussion": a discussion is created; the hidden marker <!-- idea:KEY --> makes reruns safe,
  and title/body are updated when the JSON changes
- idea with "discussion": n -> someone's own thread, only labelled (their text is never touched)
- priority / status labels are set only when missing, so later changes made on GitHub win
- "issues": old issues are closed with a link
English first, Polish version in a collapsed <details> block.
Run by .github/workflows/discussions.yml (GraphQL needs the Actions GITHUB_TOKEN)."""
import json, os, glob, urllib.request, urllib.parse, urllib.error

REPO = os.environ.get("GITHUB_REPOSITORY", "dawidseipold/gothic1-crazy-trainer")
OWNER, NAME = REPO.split("/")
HDR = {"Authorization": f"Bearer {os.environ['GITHUB_TOKEN']}", "Accept": "application/vnd.github+json"}

class GqlError(Exception): pass

def gql(q, **v):
    r = urllib.request.Request("https://api.github.com/graphql", json.dumps({"query": q, "variables": v}).encode(), HDR)
    d = json.load(urllib.request.urlopen(r))
    if d.get("errors"): raise GqlError(d["errors"])
    return d["data"]

def rest(method, path, body=None):
    r = urllib.request.Request(f"https://api.github.com/repos/{REPO}/{path}", json.dumps(body).encode() if body else None, HDR, method=method)
    try: return json.load(urllib.request.urlopen(r))
    except urllib.error.HTTPError as e:
        if e.code == 404 and method == "GET": return None
        raise

DISC = "id number title url body closed labels(first:20){nodes{name}} comments(first:100){nodes{id body}}"

def all_discussions():
    out, cur = [], None
    while True:
        d = gql("""query($o:String!,$n:String!,$c:String){repository(owner:$o,name:$n){id
          discussionCategories(first:25){nodes{id name slug}}
          discussions(first:50,after:$c){pageInfo{hasNextPage endCursor}nodes{""" + DISC + "}}}}", o=OWNER, n=NAME, c=cur)["repository"]
        out += d["discussions"]["nodes"]
        if not d["discussions"]["pageInfo"]["hasNextPage"]: return d, out
        cur = d["discussions"]["pageInfo"]["endCursor"]

_labels = {}
def label_id(name):
    if name not in _labels:
        l = rest("GET", "labels/" + urllib.parse.quote(name))
        if not l: l = rest("POST", "labels", {"name": name, "color": "8a63d2"})
        _labels[name] = l["node_id"]
    return _labels[name]

def idea_body(i):
    o = f"<!-- idea:{i['key']} -->\n{i['desc']}\n"
    if i.get("order"): o += f"\nOrder: {i['order']} _(author's priority)_\n"
    if i.get("related"): o += "\nRelated: " + ", ".join(i["related"]) + "\n"
    if i.get("credit"): o += f"\n**Source / credit:** {i['credit']}\n"
    if i.get("issues"): o += "\n_Previously: " + ", ".join(f"#{n}" for n in i["issues"]) + "_\n"
    if i.get("note"): o += f"\n_{i['note']}_\n"
    o += "\n**Vote:** up arrow or 👍 on this thread. Details and feedback go in the comments.\n"
    if i.get("pl"):
        o += f"\n<details><summary>🇵🇱 Po polsku</summary>\n\n**{i['pl']['title']}**\n\n{i['pl']['desc']}\n\nGłosuj strzałką w górę albo 👍. Możesz komentować po polsku.\n</details>\n"
    return o

def addendum_body(a):
    o = f"<!-- add:{a['key']} -->\n**Merged idea:** {a['text']}"
    if a.get("pl"): o += f"\n\n<details><summary>🇵🇱 Po polsku</summary>\n\n{a['pl']}\n</details>"
    return o

def main():
    repo, discs = all_discussions()
    cats = {c["slug"]: c for c in repo["discussionCategories"]["nodes"]}
    cat = cats.get("ideas")
    if not cat: raise SystemExit(f"No Ideas category: {list(cats)}")
    for d in discs:   # retired layout (one thread per theme, Polish) - closed threads whose links are dead now
        if d["closed"] and d["title"].startswith("Pomysły:"):
            gql("mutation($d:ID!){deleteDiscussion(input:{id:$d}){discussion{id}}}", d=d["id"]); print("deleted old theme thread", d["number"])
    by_mark = {d["body"].split("-->")[0] + "-->": d for d in discs if d["body"].startswith("<!-- idea:")}
    by_num = {d["number"]: d for d in discs}
    for f in sorted(glob.glob("docs/discussions/*.json")):
        t = json.load(open(f, encoding="utf-8"))
        for i in t["ideas"]:
            if i.get("discussion"): d = by_num[i["discussion"]]
            else:
                d = by_mark.get(f"<!-- idea:{i['key']} -->")
                body = idea_body(i)
                if not d:
                    d = gql("""mutation($r:ID!,$c:ID!,$t:String!,$b:String!){createDiscussion(input:{repositoryId:$r,categoryId:$c,title:$t,body:$b})
                               {discussion{""" + DISC + "}}}", r=repo["id"], c=cat["id"], t=i["title"], b=body)["createDiscussion"]["discussion"]
                    print("created", d["url"], i["title"])
                elif d["title"] != i["title"] or d["body"].strip() != body.strip():
                    try:
                        gql("mutation($d:ID!,$t:String!,$b:String!){updateDiscussion(input:{discussionId:$d,title:$t,body:$b}){discussion{id}}}",
                            d=d["id"], t=i["title"], b=body)
                        print("updated", d["number"], i["title"])
                    except GqlError as e:   # the Actions token may not edit discussions: recreate (only while it has no votes/comments)
                        print("update refused:", d["number"], e)
                        info = gql("query($o:String!,$n:String!,$x:Int!){repository(owner:$o,name:$n){discussion(number:$x){upvoteCount comments(first:20){nodes{body}}}}}",
                                   o=OWNER, n=NAME, x=d["number"])["repository"]["discussion"]
                        own = all(c["body"].startswith("<!-- add:") for c in info["comments"]["nodes"])
                        if info["upvoteCount"] or not own: print("  kept (has votes or comments)"); continue
                        gql("mutation($d:ID!){deleteDiscussion(input:{id:$d}){discussion{id}}}", d=d["id"])
                        old = d["number"]
                        d = gql("""mutation($r:ID!,$c:ID!,$t:String!,$b:String!){createDiscussion(input:{repositoryId:$r,categoryId:$c,title:$t,body:$b})
                                   {discussion{""" + DISC + "}}}", r=repo["id"], c=cat["id"], t=i["title"], b=body)["createDiscussion"]["discussion"]
                        print(f"recreated #{old} -> #{d['number']}", i["title"])
            for a in i.get("addenda", []):   # merged ideas -> one comment each (marker <!-- add:KEY -->)
                m, body = f"<!-- add:{a['key']} -->", addendum_body(a)
                c = next((c for c in d["comments"]["nodes"] if c["body"].startswith(m)), None)
                if not c:
                    gql("mutation($d:ID!,$b:String!){addDiscussionComment(input:{discussionId:$d,body:$b}){comment{id}}}", d=d["id"], b=body)
                    print("  merged", a["key"], "->", d["number"])
                elif c["body"].strip() != body.strip():
                    gql("mutation($c:ID!,$b:String!){updateDiscussionComment(input:{commentId:$c,body:$b}){comment{id}}}", c=c["id"], b=body)
                    print("  updated addendum", a["key"])
            have = {l["name"] for l in d["labels"]["nodes"]}
            want = [t["label"]] + i.get("labels", [])
            if i.get("priority") and not any(x.startswith("priority:") for x in have): want.append("priority: " + i["priority"])
            if i.get("status") and not any(x.startswith("status:") for x in have): want.append("status: " + i["status"])
            miss = [x for x in want if x not in have]
            if miss:
                gql("mutation($i:ID!,$l:[ID!]!){addLabelsToLabelable(input:{labelableId:$i,labelIds:$l}){clientMutationId}}",
                    i=d["id"], l=[label_id(x) for x in miss])
                print("  labels", miss, "->", d["number"])
            for n in i.get("issues", []):
                iss = rest("GET", f"issues/{n}")
                if iss and iss["state"] == "open":
                    rest("POST", f"issues/{n}/comments", {"body": f"Moved to Discussions, where it is easier to vote and talk: {d['url']}"})
                    rest("PATCH", f"issues/{n}", {"state": "closed", "state_reason": "not_planned"})
                    print("  closed #", n)

if __name__ == "__main__":
    main()

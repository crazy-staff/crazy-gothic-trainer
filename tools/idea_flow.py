#!/usr/bin/env python3
"""Idea → issue → done.
  discussion labeled "status: approved"  -> creates a linked issue (type: enhancement, same priority, status: approved)
                                           and comments the link in the discussion; the project board picks the issue up
  issue closed as completed              -> the linked idea gets "status: done" and is closed as resolved
  issue reopened                         -> the idea goes back to "status: approved" and is reopened
Link markers: issue body has <!-- idea-discussion:N -->, discussion comment has <!-- idea-issue:N -->.
Run by .github/workflows/idea-flow.yml."""
import json, os, re, urllib.request, urllib.parse, urllib.error

REPO = os.environ["GITHUB_REPOSITORY"]
OWNER, NAME = REPO.split("/")
HDR = {"Authorization": f"Bearer {os.environ['GITHUB_TOKEN']}", "Accept": "application/vnd.github+json"}
EV = json.load(open(os.environ["GITHUB_EVENT_PATH"], encoding="utf-8"))

def gql(q, **v):
    r = urllib.request.Request("https://api.github.com/graphql", json.dumps({"query": q, "variables": v}).encode(), HDR)
    d = json.load(urllib.request.urlopen(r))
    if d.get("errors"): raise SystemExit(d["errors"])
    return d["data"]

def rest(method, path, body=None):
    r = urllib.request.Request(f"https://api.github.com/repos/{REPO}/{path}", json.dumps(body).encode() if body else None, HDR, method=method)
    try: return json.load(urllib.request.urlopen(r))
    except urllib.error.HTTPError as e:
        if e.code == 404: return None
        raise

def label_ids(names):
    return [rest("GET", "labels/" + urllib.parse.quote(n))["node_id"] for n in names]

def set_status(disc, status):
    old = [l["name"] for l in disc["labels"]["nodes"] if l["name"].startswith("status:")]
    if old: gql("mutation($i:ID!,$l:[ID!]!){removeLabelsFromLabelable(input:{labelableId:$i,labelIds:$l}){clientMutationId}}", i=disc["id"], l=label_ids(old))
    gql("mutation($i:ID!,$l:[ID!]!){addLabelsToLabelable(input:{labelableId:$i,labelIds:$l}){clientMutationId}}", i=disc["id"], l=label_ids([status]))

def discussion(num):
    return gql("""query($o:String!,$n:String!,$x:Int!){repository(owner:$o,name:$n){discussion(number:$x){id number title url body closed
        labels(first:20){nodes{name}} comments(first:100){nodes{body}}}}}""", o=OWNER, n=NAME, x=num)["repository"]["discussion"]

def on_discussion_labeled():
    if EV["label"]["name"] != "status: approved": return
    d = discussion(EV["discussion"]["number"])
    for c in d["comments"]["nodes"]:
        m = re.match(r"<!-- idea-issue:(\d+) -->", c["body"])
        if m: print("already has issue", m.group(1)); return
    labels = ["type: enhancement", "status: approved"] + [l["name"] for l in d["labels"]["nodes"] if l["name"].startswith(("priority:", "category:"))]
    desc = re.sub(r"<!--.*?-->", "", d["body"], flags=re.S)
    desc = re.split(r"\n\*\*Vote:\*\*", desc)[0].strip()
    body = (f"<!-- idea-discussion:{d['number']} -->\nApproved idea: {d['url']}\n\n{desc}\n\n"
            "---\nVotes and discussion stay in the idea thread; this issue tracks the work (project board).")
    iss = rest("POST", "issues", {"title": d["title"], "body": body, "labels": labels})
    gql("mutation($d:ID!,$b:String!){addDiscussionComment(input:{discussionId:$d,body:$b}){comment{id}}}", d=d["id"],
        b=f"<!-- idea-issue:{iss['number']} -->\n✅ **Approved.** Work is tracked in #{iss['number']} — you will see its progress on the project board."
)
    print("created issue", iss["number"])

def linked_discussion(issue):
    m = re.search(r"<!-- idea-discussion:(\d+) -->", issue.get("body") or "")
    return discussion(int(m.group(1))) if m else None

def on_issue(action):
    d = linked_discussion(EV["issue"])
    if not d: return
    if action == "closed" and EV["issue"].get("state_reason") == "completed":
        set_status(d, "status: done")
        gql("mutation($d:ID!,$b:String!){addDiscussionComment(input:{discussionId:$d,body:$b}){comment{id}}}", d=d["id"],
            b=f"🎉 **Done** in #{EV['issue']['number']} — it will be in the next build. Thanks for the idea!"
)
        if not d["closed"]: gql("mutation($d:ID!){closeDiscussion(input:{discussionId:$d,reason:RESOLVED}){discussion{id}}}", d=d["id"])
    elif action == "closed":
        set_status(d, "status: rejected")
    elif action == "reopened":
        set_status(d, "status: approved")
        if d["closed"]: gql("mutation($d:ID!){reopenDiscussion(input:{discussionId:$d}){discussion{id}}}", d=d["id"])
    elif action == "labeled" and EV["label"]["name"] in ("status: in progress", "status: planned"):
        set_status(d, EV["label"]["name"])
    print("idea", d["number"], "updated for issue", action)

if __name__ == "__main__":
    name = os.environ["GITHUB_EVENT_NAME"]
    if name == "discussion": on_discussion_labeled()
    elif name == "issues": on_issue(EV["action"])

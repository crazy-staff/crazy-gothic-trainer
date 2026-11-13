# Contributing

Thanks for helping with Gothic 1 Crazy Trainer! This page explains **where to post what** and **what happens next**.

## Language

**Everything on GitHub is in English** — issues, ideas, comments, commits. This way everyone can follow and help.
The in-game menu is still in Polish (an English interface is planned); menu names may appear in Polish in reports — that is fine.

## Where to post

| You have… | Post it in | How |
|---|---|---|
| A bug (something does not work) | [Issues → Bug report](https://github.com/dawidseipold/gothic-crazy-trainer/issues/new?template=blad.yml) | best: send it from the in-game **Tester hub** — it fills in everything |
| A crash | [Issues → Crash](https://github.com/dawidseipold/gothic-crazy-trainer/issues/new?template=crash.yml) | attach the report file (see [TESTING.md](TESTING.md)) |
| A change to an existing option | [Issues → Change to an existing option](https://github.com/dawidseipold/gothic-crazy-trainer/issues/new?template=propozycja.yml) | |
| A new feature idea | [Discussions → Ideas](https://github.com/dawidseipold/gothic-crazy-trainer/discussions/new?category=ideas) | one idea per thread |
| A question | [Discussions → Q&A](https://github.com/dawidseipold/gothic-crazy-trainer/discussions/new?category=q-a) | |
| Screenshots, videos, presets | [Discussions → Show and tell](https://github.com/dawidseipold/gothic-crazy-trainer/discussions/new?category=show-and-tell) | |
| Anything else | [Discussions → General](https://github.com/dawidseipold/gothic-crazy-trainer/discussions/new?category=general) | |

Before posting, search first. If it already exists, **vote** (👍 / up arrow) and add your details there instead of opening a new one.

## What happens next

```
 Idea (Discussions)                     Bug / crash (Issues)
 status: pending   ── votes 👍 ──┐       status: triage
        │                        │              │
  author reviews                 │       author confirms, fixes
        │                        │              │
 status: approved ───► issue is created ──► Project board
 (or rejected)        automatically        Todo → In progress → Needs verification → Done
                                                   │
                         issue closed ───► idea gets status: done (closed as resolved)
```

1. **New post** — a bot checks it within a minute: adds labels (type, area / category), points to possible duplicates, and asks for English if needed. Obvious spam is closed automatically (reopened if it was a mistake). The bot only suggests — the author has the final word.
2. **Ideas** start as `status: pending`. Votes help the author decide. Approved ideas (`status: approved`) automatically get a linked **issue**, which goes on the **project board**. Rejected ideas get `status: rejected`.
3. **Issues** move across the project board. When a fix is in a build, the issue gets `status: needs verification` — testers confirm it in game, then it is closed.
4. When the issue of an idea is closed, the idea thread is marked `status: done` and closed.

The [roadmap](ROADMAP.md) shows everything in one place (refreshed automatically).

## Labels

Every label is `kind: value`:

| Kind | Values | Used on |
|---|---|---|
| `type:` | bug, crash, enhancement, idea, epic, docs, question | issues |
| `status:` | triage, pending, approved, planned, in progress, needs verification, done, rejected | issues and ideas |
| `priority:` | high, medium, low | issues and ideas |
| `area:` | the part of the trainer (party, esp, noclip, ui…) | issues |
| `category:` | topic of an idea (Teleport, World, Gameplay…) | ideas |
| `source:` | in-game (sent from the Tester hub) | issues |
| `resolution:` | duplicate, invalid, wontfix | closed issues |
| `moderation:` | spam | closed automatically |
| `option:` / `build:` | the trainer option ID / build number from the in-game form | issues |

## Using open-source code

Many ideas build on open-source Union plugins and mods. Ports are welcome **with credit**: name the author and link the source in the idea / issue and in the code.

## Commits

Commit messages are in English, short summary line first.

#!/usr/bin/env node
// Sync docs/pages to a Quackback docs (help center) over its REST API.
// QB_URL=https://feedback.example.com QB_KEY=qb_... [QB_STATE=.docs-state.json] node tools/docs/sync.mjs
import fs from 'node:fs'
import { execFileSync } from 'node:child_process'
import { createRequire } from 'node:module'
const { gothicDocsSync } = createRequire(import.meta.url)('./sync-core.js')

const base = process.env.QB_URL?.replace(/\/$/, ''), key = process.env.QB_KEY
if (!base || !key) throw new Error('set QB_URL and QB_KEY')
const stateFile = process.env.QB_STATE || '.docs-state.json'
const payload = JSON.parse(execFileSync(process.execPath, [new URL('./build-payload.mjs', import.meta.url).pathname]).toString())
const api = async (method, p, body) => {
  for (let k = 0; k < 8; k++) {
    const r = await fetch(`${base}/api/v1/${p}`, { method, headers: { Authorization: `Bearer ${key}`, 'Content-Type': 'application/json' }, body: body ? JSON.stringify(body) : undefined })
    if (r.status === 429) { await new Promise((s) => setTimeout(s, 4000)); continue }
    const t = await r.text()
    if (!r.ok) throw new Error(`${method} ${p} ${r.status} ${t.slice(0, 300)}`)
    return t ? JSON.parse(t) : {}
  }
  throw new Error('rate limited ' + p)
}
const members = (await api('GET', 'members')).data || []
const author = (members.find((m) => m.role === 'admin') || members[0])?.id
const state = fs.existsSync(stateFile) ? JSON.parse(fs.readFileSync(stateFile, 'utf8')) : {}
const res = await gothicDocsSync({ api, payload, state, author })
fs.writeFileSync(stateFile, JSON.stringify(res.state, null, 1))

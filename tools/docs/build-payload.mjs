#!/usr/bin/env node
// docs/pages -> docs payload (JSON) for sync-core.js. Prints to stdout or writes argv[2].
import fs from 'node:fs'
import path from 'node:path'
import crypto from 'node:crypto'

const root = path.resolve(path.dirname(new URL(import.meta.url).pathname), '..', '..', 'docs', 'pages')
const categories = JSON.parse(fs.readFileSync(path.join(root, 'categories.json'), 'utf8'))
const articles = []
const seen = new Set()
for (const c of categories) {
  for (const f of fs.readdirSync(path.join(root, c.dir)).filter((f) => f.endsWith('.md'))) {
    const text = fs.readFileSync(path.join(root, c.dir, f), 'utf8')
    const m = text.match(/^---\n([\s\S]*?)\n---\n?/)
    const meta = {}
    if (m) for (const line of m[1].split('\n')) {
      const i = line.indexOf(':')
      if (i > 0) meta[line.slice(0, i).trim()] = line.slice(i + 1).trim()
    }
    let content = (m ? text.slice(m[0].length) : text).trim()
    if (meta.menu) content = `> **Note:** In the menu: **${meta.menu}** (the in-game menu is in Polish for now).\n\n` + content
    const slug = f.replace(/\.md$/, '')
    if (seen.has(slug)) throw new Error(`duplicate page slug ${slug}`)
    seen.add(slug)
    articles.push({
      slug, category: c.dir, title: meta.title || slug, description: meta.description || '', order: Number(meta.order ?? 0),
      content, hash: crypto.createHash('sha1').update(text).digest('hex').slice(0, 12),
    })
  }
}
for (const a of articles) for (const [, s] of a.content.matchAll(/doc:([a-z0-9-]+)/g)) if (!seen.has(s)) throw new Error(`${a.slug}: link to unknown page doc:${s}`)
const changelogCategories = JSON.parse(fs.readFileSync(path.join(root, '..', 'changelog-categories.json'), 'utf8'))
const out = JSON.stringify({ categories, articles, changelogCategories })
if (process.argv[2]) fs.writeFileSync(process.argv[2], out)
else process.stdout.write(out)
console.error(`docs payload: ${categories.length} categories, ${articles.length} pages`)

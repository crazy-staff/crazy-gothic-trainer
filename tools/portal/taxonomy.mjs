// Sets up the portal's tags and roadmaps (idempotent: run it again any time).
//   QB_URL=http://localhost:3000 QB_KEY=qb_... node tools/portal/taxonomy.mjs
//
// - tags `game: <title>` and `renderer: <title>` for every game and renderer the trainer targets
//   (the same list as the docs pickers: portal/patch/apps/web/src/custom-docs/variants.ts)
// - tags `product: <area>` and one roadmap per area (trainer, docs, Discord, website, testers),
//   each showing the posts with its tag, with the columns of the main roadmap
// - posts on the Ideas board that have no product tag get `product: Trainer` and `game: Gothic 1`

export const GAMES = ['Gothic 1', 'Gothic 2', 'Chronicles of Myrtana']
export const RENDERERS = ['GD3D11', 'Original']
export const PRODUCTS = [
  { name: 'Trainer', slug: 'trainer', description: 'The in-game trainer: features, fixes and testing builds.', color: '#8b5cf6' },
  { name: 'Docs', slug: 'docs', description: 'The documentation on this site.', color: '#3b82f6' },
  { name: 'Discord', slug: 'discord', description: 'The Discord server and its bot.', color: '#5865f2' },
  { name: 'Website', slug: 'website', description: 'This portal: feedback, roadmap, changelog, accounts.', color: '#10b981' },
  { name: 'Testers', slug: 'testers', description: 'The tester program: reports, ranks and test builds.', color: '#f59e0b' },
]

export async function setup(base, key, log = console.log) {
  const api = async (method, path, body) => {
    const res = await fetch(`${base}/api/v1${path}`, {
      method,
      headers: { authorization: `Bearer ${key}`, 'content-type': 'application/json' },
      body: body ? JSON.stringify(body) : undefined,
    })
    const text = await res.text()
    if (!res.ok) throw new Error(`${method} ${path}: ${res.status} ${text.slice(0, 300)}`)
    return text ? JSON.parse(text).data ?? JSON.parse(text) : null
  }

  // tags
  const tags = await api('GET', '/tags')
  const tag = async (name, color, description) => {
    const found = tags.find((t) => t.name.toLowerCase() === name.toLowerCase())
    if (found) return found
    const made = await api('POST', '/tags', { name, color, description })
    tags.push(made)
    log(`tag + ${name}`)
    return made
  }
  for (const g of GAMES) await tag(`game: ${g}`, '#a78bfa', `Concerns ${g}.`)
  for (const r of RENDERERS) await tag(`renderer: ${r}`, '#64748b', `Concerns the ${r} renderer.`)
  const productTags = {}
  for (const p of PRODUCTS) productTags[p.slug] = await tag(`product: ${p.name}`, p.color, p.description)

  // roadmaps: one per product, columns copied from the main roadmap
  const roadmaps = await api('GET', '/roadmaps')
  const main = roadmaps.find((r) => !PRODUCTS.some((p) => p.slug === r.slug)) ?? roadmaps[0]
  const columns = (main?.columns ?? []).map((c, i) => ({
    statusId: c.statusId,
    name: c.name,
    icon: c.icon ?? null,
    color: c.color,
    position: c.position ?? i,
  }))
  for (const p of PRODUCTS) {
    if (roadmaps.some((r) => r.slug === p.slug)) continue
    await api('POST', '/roadmaps', {
      name: p.name,
      slug: p.slug,
      description: p.description,
      type: 'column',
      visibility: 'public',
      baseFilter: { tagIds: [productTags[p.slug].id] },
      columns,
    })
    log(`roadmap + ${p.name}`)
  }

  // existing ideas are trainer ideas for Gothic 1
  const boards = await api('GET', '/boards')
  const ideas = boards.find((b) => /idea/i.test(b.slug) || /idea/i.test(b.name))
  if (ideas) {
    const productIds = new Set(Object.values(productTags).map((t) => t.id))
    const extra = [productTags.trainer.id, tags.find((t) => t.name === 'game: Gothic 1').id]
    let cursor
    let tagged = 0
    do {
      const res = await fetch(`${base}/api/v1/posts?boardId=${ideas.id}&limit=100${cursor ? `&cursor=${cursor}` : ''}`, {
        headers: { authorization: `Bearer ${key}` },
      }).then((r) => r.json())
      for (const post of res.data ?? []) {
        const ids = (post.tags ?? []).map((t) => t.id)
        if (ids.some((id) => productIds.has(id))) continue
        await api('PATCH', `/posts/${post.id}`, { tagIds: [...new Set([...ids, ...extra])] })
        tagged++
      }
      cursor = res.meta?.pagination?.cursor ?? res.meta?.cursor ?? null
      if (!res.meta?.pagination?.hasMore && !res.meta?.hasMore) cursor = null
    } while (cursor)
    log(`ideas tagged: ${tagged}`)
  }
}

if (typeof process !== 'undefined' && process.argv[1]?.endsWith('taxonomy.mjs')) {
  const { QB_URL, QB_KEY } = process.env
  if (!QB_URL || !QB_KEY) throw new Error('QB_URL and QB_KEY are required')
  await setup(QB_URL.replace(/\/$/, ''), QB_KEY)
}

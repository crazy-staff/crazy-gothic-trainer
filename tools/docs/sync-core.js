// Docs -> Quackback docs (help center) sync. No Node APIs: runs in Node (sync.mjs) and in a browser.
// payload = { categories: [{dir,name,description,icon}], articles: [{slug,category,title,description,order,content,hash}] }
// state   = { [slug]: hash } from the previous run (to know what changed for the changelog)
// Links "doc:<slug>" in content become /hc/<category>/<slug> (Quackback redirects that to the canonical URL).
async function gothicDocsSync({ api, payload, state = {}, author, changelog = true, log = console.log }) {
  const bySlug = (list) => Object.fromEntries(list.map((x) => [x.slug, x]))
  // changelog categories (Trainer, Docs, Discord, ...): created or recoloured, never removed
  if (payload.changelogCategories?.length)
    await api('POST', 'changelog/categories', { categories: payload.changelogCategories.map(({ name, color }) => ({ name, color })) })
  const cats = bySlug((await api('GET', 'help-center/categories')).data || [])
  const catIds = {}
  for (const [i, c] of payload.categories.entries()) {
    const body = { name: c.name, slug: c.dir, description: c.description, icon: c.icon, isPublic: true, position: i }
    const ex = cats[c.dir]
    const r = ex ? await api('PATCH', `help-center/categories/${ex.id}`, body) : await api('POST', 'help-center/categories', body)
    catIds[c.dir] = (r.data || r).id
  }
  const catOf = Object.fromEntries(payload.articles.map((a) => [a.slug, a.category]))
  const resolve = (md) =>
    md.replace(/doc:([a-z0-9-]+)(#[\w-]+)?/g, (m, slug, hash) => (catOf[slug] ? `/hc/${catOf[slug]}/${slug}${hash || ''}` : m))
  const existing = bySlug((await api('GET', 'help-center/articles?limit=500&status=all')).data || [])
  const created = [], updated = [], now = new Date().toISOString(), next = {}
  for (const a of payload.articles) {
    const body = {
      categoryId: catIds[a.category], title: a.title, description: a.description || undefined,
      content: resolve(a.content), slug: a.slug, position: a.order ?? null, authorId: author,
    }
    const ex = existing[a.slug]
    if (!ex) {
      const r = await api('POST', 'help-center/articles', body)
      await api('PATCH', `help-center/articles/${(r.data || r).id}`, { publishedAt: now })
      created.push(a)
    } else if (state[a.slug] !== a.hash || ex.title !== a.title || !ex.publishedAt) {
      await api('PATCH', `help-center/articles/${ex.id}`, { ...body, authorId: undefined, publishedAt: ex.publishedAt || now })
      if (state[a.slug] && state[a.slug] !== a.hash) updated.push(a)
    }
    next[a.slug] = a.hash
  }
  log(`docs: ${created.length} new, ${updated.length} updated, ${payload.articles.length} total`)
  if (changelog && (created.length || updated.length)) {
    const line = (a) => `- [${a.title}](/hc/${a.category}/${a.slug}) — ${a.description || ''}`
    const parts = []
    if (created.length) parts.push(`## New pages\n\n${created.map(line).join('\n')}`)
    if (updated.length) parts.push(`## Updated pages\n\n${updated.map(line).join('\n')}`)
    const title = created.length && !updated.length && created.length > 10
      ? 'Documentation is live'
      : `Docs: ${[created.length && `${created.length} new`, updated.length && `${updated.length} updated`].filter(Boolean).join(', ')} page${created.length + updated.length > 1 ? 's' : ''}`
    await api('POST', 'changelog', { title, content: parts.join('\n\n'), publishedAt: now, categoryNames: ['Docs'] })
    log('changelog entry: ' + title)
  }
  return { created: created.length, updated: updated.length, state: next }
}
if (typeof module !== 'undefined') module.exports = { gothicDocsSync }

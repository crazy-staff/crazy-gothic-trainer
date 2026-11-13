/**
 * Changelog categories by name for the API (creates missing ones), so automation can file
 * an entry under "Docs", "Trainer", ... without knowing ids.
 */
import type { ChangelogCategoryId } from '@quackback/ids'

const COLORS: Record<string, string> = {
  trainer: '#a88cff',
  docs: '#6bb3ff',
  portal: '#73eb8c',
  discord: '#5865f2',
  'tester program': '#fbcc4d',
  'bug fixes': '#f87171',
}

export async function changelogCategoryIdsByName(names?: string[]): Promise<ChangelogCategoryId[] | undefined> {
  if (!names || names.length === 0) return undefined
  const { listChangelogCategories, createChangelogCategory } = await import(
    '@/lib/server/domains/changelog/changelog-category.service'
  )
  const existing = await listChangelogCategories()
  const ids: ChangelogCategoryId[] = []
  for (const raw of names) {
    const name = raw.trim()
    if (!name) continue
    const found = existing.find((c) => c.name.toLowerCase() === name.toLowerCase())
    const cat = found ?? (await createChangelogCategory({ name, color: COLORS[name.toLowerCase()] }))
    if (!found) existing.push(cat)
    ids.push(cat.id as ChangelogCategoryId)
  }
  return ids
}

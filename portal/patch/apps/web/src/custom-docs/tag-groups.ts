import type { PostTag } from '@/lib/shared/db-types'

/** "area: esp" -> group "Area", value "esp"; tags without a prefix go to "Tags". */
export function groupTags(tags: PostTag[]) {
  const groups = new Map<string, { tag: PostTag; label: string }[]>()
  for (const tag of tags) {
    const i = tag.name.indexOf(':')
    const group = i > 0 ? tag.name.slice(0, i).trim() : 'tags'
    const label = i > 0 ? tag.name.slice(i + 1).trim() : tag.name
    const name = group.charAt(0).toUpperCase() + group.slice(1)
    if (!groups.has(name)) groups.set(name, [])
    groups.get(name)!.push({ tag, label })
  }
  return [...groups.entries()]
}


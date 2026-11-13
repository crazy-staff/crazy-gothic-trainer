/**
 * The games and renderers the trainer targets. One list for the docs pickers, the search
 * filters (tags `game:<id>` and `renderer:<id>`), the "coming later" pages and the post tags
 * (`game: <title>`, `renderer: <title>`). Adding a story mod or another game is one entry here.
 */
export type VariantStatus = 'available' | 'planned' | 'later'

export interface Variant {
  id: string
  title: string
  /** Two letters for the badge. */
  short: string
  description: string
  status: VariantStatus
  /** Extra labels shown next to the title (e.g. "Story mod"). */
  tags?: string[]
  /** Text for the "coming later" page. */
  soon?: string
}

export const GAMES: Variant[] = [
  { id: 'gothic-1', title: 'Gothic 1', short: 'G1', description: 'Steam · GothicMod 1.08k', status: 'available' },
  {
    id: 'gothic-2',
    title: 'Gothic 2',
    short: 'G2',
    description: 'Night of the Raven',
    status: 'planned',
    soon: 'The trainer for Gothic 2: Night of the Raven is planned. Its docs will live here.',
  },
  {
    id: 'chronicles-of-myrtana',
    title: 'Chronicles of Myrtana',
    short: 'CM',
    description: 'Story mod for Gothic 2',
    status: 'later',
    tags: ['Story mod'],
    soon: 'Support for story mods such as Chronicles of Myrtana comes after Gothic 2. Its docs will live here.',
  },
]

export const RENDERERS: Variant[] = [
  { id: 'gd3d11', title: 'GD3D11', short: 'DX', description: 'DirectX 11 renderer', status: 'available' },
  {
    id: 'original',
    title: 'Original',
    short: 'D7',
    description: 'The game’s own DirectX 7 renderer',
    status: 'planned',
    soon: 'Notes for playing without GD3D11 (the game’s own DirectX 7 renderer) will live here.',
  },
]

export const STATUS_LABEL: Record<VariantStatus, string> = { available: 'Available', planned: 'Planned', later: 'Later' }

/** The docs URL of a variant: the docs home for what is available, a placeholder otherwise. */
export const variantUrl = (kind: 'game' | 'renderer', v: Variant) =>
  v.status === 'available' ? '/hc' : `/hc/soon/${kind === 'renderer' ? `${v.id}-renderer` : v.id}`

/** Search tag of a variant (docs index and search filters). */
export const variantTag = (kind: 'game' | 'renderer', v: Variant) => `${kind}:${v.id}`

/** The variant a "coming later" page belongs to. */
export function soonVariant(what: string): Variant | undefined {
  return GAMES.find((g) => g.id === what) ?? RENDERERS.find((r) => `${r.id}-renderer` === what)
}

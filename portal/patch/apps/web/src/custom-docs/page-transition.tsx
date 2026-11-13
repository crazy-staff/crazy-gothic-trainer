/**
 * Page transitions (framer-motion). A new page comes in from the direction of its link in the
 * sidebar: a section lower in the menu (or a docs page further down the tree) rises from
 * below, one higher drops in from above; a page deeper in the same section rises too.
 */
import { useEffect, type ReactNode } from 'react'
import { motion, useReducedMotion } from 'framer-motion'

export const EASE = [0.22, 1, 0.36, 1] as const

/** The first page of a visit is rendered by the server: shown as it is, not animated in. */
let booted = false
/** The page shown before this one (kept outside React: pages remount on navigation). */
let last: { id: string; position: number } | null = null
const dirs = new Map<string, number>()

export function PageTransition({
  id,
  position,
  children,
  className,
}: {
  /** Changes when the page changes. */
  id: string
  /** Where the page sits in the sidebar, top to bottom (any increasing number). */
  position: number
  children: ReactNode
  className?: string
}) {
  const reduce = useReducedMotion()
  if (last?.id !== id) {
    dirs.set(id, !last || last.position === position ? 0 : position > last.position ? 1 : -1)
    last = { id, position }
  }
  const dir = dirs.get(id) ?? 0
  useEffect(() => {
    booted = true
  }, [])
  return (
    <motion.div
      key={id}
      className={className}
      initial={booted && !reduce ? { opacity: 0, y: dir === 0 ? 16 : dir * 56, scale: 0.985, filter: 'blur(6px)' } : false}
      animate={{ opacity: 1, y: 0, scale: 1, filter: 'blur(0px)' }}
      transition={{ duration: 0.55, ease: EASE }}
    >
      {children}
    </motion.div>
  )
}

/** Children rise in one after another (lists, card grids). */
export function Stagger({ children, className }: { children: ReactNode[]; className?: string }) {
  const reduce = useReducedMotion()
  return (
    <div className={className}>
      {children.map((child, i) => (
        <motion.div
          key={i}
          initial={reduce ? false : { opacity: 0, y: 14 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true, margin: '0px 0px -5% 0px' }}
          transition={{ duration: 0.45, ease: EASE, delay: Math.min(i, 8) * 0.04 }}
        >
          {child}
        </motion.div>
      ))}
    </div>
  )
}

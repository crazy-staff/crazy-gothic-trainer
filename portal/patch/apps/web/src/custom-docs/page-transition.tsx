/**
 * Page transitions (framer-motion): a new page slides in from the side its link sits on.
 * Moving to a section further right in the top bar (or deeper into a section) slides from the
 * right, back towards the left slides from the left; within one page it only fades.
 */
import { useEffect, type ReactNode } from 'react'
import { motion, useReducedMotion } from 'framer-motion'

export const EASE = [0.22, 1, 0.36, 1] as const

/** The first page of a visit is rendered by the server: shown as it is, not animated in. */
let booted = false
/** The page shown before this one (kept outside React: pages remount on navigation). */
let last: { id: string; position: [number, number] } | null = null
const dirs = new Map<string, number>()

/** Direction for a move from `prev` to `next` ([section, depth or order]). */
function direction(prev: [number, number] | null, next: [number, number]): number {
  if (!prev) return 0
  if (prev[0] !== next[0]) return next[0] > prev[0] ? 1 : -1
  if (prev[1] !== next[1]) return next[1] > prev[1] ? 1 : -1
  return 0
}

export function PageTransition({
  id,
  position,
  children,
  className,
}: {
  /** Changes when the page changes. */
  id: string
  /** [section index, depth or order in the section] of this page. */
  position: [number, number]
  children: ReactNode
  className?: string
}) {
  const reduce = useReducedMotion()
  if (last?.id !== id) {
    dirs.set(id, direction(last?.position ?? null, position))
    last = { id, position }
  }
  const dir = { current: dirs.get(id) ?? 0 }
  useEffect(() => {
    booted = true
  }, [])
  const animateIn = booted && !reduce
  return (
    <motion.div
      key={id}
      className={className}
      initial={animateIn ? { opacity: 0, x: dir.current * 24, y: dir.current === 0 ? 6 : 0 } : false}
      animate={{ opacity: 1, x: 0, y: 0 }}
      transition={{ duration: 0.3, ease: EASE }}
    >
      {children}
    </motion.div>
  )
}

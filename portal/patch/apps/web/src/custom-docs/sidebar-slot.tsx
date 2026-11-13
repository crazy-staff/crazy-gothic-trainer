/**
 * A page puts its own panel (filters, a section menu) into the shell's sidebar:
 *   <SidebarSlot fallback={<ThePageColumn />}>{panel}</SidebarSlot>
 * While the desktop sidebar is on screen the panel is rendered there (a React portal, so it
 * keeps its state and handlers); otherwise (phones, the first server render) the page shows
 * `fallback` in its own layout, as Quackback had it.
 */
import { useEffect, useState, type ReactNode } from 'react'
import { createPortal } from 'react-dom'
import { motion } from 'framer-motion'

export const SIDEBAR_SLOT_ID = 'app-sidebar-slot'

export function SidebarSlot({ children, fallback = null }: { children: ReactNode; fallback?: ReactNode }) {
  const [target, setTarget] = useState<HTMLElement | null>(null)
  useEffect(() => {
    const find = () => {
      const el = document.getElementById(SIDEBAR_SLOT_ID)
      setTarget(el && el.parentElement && el.parentElement.offsetParent !== null ? el : null)
    }
    find()
    // the sidebar mounts, collapses and switches to the drawer with the window size
    const observer = new MutationObserver(find)
    observer.observe(document.body, { childList: true, subtree: true })
    window.addEventListener('resize', find)
    return () => {
      observer.disconnect()
      window.removeEventListener('resize', find)
    }
  }, [])
  if (!target) return <>{fallback}</>
  return createPortal(
    <motion.div
      className="app-sidebar-panel"
      initial={{ opacity: 0, y: 6 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.3, ease: [0.22, 1, 0.36, 1] }}
    >
      {children}
    </motion.div>,
    target
  )
}

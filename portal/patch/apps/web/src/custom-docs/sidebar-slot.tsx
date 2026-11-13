/**
 * A page puts its own panel (filters, a section menu) into the app sidebar:
 *   <SidebarSlot fallback={<ThePageColumn />}>{panel}</SidebarSlot>
 * While the desktop sidebar is on screen the panel is rendered there (a React portal, so it
 * keeps its state and handlers); otherwise (phones, the server render) the page shows
 * `fallback` in its own layout.
 */
import { useEffect, useState, type ReactNode } from 'react'
import { createPortal } from 'react-dom'
import { motion } from 'framer-motion'
import { EASE } from './page-transition'
import { SIDEBAR_SLOT_ID } from './sidebar'

export function SidebarSlot({ children, fallback = null }: { children: ReactNode; fallback?: ReactNode }) {
  const [target, setTarget] = useState<HTMLElement | null>(null)
  useEffect(() => {
    const find = () => {
      const el = document.querySelector<HTMLElement>(`#nd-sidebar #${SIDEBAR_SLOT_ID}`)
      const shown = el && document.getElementById('nd-sidebar')?.dataset.collapsed !== 'true'
      setTarget((cur) => (shown ? el : null) ?? (cur && shown ? cur : null))
    }
    find()
    const sidebar = document.getElementById('nd-sidebar')
    const observer = new MutationObserver(find)
    if (sidebar) observer.observe(sidebar, { attributes: true, attributeFilter: ['data-collapsed'] })
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
      initial={{ opacity: 0, y: 12 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.4, ease: EASE }}
    >
      {children}
    </motion.div>,
    target
  )
}

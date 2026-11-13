/** The portal settings menu as sidebar rows (it moves into the app sidebar on desktop). */
import { Link, useRouterState } from '@tanstack/react-router'
import { UserIcon, Cog6ToothIcon, KeyIcon } from '@heroicons/react/20/solid'
import { PageTransition } from './page-transition'
import { sidebarItem, sidebarLabel } from './sidebar'

export function SettingsRows({ showApiKeys }: { showApiKeys: boolean }) {
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const items = [
    { to: '/settings/profile', label: 'Profile', icon: <UserIcon /> },
    { to: '/settings/preferences', label: 'Preferences', icon: <Cog6ToothIcon /> },
    ...(showApiKeys ? [{ to: '/settings/api-keys', label: 'API keys', icon: <KeyIcon /> }] : []),
  ]
  return (
    <nav className="flex flex-col">
      <p className={sidebarLabel}>Personal</p>
      {items.map((i) => {
        const active = pathname.startsWith(i.to)
        return (
          <Link key={i.to} to={i.to as '/'} data-active={active} className={sidebarItem(active)}>
            {i.icon}
            {i.label}
          </Link>
        )
      })}
    </nav>
  )
}

const ORDER = ['/settings/profile', '/settings/preferences', '/settings/api-keys']

/** Only the settings page itself moves when switching between settings pages. */
export function SettingsOutlet({ children }: { children: React.ReactNode }) {
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  return (
    <PageTransition id={pathname} position={Math.max(0, ORDER.findIndex((p) => pathname.startsWith(p)))}>
      {children}
    </PageTransition>
  )
}

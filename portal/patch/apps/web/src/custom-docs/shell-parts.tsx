/**
 * Pieces shared by every page's frame (docs and the rest of the portal), so the top bar and
 * the sidebar header are the same component everywhere.
 */
import type { ComponentProps } from 'react'
import { Link as RouterLink, useRouter, useRouterState } from '@tanstack/react-router'
import { useIntl } from 'react-intl'
import { useQueryClient } from '@tanstack/react-query'
import { Menu } from '@base-ui/react/menu'
import { HeaderActions } from '@fumadocs/base-ui/layouts/spacious/slots/actions'
import {
  ArrowRightStartOnRectangleIcon,
  Cog6ToothIcon,
  ShieldCheckIcon,
  SwatchIcon,
} from '@heroicons/react/20/solid'
import { usePortalNavItems } from '@/components/public/use-portal-nav-items'
import type { PortalNavItem } from '@/components/public/portal-header-nav'
import { useSessionContext, useUserRole, useWorkspaceSettings } from '@/lib/client/hooks/use-root-context'
import { useAuthPopoverSafe } from '@/components/auth/auth-popover-context'
import { Avatar } from '@/components/ui/avatar'
import { NotificationBell } from '@/components/notifications'
import { signOut } from '@/lib/client/auth-client'
import { removeViewerScopedPortalQueries } from '@/lib/client/queries/portal'

/** The workspace logo and name, at the top of the sidebar. */
export function NavTitle() {
  const settings = useWorkspaceSettings() as { name?: string; brandingData?: { logoUrl?: string | null } } | undefined
  const name = settings?.name ?? 'Portal'
  const logo = settings?.brandingData?.logoUrl
  return (
    <span className="inline-flex min-w-0 items-center gap-2">
      {logo ? (
        <img src={logo} alt="" className="size-6 shrink-0 rounded-md" />
      ) : (
        <span className="flex size-6 shrink-0 items-center justify-center rounded-md bg-fd-primary text-xs font-bold text-fd-primary-foreground">
          {name.slice(0, 1).toUpperCase()}
        </span>
      )}
      <span className="truncate">{name}</span>
    </span>
  )
}

export function navItemLabel(item: PortalNavItem, format: (d: { id: string; defaultMessage: string }) => string) {
  return item.kind === 'builtin' ? (item.label ?? format({ id: item.messageId, defaultMessage: item.defaultMessage })) : item.label
}

/** Which top-level section a path belongs to: the index in the nav order (for page transitions). */
export function useSectionIndex(pathname: string): number {
  const items = usePortalNavItems()
  const i = items.findIndex(
    (item) =>
      item.kind === 'builtin' &&
      (item.type === 'help'
        ? pathname.startsWith('/hc')
        : item.to === '/'
          ? pathname === '/' || pathname.startsWith('/b/') || pathname.startsWith('/posts')
          : pathname.startsWith(item.to))
  )
  return i === -1 ? items.length : i
}

const menuPopup =
  'flex flex-col min-w-48 p-1 rounded-2xl border bg-fd-popover text-sm text-fd-popover-foreground shadow-lg outline-none origin-(--transform-origin) transition-[opacity,scale] duration-150 ease-[cubic-bezier(0.22,1,0.36,1)] data-starting-style:opacity-0 data-starting-style:scale-95 data-ending-style:opacity-0 data-ending-style:scale-97 data-ending-style:duration-100'
const menuItem =
  'flex w-full min-h-9 items-center gap-3 rounded-lg px-2.5 py-1.5 text-start outline-none transition-colors duration-100 data-highlighted:bg-fd-accent data-highlighted:text-fd-accent-foreground [&_svg]:size-4 [&_svg]:shrink-0'

/** Log in, or the signed-in user with a menu (settings, admin, sign out). */
function Account() {
  const session = useSessionContext() as {
    user?: { name?: string | null; email?: string | null; image?: string | null; principalType?: string }
  } | null
  const role = useUserRole() as string | null | undefined
  const team = role === 'admin' || role === 'member' || role === 'owner'
  const auth = useAuthPopoverSafe()
  const router = useRouter()
  const queryClient = useQueryClient()
  const user = session?.user && session.user.principalType !== 'anonymous' ? session.user : null
  if (!user) {
    if (!auth?.openAuthPopover) return null
    return (
      <button
        type="button"
        onClick={() => auth.openAuthPopover({ mode: 'login' })}
        className="ms-1 h-7.5 rounded-lg bg-fd-primary px-3 text-sm font-medium text-fd-primary-foreground transition-[opacity,scale] hover:opacity-90 active:scale-[0.97]"
      >
        Log in
      </button>
    )
  }
  return (
    <Menu.Root>
      <Menu.Trigger className="ms-1 rounded-full outline-none transition-transform hover:scale-105 focus-visible:ring-2 focus-visible:ring-fd-ring">
        <Avatar className="size-7" src={user.image ?? undefined} name={user.name ?? undefined} />
      </Menu.Trigger>
      <Menu.Portal>
        <Menu.Positioner align="end" sideOffset={8} className="z-50">
          <Menu.Popup className={menuPopup}>
            <div className="px-2.5 py-2">
              <p className="truncate font-medium">{user.name}</p>
              {user.email && <p className="truncate text-xs text-fd-muted-foreground">{user.email}</p>}
            </div>
            <Menu.LinkItem render={<RouterLink to="/settings" />} className={menuItem}>
              <Cog6ToothIcon />
              Settings
            </Menu.LinkItem>
            {team && (
              <Menu.LinkItem render={<a href="/admin" />} className={menuItem}>
                <ShieldCheckIcon />
                Admin
              </Menu.LinkItem>
            )}
            {team && (
              <Menu.LinkItem render={<RouterLink to={'/design-system' as '/'} />} className={menuItem}>
                <SwatchIcon />
                Design system
              </Menu.LinkItem>
            )}
            <Menu.Item
              className={menuItem}
              onClick={async () => {
                await signOut()
                queryClient.invalidateQueries({ queryKey: ['votedPosts'] })
                removeViewerScopedPortalQueries(queryClient)
                router.invalidate()
                router.navigate({ to: '/' })
              }}
            >
              <ArrowRightStartOnRectangleIcon />
              Sign out
            </Menu.Item>
          </Menu.Popup>
        </Menu.Positioner>
      </Menu.Portal>
    </Menu.Root>
  )
}

/** The top bar of every page: the portal sections, Admin, notifications and the account. */
export function PortalBarActions({ className, ...props }: ComponentProps<'div'>) {
  const intl = useIntl()
  const items = usePortalNavItems()
  const pathname = useRouterState({ select: (s) => s.location.pathname })
  const current = useSectionIndex(pathname)
  const role = useUserRole() as string | null | undefined
  const session = useSessionContext() as { user?: { principalType?: string } } | null
  const signedIn = !!session?.user && session.user.principalType !== 'anonymous'
  const team = role === 'admin' || role === 'member' || role === 'owner'
  return (
    <div className={'flex min-w-0 items-center gap-1 ' + (className ?? '')} {...props}>
      <nav className="relative flex min-w-0 items-center gap-0.5 overflow-x-auto rounded-lg bg-fd-background max-lg:hidden">
        {items.map((item, i) => {
          const label = navItemLabel(item, intl.formatMessage)
          const cls =
            'relative rounded-lg px-2.5 py-1 text-sm whitespace-nowrap transition-colors duration-200 ' +
            (i === current
              ? 'bg-fd-primary/10 font-medium text-fd-primary'
              : 'text-fd-muted-foreground hover:bg-fd-accent hover:text-fd-accent-foreground')
          return item.kind === 'link' ? (
            <a key={item.id} href={item.href} target={item.newTab ? '_blank' : undefined} rel="noopener noreferrer" className={cls}>
              {label}
            </a>
          ) : (
            <RouterLink key={item.id} to={item.to as '/'} className={cls}>
              {label}
            </RouterLink>
          )
        })}
      </nav>
      {team && (
        <a
          href="/admin"
          className="inline-flex h-7.5 items-center gap-1.5 rounded-lg border px-2.5 text-sm text-fd-muted-foreground transition-colors hover:bg-fd-accent hover:text-fd-accent-foreground"
        >
          <ShieldCheckIcon className="size-3.5" />
          Admin
        </a>
      )}
      {signedIn && (
        <div className="app-bell">
          <NotificationBell popoverSide="bottom" />
        </div>
      )}
      <Account />
      <HeaderActions />
    </div>
  )
}

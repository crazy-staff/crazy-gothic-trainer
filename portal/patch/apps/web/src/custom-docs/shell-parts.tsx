/**
 * Pieces shared by every page's frame (docs and the rest of the portal), so the top bar and
 * the sidebar header are the same component everywhere.
 */
import type { ComponentProps } from 'react'
import { Link as RouterLink, useRouter } from '@tanstack/react-router'
import { useQueryClient } from '@tanstack/react-query'
import { Menu } from '@base-ui/react/menu'
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
/** The portal sections in sidebar order: the docs last, their tree opens under them. */
export function useSections() {
  const items = usePortalNavItems()
  return [...items.filter((i) => !(i.kind === 'builtin' && i.type === 'help')), ...items.filter((i) => i.kind === 'builtin' && i.type === 'help')]
}

export function useSectionIndex(pathname: string): number {
  const items = useSections()
  const i = items.findIndex(
    (item) =>
      item.kind === 'builtin' &&
      (item.type === 'help'
        ? pathname.startsWith('/hc')
        : item.to === '/'
          ? pathname === '/' || pathname.startsWith('/b/') || pathname.startsWith('/posts')
          : pathname.startsWith(item.to))
  )
  return i === -1 ? items.length + 1 : i
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
        className="h-8 rounded-lg bg-fd-primary px-3 text-sm font-medium text-fd-primary-foreground transition-[opacity,scale] hover:opacity-90 active:scale-[0.97]"
      >
        Log in
      </button>
    )
  }
  return (
    <Menu.Root>
      <Menu.Trigger className="rounded-full outline-none transition-transform hover:scale-105 focus-visible:ring-2 focus-visible:ring-fd-ring">
        <Avatar className="size-8" src={user.image ?? undefined} name={user.name ?? undefined} />
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
            <Menu.Separator className="my-1 h-px bg-fd-border" />
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

/** The top bar of every page: notifications and the account (sections live in the sidebar). */
export function BarActions({ className, ...props }: ComponentProps<'div'>) {
  const session = useSessionContext() as { user?: { principalType?: string } } | null
  const signedIn = !!session?.user && session.user.principalType !== 'anonymous'
  return (
    <div className={'flex items-center gap-1.5 ' + (className ?? '')} {...props}>
      {signedIn && (
        <div className="app-bell">
          <NotificationBell popoverSide="bottom" />
        </div>
      )}
      <Account />
    </div>
  )
}

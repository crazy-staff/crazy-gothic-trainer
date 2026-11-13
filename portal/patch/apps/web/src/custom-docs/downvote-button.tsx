/** The down arrow under a post's vote button (downvote-fns.ts). Signed-in users only. */
import { useEffect } from 'react'
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { ChevronDownIcon } from '@heroicons/react/24/solid'
import { voteCountKeys } from '@/lib/client/hooks/use-post-vote'
import { votedPostsKeys } from '@/lib/client/hooks/use-portal-posts-query'
import { useSessionContext } from '@/lib/client/hooks/use-root-context'
import { useAuthPopoverSafe } from '@/components/auth/auth-popover-context'
import { cn } from '@/lib/shared/utils'
import { getDownvotedPostsFn, toggleDownvoteFn } from './downvote-fns'

const KEY = ['downvotedPosts'] as const

export function DownvoteButton({ postId, className }: { postId: string; className?: string }) {
  const queryClient = useQueryClient()
  const session = useSessionContext() as { user?: { principalType?: string } } | null
  const signedIn = !!session?.user && session.user.principalType !== 'anonymous'
  const auth = useAuthPopoverSafe()
  const { data: down } = useQuery({
    queryKey: KEY,
    queryFn: async () => new Set((await getDownvotedPostsFn()).postIds),
    staleTime: 5 * 60_000,
    enabled: signedIn,
  })
  const { data: up } = useQuery<Set<string>>({ queryKey: votedPostsKeys.byWorkspace(), enabled: false })
  const active = down?.has(postId) ?? false
  const upvoted = up?.has(postId) ?? false
  // an upvote replaces the downvote on the server: mirror it here
  useEffect(() => {
    if (upvoted && active)
      queryClient.setQueryData<Set<string>>(KEY, (s) => {
        const next = new Set(s)
        next.delete(postId)
        return next
      })
  }, [upvoted, active, postId, queryClient])

  const mutation = useMutation({
    mutationFn: () => toggleDownvoteFn({ data: { postId } }),
    onSuccess: (r) => {
      queryClient.setQueryData(voteCountKeys.byPost(postId as never), r.voteCount)
      queryClient.setQueryData<Set<string>>(KEY, (s) => {
        const next = new Set(s)
        if (r.downvoted) next.add(postId)
        else next.delete(postId)
        return next
      })
      if (r.removedUpvote)
        queryClient.setQueryData<Set<string>>(votedPostsKeys.byWorkspace(), (s) => {
          const next = new Set(s)
          next.delete(postId)
          return next
        })
    },
  })
  return (
    <button
      type="button"
      aria-label={active ? 'Remove downvote' : 'Downvote'}
      aria-pressed={active}
      title={active ? 'Remove downvote' : 'Not a good idea'}
      disabled={mutation.isPending}
      onClick={(e) => {
        e.preventDefault()
        e.stopPropagation()
        if (!signedIn) auth?.openAuthPopover({ mode: 'login' })
        else mutation.mutate()
      }}
      className={cn(
        'group/down flex h-7 w-12 items-center justify-center rounded-lg border transition-colors duration-200',
        active
          ? 'border-destructive/50 bg-destructive/15 text-destructive'
          : 'border-border/50 bg-muted/40 text-muted-foreground hover:border-border hover:bg-muted/60 hover:text-foreground/80',
        mutation.isPending && 'cursor-wait opacity-70',
        className
      )}
    >
      <ChevronDownIcon className="h-4 w-4 transition-transform duration-200 group-hover/down:translate-y-0.5" />
    </button>
  )
}

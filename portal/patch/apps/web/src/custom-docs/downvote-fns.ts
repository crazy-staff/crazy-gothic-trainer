/**
 * Downvotes for ideas. A downvote takes one point off the post's score (posts.vote_count, the
 * number the feed shows and sorts by) and replaces the viewer's upvote if there is one.
 * Kept in its own small table, created on first use; signed-in users only.
 */
import { createServerFn } from '@tanstack/react-start'
import { z } from 'zod'

let ready: Promise<void> | null = null
async function ensure() {
  ready ??= (async () => {
    const { db, sql } = await import('@/lib/server/db')
    await db.execute(sql`CREATE TABLE IF NOT EXISTS post_downvotes (
      post_id uuid NOT NULL REFERENCES posts(id) ON DELETE CASCADE,
      principal_id uuid NOT NULL,
      created_at timestamptz NOT NULL DEFAULT now(),
      PRIMARY KEY (post_id, principal_id))`)
    // a score can go below zero now
    await db.execute(sql`ALTER TABLE posts DROP CONSTRAINT IF EXISTS vote_count_non_negative`)
  })().catch((e) => {
    ready = null
    throw e
  })
  return ready
}

async function viewer() {
  const { getOptionalAuth, hasAuthCredentials } = await import('@/lib/server/functions/auth-helpers')
  if (!hasAuthCredentials()) return null
  const auth = await getOptionalAuth()
  if (!auth || auth.principal.type === 'anonymous') return null
  return auth
}

const rows = <T,>(r: unknown): T[] => (Array.isArray(r) ? r : ((r as { rows?: T[] }).rows ?? [])) as T[]

/** Remove the viewer's downvote before they upvote (called by the vote function). */
export async function clearDownvote(postId: string, principalId: string): Promise<void> {
  await ensure()
  const { db, sql } = await import('@/lib/server/db')
  const { toUuid } = await import('@quackback/ids')
  const post = toUuid(postId as never)
  const who = toUuid(principalId as never)
  const gone = rows(await db.execute(sql`DELETE FROM post_downvotes WHERE post_id = ${post}::uuid AND principal_id = ${who}::uuid RETURNING 1`))
  if (gone.length) await db.execute(sql`UPDATE posts SET vote_count = vote_count + 1 WHERE id = ${post}::uuid`)
}

export const getDownvotedPostsFn = createServerFn({ method: 'GET' }).handler(async (): Promise<{ postIds: string[] }> => {
  const auth = await viewer()
  if (!auth) return { postIds: [] }
  await ensure()
  const { db, sql } = await import('@/lib/server/db')
  const { toUuid, fromUuid } = await import('@quackback/ids')
  const who = toUuid(auth.principal.id)
  const list = rows<{ post_id: string }>(await db.execute(sql`SELECT post_id FROM post_downvotes WHERE principal_id = ${who}::uuid`))
  return { postIds: list.map((r) => fromUuid('post', r.post_id) as string) }
})

export const toggleDownvoteFn = createServerFn({ method: 'POST' })
  .validator(z.object({ postId: z.string() }))
  .handler(async ({ data }): Promise<{ downvoted: boolean; removedUpvote: boolean; voteCount: number }> => {
    const auth = await viewer()
    if (!auth) throw new Error('Sign in to vote')
    const { assertPostVotable } = await import('@/lib/server/domains/posts/post.access')
    const { policyActorFromAuth } = await import('@/lib/server/functions/auth-helpers')
    await assertPostVotable(data.postId as never, await policyActorFromAuth(auth as never))
    await ensure()
    const { db, sql } = await import('@/lib/server/db')
    const { toUuid } = await import('@quackback/ids')
    const post = toUuid(data.postId as never)
    const who = toUuid(auth.principal.id)
    const had = rows(await db.execute(sql`DELETE FROM post_downvotes WHERE post_id = ${post}::uuid AND principal_id = ${who}::uuid RETURNING 1`)).length > 0
    let delta = 0
    let removedUpvote = false
    if (had) delta = 1
    else {
      const up = rows(await db.execute(sql`DELETE FROM post_votes WHERE post_id = ${post}::uuid AND principal_id = ${who}::uuid RETURNING 1`))
      removedUpvote = up.length > 0
      await db.execute(sql`INSERT INTO post_downvotes (post_id, principal_id) VALUES (${post}::uuid, ${who}::uuid) ON CONFLICT DO NOTHING`)
      delta = -1 - up.length
    }
    const out = rows<{ vote_count: number }>(
      await db.execute(sql`UPDATE posts SET vote_count = vote_count + ${delta} WHERE id = ${post}::uuid RETURNING vote_count`)
    )
    return { downvoted: !had, removedUpvote, voteCount: Number(out[0]?.vote_count ?? 0) }
  })

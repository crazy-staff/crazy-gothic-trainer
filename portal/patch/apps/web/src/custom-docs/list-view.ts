import { useCallback, useEffect, useState } from 'react'

export type ListView = 'list' | 'grid'

/** List or grid for one list, remembered per browser. */
export function useListView(key: string, fallback: ListView = 'list') {
  const storageKey = `list-view:${key}`
  const [view, setViewState] = useState<ListView>(fallback)
  useEffect(() => {
    try {
      const saved = localStorage.getItem(storageKey)
      if (saved === 'list' || saved === 'grid') setViewState(saved)
    } catch {
      /* storage unavailable */
    }
  }, [storageKey])
  const setView = useCallback(
    (next: ListView) => {
      setViewState(next)
      try {
        localStorage.setItem(storageKey, next)
      } catch {
        /* storage unavailable */
      }
    },
    [storageKey]
  )
  return [view, setView] as const
}

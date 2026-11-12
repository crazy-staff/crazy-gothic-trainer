/**
 * Fumadocs' default search dialog, rebuilt from its exported parts so the category filter sits
 * inside the dialog (in fumadocs-ui 16.19 the footer renders outside it, on the page).
 */
import { useState } from 'react'
import type { SharedProps } from 'fumadocs-ui/components/dialog/search'
import {
  SearchDialog,
  SearchDialogClose,
  SearchDialogContent,
  SearchDialogFooter,
  SearchDialogHeader,
  SearchDialogIcon,
  SearchDialogInput,
  SearchDialogList,
  SearchDialogOverlay,
  TagsList,
  TagsListItem,
} from 'fumadocs-ui/components/dialog/search'
import { useDocsSearch } from 'fumadocs-core/search/client'
import { fetchClient } from 'fumadocs-core/search/client/fetch'

export interface DocsSearchTag {
  name: string
  value: string
}

export function createDocsSearchDialog(tags: DocsSearchTag[]) {
  return function DocsSearchDialog(props: SharedProps) {
    const [tag, setTag] = useState<string | undefined>(undefined)
    const search = useDocsSearch({ client: fetchClient({ api: '/api/docs-search', tag }) })
    return (
      <SearchDialog {...search} {...props}>
        <SearchDialogOverlay />
        <SearchDialogContent>
          <SearchDialogHeader>
            <SearchDialogIcon />
            <SearchDialogInput />
            <SearchDialogClose />
          </SearchDialogHeader>
          <SearchDialogList />
          {tags.length > 0 && (
            <SearchDialogFooter>
              <TagsList tag={tag} onTagChange={setTag} allowClear>
                {tags.map((t) => (
                  <TagsListItem key={t.value} value={t.value}>
                    {t.name}
                  </TagsListItem>
                ))}
              </TagsList>
            </SearchDialogFooter>
          )}
        </SearchDialogContent>
      </SearchDialog>
    )
  }
}

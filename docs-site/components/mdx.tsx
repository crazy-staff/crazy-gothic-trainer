import defaultMdxComponents from 'fumadocs-ui/mdx';
import { Step, Steps } from 'fumadocs-ui/components/steps';
import type { MDXComponents } from 'mdx/types';
import type { ComponentProps, FC } from 'react';

/** Links to the Quackback portal (same domain, outside this app) must not get the /docs base path. */
const PORTAL = /^\/(roadmap|changelog|support|status|auth|b|hc)?(\/|$|\?)/;
const isPortal = (href?: string) => !!href && (href === '/' || (PORTAL.test(href) && !href.startsWith('/docs')));

export function getMDXComponents(components?: MDXComponents): MDXComponents {
  const A = (components?.a ?? defaultMdxComponents.a) as FC<ComponentProps<'a'>>;
  return {
    ...defaultMdxComponents,
    Step,
    Steps,
    ...components,
    a: (props: ComponentProps<'a'>) => (isPortal(props.href) ? <a {...props} /> : <A {...props} />),
  };
}

export const useMDXComponents = getMDXComponents;

declare global {
  type MDXProvidedComponents = ReturnType<typeof getMDXComponents>;
}

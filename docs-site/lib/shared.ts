export const appName = 'Gothic 1 Crazy Trainer';
/** Docs pages live at the site root of this app; Next adds the /docs base path. */
export const docsRoute = '/';
export const basePath = '/docs';

export const gitConfig = {
  user: 'dawidseipold',
  repo: 'gothic1-crazy-trainer',
  branch: 'main',
};

/**
 * Portal tabs, in the same order as the Quackback header. These are plain links to
 * the portal on the same domain (outside this app's base path).
 */
export const portalNav = [
  { label: 'Feedback', href: '/' },
  { label: 'Roadmap', href: '/roadmap' },
  { label: 'Changelog', href: '/changelog' },
  { label: 'Docs', href: '/docs/', active: true },
  { label: 'Support', href: '/support' },
  { label: 'Status', href: '/status' },
];

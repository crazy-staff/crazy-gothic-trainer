import { createMDX } from 'fumadocs-mdx/next';

const withMDX = createMDX();

/**
 * Static export served under /docs on the same domain as the Quackback portal
 * (a reverse proxy sends /docs/* here and everything else to Quackback).
 * @type {import('next').NextConfig}
 */
const config = {
  output: 'export',
  basePath: '/docs',
  trailingSlash: true,
  images: { unoptimized: true },
  reactStrictMode: true,
};

export default withMDX(config);

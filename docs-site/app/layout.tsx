import '@fontsource-variable/inter';
import { Provider } from '@/components/provider';
import { PortalHeader, PortalGlow } from '@/components/portal-header';
import './global.css';

export const metadata = {
  title: { template: '%s · Docs · Gothic 1 Crazy Trainer', default: 'Docs · Gothic 1 Crazy Trainer' },
  description: 'Documentation of Gothic 1 Crazy Trainer: install, every option, testing.',
};

export default function Layout({ children }: LayoutProps<'/'>) {
  return (
    <html lang="en" className="dark" style={{ colorScheme: 'dark' }} suppressHydrationWarning>
      <body className="flex flex-col min-h-screen">
        <Provider>
          <PortalHeader />
          <PortalGlow />
          {children}
        </Provider>
      </body>
    </html>
  );
}

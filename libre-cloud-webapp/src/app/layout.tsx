import type { Metadata } from "next";
import "./globals.css";
import { ClerkProvider } from "@clerk/nextjs";
import Navigation from "@/components/navigation";

export const metadata: Metadata = {
  title: "LibreCloud - Document Collaboration Platform",
  description: "The future of document collaboration. LibreOffice meets the cloud.",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" suppressHydrationWarning>
      <body className="antialiased">
        <ClerkProvider>
          <Navigation />
          {children}
        </ClerkProvider>
      </body>
    </html>
  );
}

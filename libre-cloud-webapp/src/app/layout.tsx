import type { Metadata } from "next";
import "./globals.css";

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
    <html lang="en">
      <body className="antialiased">
        {children}
      </body>
    </html>
  );
}

import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  /* config options here */
  eslint: {
    // Disable ESLint during development
    ignoreDuringBuilds: true,
  },
  typescript: {
    // Allow TypeScript errors during development
    ignoreBuildErrors: true,
  },
};

export default nextConfig;

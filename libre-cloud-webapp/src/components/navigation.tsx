"use client"

import { useUser, SignInButton, UserButton } from "@clerk/nextjs"
import Link from "next/link"

export default function Navigation() {
  const { user, isLoaded } = useUser()

  return (
    <nav className="bg-white shadow-sm border-b border-gray-200">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex justify-between items-center h-16">
          {/* Logo */}
          <div className="flex items-center">
            <Link href="/" className="text-2xl font-bold text-blue-600 hover:text-blue-700 transition-colors">
              LibreCloud
            </Link>
          </div>

          {/* Navigation Links & User Menu */}
          <div className="flex items-center space-x-4">
            {!isLoaded ? (
              <div className="animate-pulse flex space-x-4">
                <div className="rounded-full bg-gray-200 h-8 w-20"></div>
                <div className="rounded-full bg-gray-200 h-8 w-8"></div>
              </div>
            ) : user ? (
              <>
                {/* Navigation Links for Authenticated Users */}
                <Link
                  href="/dashboard"
                  className="text-gray-700 hover:text-blue-600 px-3 py-2 rounded-md text-sm font-medium transition-colors"
                >
                  Dashboard
                </Link>

                {/* User Button - Clerk handles the dropdown and menu automatically */}
                <UserButton 
                  afterSignOutUrl="/"
                  appearance={{
                    elements: {
                      avatarBox: "h-8 w-8"
                    }
                  }}
                />
              </>
            ) : (
              /* Sign In Button for Unauthenticated Users */
              <SignInButton mode="modal">
                <button className="bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded-md text-sm font-medium transition-colors">
                  Sign In
                </button>
              </SignInButton>
            )}
          </div>
        </div>
      </div>
    </nav>
  )
} 
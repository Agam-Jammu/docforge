"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { cn } from "@/lib/utils";
import { FileText, Settings, Upload } from "lucide-react";

const navItems = [
  { href: "/", label: "Dashboard", icon: FileText },
  { href: "/upload", label: "Upload", icon: Upload },
  { href: "/admin", label: "Workflows", icon: Settings },
];

export default function Sidebar() {
  const pathname = usePathname();

  return (
    <aside className="w-64 min-h-screen border-r bg-card flex flex-col">
      <div className="p-6 border-b">
        <Link href="/" className="flex items-center gap-2">
          <div className="h-8 w-8 rounded-md bg-primary flex items-center justify-center">
            <FileText className="h-4 w-4 text-primary-foreground" />
          </div>
          <div>
            <h1 className="font-semibold text-lg leading-tight">DocForge</h1>
            <p className="text-xs text-muted-foreground">Document Pipeline</p>
          </div>
        </Link>
      </div>

      <nav className="flex-1 p-4 space-y-1">
        {navItems.map((item) => {
          const isActive = pathname === item.href;
          const Icon = item.icon;
          return (
            <Link
              key={item.href}
              href={item.href}
              className={cn(
                "flex items-center gap-3 px-3 py-2.5 rounded-md text-sm font-medium transition-colors",
                isActive
                  ? "bg-primary/10 text-primary"
                  : "text-muted-foreground hover:bg-accent hover:text-accent-foreground"
              )}
            >
              <Icon className="h-4 w-4" />
              {item.label}
            </Link>
          );
        })}
      </nav>

      <div className="p-4 border-t">
        <p className="text-xs text-muted-foreground">
          Capture → Classify → Extract → Validate → Export
        </p>
      </div>
    </aside>
  );
}
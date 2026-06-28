"use client";

import { Badge } from "@/components/ui/badge";
import type { DocumentStatus } from "@/lib/types";

const statusConfig: Record<DocumentStatus, { label: string; variant: "success" | "warning" | "info" | "destructive" | "secondary" }> = {
  Pending: { label: "Pending", variant: "secondary" },
  Processing: { label: "Processing", variant: "info" },
  Validated: { label: "Validated", variant: "success" },
  Exported: { label: "Exported", variant: "success" },
  Failed: { label: "Failed", variant: "destructive" },
};

export default function StatusBadge({ status }: { status: DocumentStatus }) {
  const config = statusConfig[status] ?? { label: status, variant: "secondary" as const };
  return <Badge variant={config.variant}>{config.label}</Badge>;
}
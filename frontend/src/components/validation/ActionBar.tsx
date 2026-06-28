"use client";

import { Button } from "@/components/ui/button";
import { CheckCircle2, XCircle, Send, Loader2 } from "lucide-react";

interface ActionBarProps {
  status: string;
  valid: boolean;
  onAccept: () => void;
  onReject: () => void;
  onExport: () => void;
  loading?: boolean;
}

export default function ActionBar({
  status,
  valid,
  onAccept,
  onReject,
  onExport,
  loading,
}: ActionBarProps) {
  const isProcessing = status === "Processing" || status === "Pending";
  const isValidated = status === "Validated";
  const isExported = status === "Exported";

  return (
    <div className="sticky bottom-0 bg-background border-t p-4 flex items-center justify-between">
      <div className="text-sm text-muted-foreground">
        {status === "Validated" && "Document validated. Ready for export."}
        {status === "Exported" && "Document exported successfully."}
        {status === "Failed" && "Processing failed."}
        {isProcessing && "Document is being processed..."}
      </div>

      <div className="flex items-center gap-2">
        {!isValidated && !isExported && status !== "Failed" && (
          <>
            <Button
              variant="outline"
              size="sm"
              onClick={onReject}
              disabled={loading || isProcessing}
              className="gap-1.5"
            >
              {loading ? (
                <Loader2 className="h-4 w-4 animate-spin" />
              ) : (
                <XCircle className="h-4 w-4" />
              )}
              Reject
            </Button>
            <Button
              variant="default"
              size="sm"
              onClick={onAccept}
              disabled={loading || isProcessing}
              className="gap-1.5"
            >
              {loading ? (
                <Loader2 className="h-4 w-4 animate-spin" />
              ) : (
                <CheckCircle2 className="h-4 w-4" />
              )}
              Accept & Validate
            </Button>
          </>
        )}

        {isValidated && (
          <Button
            variant="default"
            size="sm"
            onClick={onExport}
            disabled={loading}
            className="gap-1.5"
          >
            {loading ? (
              <Loader2 className="h-4 w-4 animate-spin" />
            ) : (
              <Send className="h-4 w-4" />
            )}
            Export
          </Button>
        )}
      </div>
    </div>
  );
}
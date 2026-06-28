"use client";

import { useState, useEffect, useCallback } from "react";
import { useParams, useRouter } from "next/navigation";
import { ArrowLeft, RefreshCw, Loader2 } from "lucide-react";
import { Button } from "@/components/ui/button";
import DocumentViewer from "@/components/validation/DocumentViewer";
import ExtractedFields from "@/components/validation/ExtractedFields";
import ActionBar from "@/components/validation/ActionBar";
import StatusBadge from "@/components/documents/StatusBadge";
import {
  getDocumentExtracted,
  validateDocument,
  exportDocument,
} from "@/lib/api";
import { formatDate } from "@/lib/utils";
import type { DocumentDetail, BoundingBox } from "@/lib/types";

export default function DocumentDetailPage() {
  const params = useParams();
  const router = useRouter();
  const id = params.id as string;

  const [detail, setDetail] = useState<DocumentDetail | null>(null);
  const [fieldValues, setFieldValues] = useState<Record<string, string>>({});
  const [loading, setLoading] = useState(true);
  const [actionLoading, setActionLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [activeBox, setActiveBox] = useState<BoundingBox | null>(null);

  const fetchDetail = useCallback(async () => {
    try {
      setLoading(true);
      const data = await getDocumentExtracted(id);
      setDetail(data);

      // Initialize field values
      const values: Record<string, string> = {};
      for (const field of data.fields) {
        values[field.fieldName] = field.correctedValue ?? field.extractedValue ?? "";
      }
      setFieldValues(values);
      setError(null);
    } catch (err) {
      setError(err instanceof Error ? err.message : "Failed to load document");
    } finally {
      setLoading(false);
    }
  }, [id]);

  useEffect(() => {
    fetchDetail();
  }, [fetchDetail]);

  // Poll for status updates while processing
  useEffect(() => {
    if (!detail || (detail.status !== "Pending" && detail.status !== "Processing"))
      return;

    const interval = setInterval(async () => {
      try {
        const data = await getDocumentExtracted(id);
        setDetail(data);
        const values: Record<string, string> = {};
        for (const field of data.fields) {
          values[field.fieldName] = field.correctedValue ?? field.extractedValue ?? "";
        }
        setFieldValues(values);
      } catch {
        // ignore poll errors
      }
    }, 3000);

    return () => clearInterval(interval);
  }, [detail?.status, id]);

  const handleAccept = async () => {
    if (!detail) return;
    setActionLoading(true);
    try {
      const corrections = Object.entries(fieldValues)
        .filter(([name, value]) => {
          const field = detail.fields.find((f) => f.fieldName === name);
          return field && value !== field.extractedValue;
        })
        .map(([fieldName, correctedValue]) => ({
          fieldName,
          correctedValue,
        }));

      await validateDocument(detail.id, "accepted", corrections);
      await fetchDetail();
    } catch (err) {
      setError(err instanceof Error ? err.message : "Validation failed");
    } finally {
      setActionLoading(false);
    }
  };

  const handleReject = async () => {
    if (!detail) return;
    setActionLoading(true);
    try {
      await validateDocument(detail.id, "rejected", []);
      await fetchDetail();
    } catch (err) {
      setError(err instanceof Error ? err.message : "Rejection failed");
    } finally {
      setActionLoading(false);
    }
  };

  const handleExport = async () => {
    if (!detail) return;
    setActionLoading(true);
    try {
      await exportDocument(detail.id);
      await fetchDetail();
    } catch (err) {
      setError(err instanceof Error ? err.message : "Export failed");
    } finally {
      setActionLoading(false);
    }
  };

  if (loading) {
    return (
      <div className="p-8 max-w-7xl mx-auto">
        <div className="animate-pulse space-y-4">
          <div className="h-8 w-48 bg-muted rounded" />
          <div className="h-64 bg-muted rounded-lg" />
        </div>
      </div>
    );
  }

  if (error && !detail) {
    return (
      <div className="p-8 max-w-7xl mx-auto">
        <div className="text-center py-16">
          <p className="text-destructive font-medium text-lg">Failed to load document</p>
          <p className="text-muted-foreground text-sm mt-1">{error}</p>
          <Button variant="outline" onClick={() => router.push("/")} className="mt-4">
            <ArrowLeft className="h-4 w-4 mr-2" />
            Back to Dashboard
          </Button>
        </div>
      </div>
    );
  }

  if (!detail) return null;

  return (
    <div className="flex flex-col min-h-[calc(100vh-4rem)]">
      {/* Top bar */}
      <div className="border-b bg-background px-8 py-4">
        <div className="max-w-7xl mx-auto flex items-center justify-between">
          <div className="flex items-center gap-4">
            <Button
              variant="ghost"
              size="icon"
              onClick={() => router.push("/")}
            >
              <ArrowLeft className="h-4 w-4" />
            </Button>
            <div>
              <h1 className="text-lg font-semibold truncate max-w-md">
                {detail.filename}
              </h1>
              <div className="flex items-center gap-2 mt-0.5">
                <StatusBadge status={detail.status as any} />
                <span className="text-xs text-muted-foreground">
                  {formatDate(detail.uploadedAt)}
                </span>
              </div>
            </div>
          </div>

          <Button variant="outline" size="sm" onClick={fetchDetail}>
            <RefreshCw className="h-4 w-4 mr-1.5" />
            Refresh
          </Button>
        </div>
      </div>

      {/* Error toast */}
      {error && (
        <div className="bg-destructive/10 border-b border-destructive/20 px-8 py-2">
          <p className="text-sm text-destructive">{error}</p>
        </div>
      )}

      {/* Two-panel layout */}
      <div className="flex-1 flex flex-col lg:flex-row">
        {/* Left: Document Viewer */}
        <div className="lg:w-3/5 p-4 lg:p-8 lg:border-r">
          <DocumentViewer
            filename={detail.filename}
            storedFilename={detail.storedFilename}
            documentType={detail.documentType}
            activeBoundingBox={activeBox}
          />
        </div>

        {/* Right: Extracted Fields */}
        <div className="lg:w-2/5 p-4 lg:p-8 overflow-y-auto">
          <ExtractedFields
            fields={detail.fields}
            fieldValues={fieldValues}
            documentType={detail.documentType}
            confidence={detail.confidence}
            onFieldChange={(name, value) =>
              setFieldValues((prev) => ({ ...prev, [name]: value }))
            }
            onHighlightField={(box) => setActiveBox(box)}
          />
        </div>
      </div>

      {/* Bottom action bar */}
      <ActionBar
        status={detail.status}
        valid={detail.fields.length > 0}
        onAccept={handleAccept}
        onReject={handleReject}
        onExport={handleExport}
        loading={actionLoading}
      />
    </div>
  );
}
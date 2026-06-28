"use client";

import Link from "next/link";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import StatusBadge from "./StatusBadge";
import { formatDate } from "@/lib/utils";
import type { Document } from "@/lib/types";
import { FileText, ChevronRight } from "lucide-react";

interface DocumentListProps {
  documents: Document[];
  loading?: boolean;
}

export default function DocumentList({ documents, loading }: DocumentListProps) {
  if (loading) {
    return (
      <Card>
        <CardHeader>
          <CardTitle>Documents</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="space-y-3">
            {[1, 2, 3].map((i) => (
              <div key={i} className="h-16 bg-muted animate-pulse rounded-md" />
            ))}
          </div>
        </CardContent>
      </Card>
    );
  }

  if (documents.length === 0) {
    return (
      <Card>
        <CardHeader>
          <CardTitle>Documents</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="text-center py-8 text-muted-foreground">
            <FileText className="mx-auto h-12 w-12 mb-3 opacity-50" />
            <p className="text-sm">No documents yet</p>
            <p className="text-xs mt-1">Upload a document to get started</p>
          </div>
        </CardContent>
      </Card>
    );
  }

  return (
    <Card>
      <CardHeader>
        <CardTitle>Documents</CardTitle>
      </CardHeader>
      <CardContent className="p-0">
        <div className="divide-y">
          {documents.map((doc) => (
            <Link
              key={doc.id}
              href={`/documents/${doc.id}`}
              className="flex items-center gap-4 px-6 py-4 hover:bg-muted/50 transition-colors"
            >
              <div className="h-10 w-10 rounded-md bg-primary/10 flex items-center justify-center shrink-0">
                <FileText className="h-5 w-5 text-primary" />
              </div>
              <div className="flex-1 min-w-0">
                <p className="text-sm font-medium truncate">{doc.filename}</p>
                <div className="flex items-center gap-2 mt-1">
                  <StatusBadge status={doc.status as any} />
                  {doc.documentType && doc.documentType !== "unknown" && (
                    <Badge variant="outline" className="text-xs capitalize">
                      {doc.documentType.replace("_", " ")}
                    </Badge>
                  )}
                  {doc.confidence !== undefined && doc.confidence > 0 && (
                    <span className="text-xs text-muted-foreground">
                      {doc.confidence.toFixed(0)}% confidence
                    </span>
                  )}
                </div>
              </div>
              <div className="text-right shrink-0">
                <p className="text-xs text-muted-foreground">{formatDate(doc.uploadedAt)}</p>
              </div>
              <ChevronRight className="h-4 w-4 text-muted-foreground shrink-0" />
            </Link>
          ))}
        </div>
      </CardContent>
    </Card>
  );
}
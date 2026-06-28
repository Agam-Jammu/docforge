"use client";

import { useState, useEffect } from "react";
import DocumentList from "@/components/documents/DocumentList";
import DocumentUpload from "@/components/documents/DocumentUpload";
import { getDocuments } from "@/lib/api";
import type { Document } from "@/lib/types";

export default function DashboardPage() {
  const [documents, setDocuments] = useState<Document[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const fetchDocs = async () => {
      try {
        const docs = await getDocuments();
        setDocuments(docs);
      } catch {
        // API may not be running
      } finally {
        setLoading(false);
      }
    };
    fetchDocs();
  }, []);

  const handleUploadComplete = (newDocs: Document[]) => {
    // Reload from API to get the full records with IDs
    getDocuments().then(setDocuments).catch(() => {});
  };

  return (
    <div className="p-8 max-w-5xl mx-auto space-y-8">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Dashboard</h1>
        <p className="text-muted-foreground mt-1">
          Upload documents and manage the capture pipeline
        </p>
      </div>

      <DocumentUpload onUploadComplete={handleUploadComplete} />

      <DocumentList documents={documents} loading={loading} />
    </div>
  );
}
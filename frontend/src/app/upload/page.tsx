"use client";

import { useState } from "react";
import { useRouter } from "next/navigation";
import DocumentUpload from "@/components/documents/DocumentUpload";
import type { Document } from "@/lib/types";

export default function UploadPage() {
  const router = useRouter();
  const [uploaded, setUploaded] = useState<Document[]>([]);

  const handleUploadComplete = (docs: Document[]) => {
    setUploaded((prev) => [...prev, ...docs]);
  };

  return (
    <div className="p-8 max-w-3xl mx-auto space-y-8">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Upload Documents</h1>
        <p className="text-muted-foreground mt-1">
          Upload PDFs and images for processing through the capture pipeline
        </p>
      </div>

      <DocumentUpload onUploadComplete={handleUploadComplete} />

      {uploaded.length > 0 && (
        <div className="bg-green-50 border border-green-200 rounded-md p-4">
          <p className="text-sm font-medium text-green-800">
            {uploaded.length} document{uploaded.length > 1 ? "s" : ""} uploaded successfully
          </p>
          <div className="mt-2 space-y-1">
            {uploaded.map((doc) => (
              <div key={doc.id} className="flex items-center gap-2">
                <span className="text-sm text-green-700">{doc.filename}</span>
                <button
                  onClick={() => router.push(`/documents/${doc.id}`)}
                  className="text-xs text-primary hover:underline"
                >
                  View →
                </button>
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}
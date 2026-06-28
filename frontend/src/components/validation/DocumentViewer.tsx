"use client";

import { useState } from "react";
import { Card, CardContent } from "@/components/ui/card";
import { FileText } from "lucide-react";
import type { BoundingBox } from "@/lib/types";

interface DocumentViewerProps {
  filename: string;
  storedFilename?: string;
  documentType?: string;
  activeBoundingBox?: BoundingBox | null;
}

export default function DocumentViewer({
  filename,
  storedFilename,
  documentType,
  activeBoundingBox,
}: DocumentViewerProps) {
  const [imgError, setImgError] = useState(false);

  // The API serves uploaded files from the /uploads/ endpoint.
  // This is a raw static files endpoint, NOT under /api/.
  const apiBase = process.env.NEXT_PUBLIC_API_URL?.replace(/\/api$/, "") || "http://localhost:5178";
  // Use storedFilename (with UUID prefix) or fall back to original filename
  const imageUrl = `${apiBase}/uploads/${storedFilename || filename}`;

  return (
    <Card className="h-full">
      <CardContent className="p-4">
        <div className="relative bg-muted rounded-lg overflow-hidden min-h-[400px] flex items-center justify-center">
          {!imgError ? (
            <div className="relative w-full">
              <img
                src={imageUrl}
                alt={filename}
                className="w-full h-auto object-contain"
                onError={() => setImgError(true)}
              />
              {activeBoundingBox && (
                <div
                  className="absolute border-2 border-amber-500 bg-amber-500/20 pointer-events-none transition-all duration-200"
                  style={{
                    left: `${activeBoundingBox.x}px`,
                    top: `${activeBoundingBox.y}px`,
                    width: `${activeBoundingBox.width}px`,
                    height: `${activeBoundingBox.height}px`,
                  }}
                />
              )}
            </div>
          ) : (
            <div className="text-center py-16">
              <FileText className="mx-auto h-16 w-16 text-muted-foreground/50 mb-4" />
              <p className="text-sm font-medium text-muted-foreground">
                Document Preview
              </p>
              <p className="text-xs text-muted-foreground/70 mt-1">
                {filename}
              </p>
              {documentType && (
                <p className="text-xs text-muted-foreground/70 capitalize mt-1">
                  Type: {documentType.replace("_", " ")}
                </p>
              )}
              <p className="text-xs text-muted-foreground/50 mt-4">
                Image preview not available from the API
              </p>
            </div>
          )}
        </div>
      </CardContent>
    </Card>
  );
}
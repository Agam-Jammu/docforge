"use client";

import { Input } from "@/components/ui/input";
import { Badge } from "@/components/ui/badge";
import { confidenceBadgeColor, parseBoundingBox } from "@/lib/utils";
import type { ExtractedField, BoundingBox } from "@/lib/types";
import { AlertTriangle, CheckCircle2 } from "lucide-react";

interface FieldRowProps {
  field: ExtractedField;
  value: string;
  onChange: (value: string) => void;
  onHighlight?: (box: BoundingBox | null) => void;
}

export default function FieldRow({ field, value, onChange, onHighlight }: FieldRowProps) {
  const isLowConfidence = field.confidence < 60;
  const isCorrected = field.isHumanCorrected;
  const box = parseBoundingBox(field.boundingBoxJson);

  return (
    <div
      className={`rounded-md border p-3 transition-colors ${
        isLowConfidence
          ? "border-amber-300 bg-amber-50/50"
          : isCorrected
          ? "border-green-200 bg-green-50/50"
          : "border-border"
      }`}
    >
      <div className="flex items-center justify-between mb-1.5">
        <label className="text-sm font-medium capitalize flex items-center gap-1.5">
          {field.fieldName.replace(/_/g, " ")}
          {isLowConfidence && (
            <AlertTriangle className="h-3.5 w-3.5 text-amber-500" />
          )}
          {isCorrected && (
            <CheckCircle2 className="h-3.5 w-3.5 text-green-500" />
          )}
        </label>
        <div className="flex items-center gap-2">
          <Badge
            variant="outline"
            className={`text-xs ${confidenceBadgeColor(field.confidence)}`}
          >
            {field.confidence.toFixed(0)}%
          </Badge>
          {box && onHighlight && (
            <button
              onClick={() => onHighlight(box)}
              className="text-xs text-primary hover:underline"
              title="Show on document"
            >
              Locate
            </button>
          )}
        </div>
      </div>
      <Input
        value={value}
        onChange={(e) => onChange(e.target.value)}
        className={`text-sm ${
          isLowConfidence
            ? "border-amber-300 focus-visible:ring-amber-500"
            : ""
        }`}
      />
      {field.extractedValue !== value && (
        <p className="text-xs text-muted-foreground mt-1">
          Original: {field.extractedValue}
        </p>
      )}
    </div>
  );
}
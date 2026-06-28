"use client";

import FieldRow from "./FieldRow";
import { Badge } from "@/components/ui/badge";
import { confidenceBadgeColor } from "@/lib/utils";
import type { ExtractedField, BoundingBox } from "@/lib/types";

interface ExtractedFieldsProps {
  fields: ExtractedField[];
  fieldValues: Record<string, string>;
  documentType?: string;
  confidence?: number;
  onFieldChange: (fieldName: string, value: string) => void;
  onHighlightField?: (box: BoundingBox | null) => void;
}

export default function ExtractedFields({
  fields,
  fieldValues,
  documentType,
  confidence,
  onFieldChange,
  onHighlightField,
}: ExtractedFieldsProps) {
  const lowConfidenceCount = fields.filter((f) => f.confidence < 60).length;

  return (
    <div className="space-y-4">
      {/* Document type + confidence header */}
      <div className="flex items-center justify-between">
        <div>
          <h3 className="text-lg font-semibold">Extracted Fields</h3>
          {documentType && documentType !== "unknown" && (
            <p className="text-sm text-muted-foreground capitalize">
              Type: {documentType.replace("_", " ")}
            </p>
          )}
        </div>
        <div className="text-right">
          {confidence !== undefined && confidence > 0 && (
            <Badge
              variant="outline"
              className={`text-sm px-3 py-1 ${confidenceBadgeColor(confidence)}`}
            >
              {confidence.toFixed(0)}% overall confidence
            </Badge>
          )}
        </div>
      </div>

      {/* Low confidence warning */}
      {lowConfidenceCount > 0 && (
        <div className="bg-amber-50 border border-amber-200 rounded-md p-3 text-sm text-amber-800">
          <p className="font-medium">
            {lowConfidenceCount} field{lowConfidenceCount > 1 ? "s" : ""} need{lowConfidenceCount === 1 ? "s" : ""} review
          </p>
          <p className="text-amber-600 text-xs mt-0.5">
            Amber-highlighted fields have low confidence. Please verify the values and correct if necessary.
          </p>
        </div>
      )}

      {/* Field rows */}
      <div className="space-y-3">
        {fields.map((field) => (
          <FieldRow
            key={field.fieldName}
            field={field}
            value={fieldValues[field.fieldName] ?? field.extractedValue ?? ""}
            onChange={(value) => onFieldChange(field.fieldName, value)}
            onHighlight={onHighlightField}
          />
        ))}
      </div>

      {fields.length === 0 && (
        <div className="text-center py-8 text-muted-foreground">
          <p className="text-sm">No fields extracted yet</p>
          <p className="text-xs mt-1">Document may still be processing</p>
        </div>
      )}
    </div>
  );
}
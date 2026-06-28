export interface Document {
  id: string;
  filename: string;
  status: DocumentStatus;
  uploadedAt: string;
  documentType?: string;
  confidence?: number;
}

export type DocumentStatus =
  | "Pending"
  | "Processing"
  | "Validated"
  | "Exported"
  | "Failed";

export interface ExtractedField {
  id: string;
  fieldName: string;
  extractedValue: string;
  correctedValue?: string;
  confidence: number;
  boundingBoxJson?: string;
  isHumanCorrected: boolean;
}

export interface DocumentDetail extends Document {
  storedFilename?: string;
  rawText?: string;
  fields: ExtractedField[];
}

export interface FieldCorrection {
  fieldName: string;
  correctedValue: string;
}

export interface ValidateRequest {
  status: "accepted" | "rejected";
  corrections: FieldCorrection[];
}

export interface WorkflowConfig {
  id: string;
  documentType: string;
  exportTarget: string;
  exportConfigJson: string;
  createdAt: string;
}

export interface BoundingBox {
  x: number;
  y: number;
  width: number;
  height: number;
}
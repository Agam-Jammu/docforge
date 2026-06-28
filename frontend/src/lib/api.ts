import type { Document, DocumentDetail, FieldCorrection, WorkflowConfig } from "./types";

const API_BASE = process.env.NEXT_PUBLIC_API_URL || "http://localhost:5178/api";

async function apiFetch<T>(
  path: string,
  options?: RequestInit
): Promise<T> {
  const res = await fetch(`${API_BASE}${path}`, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  if (!res.ok) {
    const text = await res.text();
    throw new Error(`${res.status} ${res.statusText}: ${text}`);
  }
  return res.json();
}

export async function uploadDocuments(files: File[]): Promise<Document[]> {
  const formData = new FormData();
  files.forEach((f) => formData.append("files", f));

  const res = await fetch(`${API_BASE}/documents`, {
    method: "POST",
    body: formData,
  });
  if (!res.ok) {
    const text = await res.text();
    throw new Error(`${res.status} ${res.statusText}: ${text}`);
  }
  return res.json();
}

export async function getDocuments(): Promise<Document[]> {
  return apiFetch("/documents");
}

export async function getDocumentStatus(
  id: string
): Promise<{ id: string; filename: string; status: string; confidence?: number; documentType?: string }> {
  return apiFetch(`/documents/${id}/status`);
}

export async function getDocumentExtracted(id: string): Promise<DocumentDetail> {
  return apiFetch(`/documents/${id}/extracted`);
}

export async function validateDocument(
  id: string,
  status: "accepted" | "rejected",
  corrections: FieldCorrection[]
): Promise<{ id: string; status: string }> {
  return apiFetch(`/documents/${id}/validate`, {
    method: "POST",
    body: JSON.stringify({ status, corrections }),
  });
}

export async function exportDocument(
  id: string
): Promise<{ id: string; status: string; destination: string }> {
  return apiFetch(`/documents/${id}/export`, {
    method: "POST",
  });
}

export async function getWorkflows(): Promise<WorkflowConfig[]> {
  return apiFetch("/admin/workflows");
}

export async function upsertWorkflow(
  config: Omit<WorkflowConfig, "id" | "createdAt">
): Promise<{ status: string; documentType: string }> {
  return apiFetch("/admin/workflows", {
    method: "PUT",
    body: JSON.stringify(config),
  });
}
import { type ClassValue, clsx } from "clsx";
import { twMerge } from "tailwind-merge";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

export function formatDate(dateString: string): string {
  return new Date(dateString).toLocaleString();
}

export function confidenceColor(confidence: number): string {
  if (confidence >= 85) return "text-green-600 bg-green-50 border-green-200";
  if (confidence >= 60) return "text-amber-600 bg-amber-50 border-amber-200";
  return "text-red-600 bg-red-50 border-red-200";
}

export function confidenceBadgeColor(confidence: number): string {
  if (confidence >= 85) return "bg-green-100 text-green-800 border-green-200";
  if (confidence >= 60) return "bg-amber-100 text-amber-800 border-amber-200";
  return "bg-red-100 text-red-800 border-red-200";
}

export function parseBoundingBox(json?: string): { x: number; y: number; width: number; height: number } | null {
  if (!json) return null;
  try {
    return JSON.parse(json);
  } catch {
    return null;
  }
}
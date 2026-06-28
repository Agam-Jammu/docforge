"use client";

import { useState, useEffect } from "react";
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select";
import { Badge } from "@/components/ui/badge";
import { getWorkflows, upsertWorkflow } from "@/lib/api";
import type { WorkflowConfig } from "@/lib/types";
import { Settings, Loader2 } from "lucide-react";

export default function AdminPage() {
  const [workflows, setWorkflows] = useState<WorkflowConfig[]>([]);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    loadWorkflows();
  }, []);

  const loadWorkflows = async () => {
    try {
      setLoading(true);
      const data = await getWorkflows();
      setWorkflows(data);
    } catch (err) {
      setError(err instanceof Error ? err.message : "Failed to load workflows");
    } finally {
      setLoading(false);
    }
  };

  const handleUpdate = async (docType: string, exportTarget: string) => {
    setSaving(docType);
    setError(null);
    try {
      await upsertWorkflow({
        documentType: docType,
        exportTarget,
        exportConfigJson: "{}",
      });
      await loadWorkflows();
    } catch (err) {
      setError(err instanceof Error ? err.message : "Failed to update workflow");
    } finally {
      setSaving(null);
    }
  };

  const exportTargets = [
    { value: "json_webhook", label: "JSON Webhook" },
    { value: "postgres_write", label: "PostgreSQL Write" },
    { value: "file_export", label: "File Export (S3)" },
  ];

  return (
    <div className="p-8 max-w-4xl mx-auto space-y-8">
      <div>
        <h1 className="text-3xl font-bold tracking-tight">Workflows</h1>
        <p className="text-muted-foreground mt-1">
          Configure export targets per document type (no-code workflow configurator)
        </p>
      </div>

      {error && (
        <div className="bg-destructive/10 border border-destructive/20 rounded-md p-3 text-sm text-destructive">
          {error}
        </div>
      )}

      <Card>
        <CardHeader>
          <CardTitle>Export Configuration</CardTitle>
          <CardDescription>
            Choose where validated documents of each type are delivered
          </CardDescription>
        </CardHeader>
        <CardContent>
          {loading ? (
            <div className="space-y-3">
              {[1, 2, 3].map((i) => (
                <div key={i} className="h-14 bg-muted animate-pulse rounded-md" />
              ))}
            </div>
          ) : workflows.length === 0 ? (
            <div className="text-center py-8 text-muted-foreground">
              <Settings className="mx-auto h-12 w-12 mb-3 opacity-50" />
              <p className="text-sm">No workflow configurations found</p>
              <p className="text-xs mt-1">
                Upload and process documents to generate default workflows
              </p>
            </div>
          ) : (
            <div className="divide-y">
              {workflows.map((wf) => (
                <div
                  key={wf.id}
                  className="flex items-center justify-between py-4 first:pt-0 last:pb-0"
                >
                  <div>
                    <p className="text-sm font-medium capitalize">
                      {wf.documentType.replace(/_/g, " ")}
                    </p>
                    <Badge variant="outline" className="text-xs mt-1">
                      {exportTargets.find((t) => t.value === wf.exportTarget)?.label ?? wf.exportTarget}
                    </Badge>
                  </div>
                  <div className="flex items-center gap-3">
                    <Select
                      defaultValue={wf.exportTarget}
                      onValueChange={(value) => handleUpdate(wf.documentType, value)}
                    >
                      <SelectTrigger className="w-48">
                        <SelectValue />
                      </SelectTrigger>
                      <SelectContent>
                        {exportTargets.map((target) => (
                          <SelectItem key={target.value} value={target.value}>
                            {target.label}
                          </SelectItem>
                        ))}
                      </SelectContent>
                    </Select>
                    {saving === wf.documentType && (
                      <Loader2 className="h-4 w-4 animate-spin text-muted-foreground" />
                    )}
                  </div>
                </div>
              ))}
            </div>
          )}
        </CardContent>
      </Card>
    </div>
  );
}
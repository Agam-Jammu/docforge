CREATE TABLE IF NOT EXISTS documents (
    id UUID PRIMARY KEY,
    filename VARCHAR(500) NOT NULL,
    document_type VARCHAR(100) DEFAULT 'unknown',
    status VARCHAR(20) DEFAULT 'Pending',
    confidence DOUBLE PRECISION DEFAULT 0,
    uploaded_at TIMESTAMPTZ DEFAULT NOW(),
    raw_file_path TEXT,
    raw_text TEXT
);

CREATE INDEX IF NOT EXISTS idx_documents_status ON documents(status) WHERE status = 'Pending';

CREATE TABLE IF NOT EXISTS extracted_fields (
    id UUID PRIMARY KEY,
    document_id UUID NOT NULL REFERENCES documents(id) ON DELETE CASCADE,
    field_name VARCHAR(200) NOT NULL,
    extracted_value TEXT NOT NULL,
    corrected_value TEXT,
    bounding_box_json TEXT,
    confidence DOUBLE PRECISION DEFAULT 0,
    is_human_corrected BOOLEAN DEFAULT FALSE
);

CREATE INDEX IF NOT EXISTS idx_extracted_fields_document_id ON extracted_fields(document_id);

CREATE TABLE IF NOT EXISTS corrections (
    id UUID PRIMARY KEY,
    document_id UUID NOT NULL,
    field_name VARCHAR(200) NOT NULL,
    original_value TEXT NOT NULL,
    corrected_value TEXT NOT NULL,
    corrected_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_corrections_document_id ON corrections(document_id);

CREATE TABLE IF NOT EXISTS workflow_configs (
    id UUID PRIMARY KEY,
    document_type VARCHAR(100) NOT NULL UNIQUE,
    export_target VARCHAR(50) DEFAULT 'json_webhook',
    export_config_json TEXT DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS export_log (
    id UUID PRIMARY KEY,
    document_id UUID NOT NULL,
    exported_at TIMESTAMPTZ DEFAULT NOW(),
    destination VARCHAR(100) NOT NULL,
    status VARCHAR(20) DEFAULT 'pending',
    response_json TEXT
);

CREATE INDEX IF NOT EXISTS idx_export_log_document_id ON export_log(document_id);

-- Seed default workflow configs
INSERT INTO workflow_configs (id, document_type, export_target) VALUES
    (gen_random_uuid(), 'invoice', 'json_webhook'),
    (gen_random_uuid(), 'receipt', 'json_webhook'),
    (gen_random_uuid(), 'medical_form', 'file_export')
ON CONFLICT (document_type) DO NOTHING;
-- Mock ERP/CRM table for postgres_write export target
CREATE TABLE IF NOT EXISTS mock_erp_orders (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    document_id UUID NOT NULL REFERENCES documents(id),
    document_type VARCHAR(100) NOT NULL,
    filename VARCHAR(500) NOT NULL,
    payload_json TEXT NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_mock_erp_orders_document_id ON mock_erp_orders(document_id);
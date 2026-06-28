"""
DocForge Document Classifier — FastAPI microservice.

Exposes a single classification endpoint used by the .NET Core orchestrator
as part of the cascading recognition pipeline (Strategy A fallback).
"""

import logging
from pathlib import Path

from fastapi import FastAPI
from pydantic import BaseModel

from .classifier import DocumentClassifier

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger(__name__)

app = FastAPI(
    title="DocForge Classifier",
    description="Document type classifier using TF-IDF + Logistic Regression",
    version="1.0.0",
)

classifier = DocumentClassifier()


class ClassifyRequest(BaseModel):
    text: str


class ClassifyResponse(BaseModel):
    document_type: str
    confidence: float
    needs_review: bool


@app.get("/health")
async def health():
    return {"status": "ok", "service": "classifier"}


@app.post("/classify", response_model=ClassifyResponse)
async def classify(request: ClassifyRequest) -> ClassifyResponse:
    """
    Classify a document's OCR text into a document type.

    Returns the predicted type, confidence score (0-100), and a flag
    indicating whether the result needs human review (confidence < 75%).
    """
    doc_type, confidence = classifier.predict(request.text)
    return ClassifyResponse(
        document_type=doc_type,
        confidence=confidence,
        needs_review=confidence < 75.0,
    )
#!/usr/bin/env python3
"""
Training script for the ClearCapture document classifier.

Usage:
    python train.py [--samples 500]

Generates synthetic training data, trains a TF-IDF + Logistic Regression
pipeline, and saves the model to model/classifier.joblib.

To use real RVL-CDIP data, pass the path to extracted text files:
    python train.py --data /path/to/rvl-cdip/texts/ --labels /path/to/labels.csv
"""

import argparse
import logging
import sys
from pathlib import Path

# Add parent to path so we can import from app
sys.path.insert(0, str(Path(__file__).parent))

from app.classifier import DocumentClassifier, _generate_synthetic_samples

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)
logger = logging.getLogger(__name__)


def main():
    parser = argparse.ArgumentParser(description="Train document classifier")
    parser.add_argument(
        "--samples",
        type=int,
        default=500,
        help="Number of synthetic samples per category",
    )
    parser.add_argument(
        "--data",
        type=str,
        default=None,
        help="Path to RVL-CDIP text files (optional, uses synthetic data otherwise)",
    )
    parser.add_argument(
        "--labels",
        type=str,
        default=None,
        help="Path to RVL-CDIP labels CSV (required with --data)",
    )
    args = parser.parse_args()

    classifier = DocumentClassifier()

    if args.data and args.labels:
        logger.info("Loading RVL-CDIP data from %s", args.data)
        texts, labels = _load_rvl_cdip(args.data, args.labels)
    else:
        logger.info(
            "Generating %d synthetic samples per category...", args.samples
        )
        texts, labels = _generate_synthetic_samples(
            n_per_category=args.samples
        )

    logger.info(
        "Training classifier on %d samples across %d categories...",
        len(texts),
        len(set(labels)),
    )
    classifier._train(texts, labels)  # noqa: SLF001
    logger.info("Done. Model saved.")


def _load_rvl_cdip(data_path: str, labels_path: str) -> tuple[list[str], list[str]]:
    """Load RVL-CDIP text files and labels.

    The labels CSV should have columns: filename, label
    Text files should be named <filename>.txt in data_path.
    """
    import pandas as pd

    df = pd.read_csv(labels_path)
    texts: list[str] = []
    labels: list[str] = []

    data_dir = Path(data_path)
    for _, row in df.iterrows():
        txt_path = data_dir / f"{row['filename']}.txt"
        if txt_path.exists():
            texts.append(txt_path.read_text(encoding="utf-8", errors="ignore"))
            labels.append(row["label"])

    return texts, labels


if __name__ == "__main__":
    main()
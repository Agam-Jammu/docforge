FROM ubuntu:24.04 AS builder

# Prevent interactive debconf prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    pkg-config \
    libtesseract-dev \
    tesseract-ocr \
    tesseract-ocr-eng \
    libopencv-dev \
    libpoppler-dev \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy source files (build context is the clearcapture/ root)
COPY cpp-engine/CMakeLists.txt ./CMakeLists.txt
COPY cpp-engine/src/ ./src/

# Build
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --parallel $(nproc) && \
    cmake --install . --prefix /install

# Runtime stage
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies only
RUN apt-get update && apt-get install -y --no-install-recommends \
    libtesseract5 \
    tesseract-ocr \
    tesseract-ocr-eng \
    libopencv-core406 \
    libopencv-imgproc406 \
    libopencv-imgcodecs406 \
    libpoppler146 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /install/lib/libclearcapture_engine.so /usr/lib/
COPY --from=builder /install/bin/clearcapture_cli /usr/local/bin/

# Set Tesseract data path
ENV TESSDATA_PREFIX=/usr/share/tesseract-ocr/5/tessdata/

ENTRYPOINT ["clearcapture_cli"]
CMD ["--help"]
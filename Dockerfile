# =============================================================
# Stage 1: Build C++ Engine (Ubuntu 24.04 + Tesseract + OpenCV)
# =============================================================
FROM ubuntu:24.04 AS cpp-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    pkg-config \
    libtesseract-dev \
    tesseract-ocr \
    tesseract-ocr-eng \
    libopencv-dev \
    libpoppler-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src/cpp-engine
COPY cpp-engine/CMakeLists.txt .
COPY cpp-engine/src/ ./src/

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --parallel $(nproc) && \
    cp docforge_cli /docforge_cli

# =============================================================
# Stage 2: Build .NET API
# =============================================================
FROM mcr.microsoft.com/dotnet/sdk:10.0 AS dotnet-builder
WORKDIR /src/api
COPY api/DocForge.Api/DocForge.Api.csproj .
RUN dotnet restore
COPY api/DocForge.Api/ .
RUN dotnet publish -c Release -o /app

# =============================================================
# Stage 3: Runtime (C++ engine + .NET runtime)
# =============================================================
FROM mcr.microsoft.com/dotnet/aspnet:10.0

# Install C++ runtime deps
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libtesseract5 \
    tesseract-ocr \
    tesseract-ocr-eng \
    libopencv-core406t64 \
    libopencv-imgproc406t64 \
    libopencv-imgcodecs406t64 \
    libpoppler-cpp0t64 \
    libpoppler134 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ENV TESSDATA_PREFIX=/usr/share/tesseract-ocr/5/tessdata/

WORKDIR /app
COPY --from=cpp-builder /docforge_cli /app/docforge_cli
COPY --from=dotnet-builder /app ./

EXPOSE 8080
ENV ASPNETCORE_URLS=http://+:8080
ENTRYPOINT ["./DocForge.Api"]
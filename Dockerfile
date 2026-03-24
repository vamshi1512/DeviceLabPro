FROM node:20-bookworm

ENV DEBIAN_FRONTEND=noninteractive
ENV DEVICELAB_PROJECT_ROOT=/workspace

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    git \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY tools/bridge/requirements.txt /workspace/tools/bridge/requirements.txt
COPY dashboard/package.json /workspace/dashboard/package.json
COPY dashboard/package-lock.json /workspace/dashboard/package-lock.json

RUN python3 -m pip install --no-cache-dir -r tools/bridge/requirements.txt \
    && npm ci --prefix dashboard

COPY . /workspace

CMD ["/bin/bash"]

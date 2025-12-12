# Use the official PyTorch image with CUDA support
FROM pytorch/pytorch:2.2.0-cuda12.1-cudnn8-runtime

WORKDIR /app

# 1. System Dependencies (git, etc.)
RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# 2. Python Dependencies
# Copy requirements first to cache pip install
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 3. Copy Source Code
COPY src/ src/
COPY scripts/ scripts/
COPY data/ data/

# 4. Environment Variables
ENV PYTHONPATH=/app

# 5. Default Command (Train Model)
# When you run this container, it will train and export artifacts
CMD ["python", "scripts/train.py"]
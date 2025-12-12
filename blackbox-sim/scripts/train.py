import sys
import os
import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
import pandas as pd

# Add src to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from src.models.autoencoder import LogAutoencoder
from src.preprocessing.tokenizer import LogTokenizer
from src.preprocessing.scaler import FeatureScaler

def main():
    # 1. Configuration
    DATA_PATH = "data/raw/training_logs.txt" # You need to provide this
    ARTIFACTS_DIR = "data/artifacts"
    os.makedirs(ARTIFACTS_DIR, exist_ok=True)

    print(">>> [1/5] Loading Data...")
    # For MVP, let's generate fake data if file doesn't exist
    if not os.path.exists(DATA_PATH):
        print("    Data file not found. Generating SYNTHETIC training data...")
        logs = ["Accepted password for root from 192.168.1.1 port 22 ssh2"] * 5000 + \
            ["Disconnected from user admin 10.0.0.5"] * 5000
    else:
        with open(DATA_PATH, 'r') as f:
            logs = f.readlines()

    # 2. Tokenization
    print(">>> [2/5] Building Vocabulary...")
    tokenizer = LogTokenizer(vocab_size=5000, seq_len=128)
    tokenizer.build_vocab(logs)

    # Convert text to integers
    vectors = [tokenizer.encode(log) for log in logs]
    np_vectors = np.array(vectors, dtype=np.float32)

    # 3. Scaling
    print(">>> [3/5] Fitting Scaler...")
    scaler = FeatureScaler()
    scaler.fit(np_vectors)
    normalized_data = scaler.transform(np_vectors)

    # 4. Training
    print(">>> [4/5] Training Autoencoder...")
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model = LogAutoencoder().to(device)

    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)

    dataset = torch.tensor(normalized_data).to(device)
    batch_size = 64
    epochs = 5 # Short for MVP

    for epoch in range(epochs):
        permutation = torch.randperm(dataset.size(0))
        epoch_loss = 0.0

        for i in range(0, dataset.size(0), batch_size):
            indices = permutation[i:i+batch_size]
            batch = dataset[indices]

            # Autoencoder: Input == Target
            optimizer.zero_grad()
            outputs = model(batch)
            loss = criterion(outputs, batch)
            loss.backward()
            optimizer.step()
            epoch_loss += loss.item()

        print(f"    Epoch {epoch+1}/{epochs} | Loss: {epoch_loss/len(dataset):.6f}")

    # 5. Export
    print(">>> [5/5] Exporting Artifacts...")

    # Save Vocab
    tokenizer.save_vocab(f"{ARTIFACTS_DIR}/vocab.txt")

    # Save Scaler
    scaler.save_params(f"{ARTIFACTS_DIR}/scaler_params.txt")

    # Save ONNX Model
    dummy_input = torch.randn(1, 128).to(device)
    onnx_path = f"{ARTIFACTS_DIR}/autoencoder.onnx"
    torch.onnx.export(
        model, 
        dummy_input, 
        onnx_path,
        input_names=['input'],
        output_names=['output'],
        dynamic_axes={'input': {0: 'batch_size'}, 'output': {0: 'batch_size'}}
    )
    print(f"[SIM] Model exported to {onnx_path}")
    print("\nDONE. Copy files from 'data/artifacts/' to 'blackbox-core/config/'")

if __name__ == "__main__":
    main()
import pandas as pd
from src.interfaces.i_loader import ILoader
from src.common.types import LogSample

class CsvLoader(ILoader):
    def __init__(self):
        self.data = None

    def load_source(self, path: str) -> None:
        # Logic to read CSV using Pandas
        self.data = pd.read_csv(path)
        print(f"Loaded {len(self.data)} rows.")

    def next_batch(self, size: int):
        # Logic to yield chunks of data
        # ... implementation details ...
        pass
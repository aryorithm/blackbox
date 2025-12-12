import re
import json
from collections import Counter

class LogTokenizer:
    def __init__(self, vocab_size=10000, seq_len=128):
        self.vocab_size = vocab_size
        self.seq_len = seq_len
        self.vocab = {}
        self.special_tokens = {"[UNK]": 0, "[PAD]": 1}

    def build_vocab(self, texts):
        """
Scans all logs and builds a frequency map.
        """
        counter = Counter()
        for text in texts:
            # Simple whitespace splitting (matches C++ logic)
            tokens = text.strip().split()
            counter.update(tokens)

        # Keep top N most frequent words
        most_common = counter.most_common(self.vocab_size - len(self.special_tokens))

        self.vocab = self.special_tokens.copy()
        for idx, (word, _) in enumerate(most_common):
            self.vocab[word] = idx + len(self.special_tokens)

    def encode(self, text):
        """
Converts text -> List[int]
        """
        tokens = text.strip().split()
        vector = []

        for token in tokens[:self.seq_len]:
            vector.append(self.vocab.get(token, self.vocab["[UNK]"]))

        # Padding
        while len(vector) < self.seq_len:
            vector.append(self.vocab["[PAD]"])

        return vector

    def save_vocab(self, path):
        """
Exports vocab.txt for C++ consumption.
Format: One word per line. Line number is index.
        """
        # Create a reverse map (Index -> Word) to ensure order
        index_to_word = {v: k for k, v in self.vocab.items()}

        with open(path, 'w', encoding='utf-8') as f:
            for i in range(len(index_to_word)):
                f.write(f"{index_to_word.get(i, '[UNK]')}\n")

        print(f"[SIM] Vocab exported to {path} ({len(self.vocab)} tokens)")
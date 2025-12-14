import unittest
import torch
import sys
import os

# Add src to path to allow import
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../blackbox-sim')))

from src.models.autoencoder import LogAutoencoder

class TestAutoencoder(unittest.TestCase):
    def setUp(self):
        self.input_dim = 128
        self.model = LogAutoencoder(input_dim=self.input_dim)

    def test_dimensions(self):
        """Test if output shape matches input shape"""
        batch_size = 10
        dummy_input = torch.randn(batch_size, self.input_dim)

        output = self.model(dummy_input)

        self.assertEqual(output.shape, (batch_size, self.input_dim))

    def test_values_range(self):
        """Test if Sigmoid activation keeps output between 0 and 1"""
        dummy_input = torch.randn(5, self.input_dim)
        output = self.model(dummy_input)

        self.assertTrue((output >= 0).all())
        self.assertTrue((output <= 1).all())

if __name__ == '__main__':
    unittest.main()
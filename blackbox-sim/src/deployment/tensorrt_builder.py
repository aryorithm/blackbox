import tensorrt as trt

class TensorRTBuilder:
    def build_engine(self, onnx_path: str, output_path: str):
        # 1. Create the Builder
        logger = trt.Logger(trt.Logger.WARNING)
        builder = trt.Builder(logger)

        # 2. Define Network (Parse ONNX)
        # ... logic to map Python layers to C++ layers ...

        # 3. Build & Serialize
        # Saves the .plan file that you drag-and-drop into blackbox-core
        with open(output_path, "wb") as f:
            f.write(engine.serialize())
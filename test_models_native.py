#!/usr/bin/env python3
import sys
import numpy as np

# Try to import onnxruntime
try:
    import onnxruntime as ort
    print(f"✓ Using onnxruntime version: {ort.__version__}")
except ImportError:
    print("✗ onnxruntime not installed. Installing...")
    import subprocess
    subprocess.run([sys.executable, "-m", "pip", "install", "onnxruntime", "--user"], check=True)
    import onnxruntime as ort
    print(f"✓ Installed onnxruntime version: {ort.__version__}")

print("\n" + "="*60)
print("TESTING ENCODER")
print("="*60)

try:
    encoder = ort.InferenceSession(
        "rust-guest/src/encoder_Q.basic.ort",
        providers=['CPUExecutionProvider']
    )
    print("✓ Encoder loaded successfully")
    
    # Print input info
    print("\nEncoder inputs:")
    for inp in encoder.get_inputs():
        print(f"  - {inp.name}: shape={inp.shape}, dtype={inp.type}")
    
    # Create test inputs (same as your Rust code)
    image_input = np.full((1, 3, 256, 256), -1.0, dtype=np.float32)
    bits_input = np.array([[1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1] + [0]*80], dtype=np.float32)
    
    print("\nRunning encoder inference...")
    encoder_out = encoder.run(None, {
        "onnx::Concat_0": image_input,
        "onnx::Gemm_1": bits_input
    })
    
    print(f"✓ Encoder output shape: {encoder_out[0].shape}")
    print(f"  Output range: [{encoder_out[0].min():.6f}, {encoder_out[0].max():.6f}]")
    print(f"  Output sample (first 20): {encoder_out[0].flatten()[:20]}")
    
except Exception as e:
    print(f"✗ Encoder failed: {e}")
    sys.exit(1)

print("\n" + "="*60)
print("TESTING DECODER")
print("="*60)

try:
    decoder = ort.InferenceSession(
        "rust-guest/src/decoder_Q.basic.ort",
        providers=['CPUExecutionProvider']
    )
    print("✓ Decoder loaded successfully")
    
    # Print input info
    print("\nDecoder inputs:")
    for inp in decoder.get_inputs():
        print(f"  - {inp.name}: shape={inp.shape}, dtype={inp.type}")
    
    print("\nRunning decoder inference...")
    decoder_out = decoder.run(None, {"image": encoder_out[0]})
    
    print(f"✓ Decoder output shape: {decoder_out[0].shape}")
    print(f"  Output range: [{decoder_out[0].min():.6f}, {decoder_out[0].max():.6f}]")
    print(f"  Output sample (first 20): {decoder_out[0].flatten()[:20]}")
    
    # Convert to bits
    bits = ''.join(['1' if v > 0 else '0' for v in decoder_out[0].flatten()])
    print(f"\n  Decoded bits: {bits}")
    
    # Compare with expected
    expected = "1011011110011000111111000000011111011111011100000110110110111" + "0"*35
    errors = sum(a != b for a, b in zip(bits, expected))
    print(f"  Bit errors: {errors}/100")
    
    if errors <= 5:
        print("\n✓✓✓ SUCCESS! Models work correctly in native Python!")
        print("    → The problem is WASM ONNX Runtime missing operators")
        print("    → You need a different WASM build or simpler models")
    else:
        print(f"\n✗✗✗ FAILURE! Models have {errors} errors even in native Python!")
        print("    → The model files themselves are broken/incompatible")
        print("    → Check if you have the correct source .onnx files")
        
except Exception as e:
    print(f"✗ Decoder failed: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)


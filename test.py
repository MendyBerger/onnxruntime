import onnxruntime as ort

# Load the CONVERTED .ort file (not the .onnx)
session = ort.InferenceSession(
    "/media/mendyberger/USB-Card/wasi/onnxruntime/rust-guest/src/encoder_B.disable.ort",
    providers=['CPUExecutionProvider']
)

print("Inputs in .ort file:")
for inp in session.get_inputs():
    print(f"  {inp.name}: {inp.shape}")

print("\nOutputs in .ort file:")
for out in session.get_outputs():
    print(f"  {out.name}: {out.shape}")

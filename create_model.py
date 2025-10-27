import torch
import torch.nn as nn

# 1. Define a simple PyTorch model
class SimpleModel(nn.Module):
    def forward(self, x):
        # This model just multiplies the input tensor by 2
        return x * 2.0

# 2. Create an instance and set it to evaluation mode
model = SimpleModel()
model.eval()

# 3. Define a dummy input with the shape we want
# We'll use a shape of [1, 5] (a batch of 1, with 5 features)
dummy_input = torch.randn(1, 5)

# 4. Define the input and output names
input_names = ["input_tensor"]
output_names = ["output_tensor"]

# 5. Export the model
torch.onnx.export(model,
                  dummy_input,
                  "simple_model.onnx",         # where to save the model
                  input_names=input_names,   # the model's input names
                  output_names=output_names, # the model's output names
                  opset_version=14)

print("Created simple_model.onnx successfully!")

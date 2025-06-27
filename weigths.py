import torch
import numpy as np
import torch.nn as nn

INPUT_SIZE = 768
HL_SIZE_MODEL = 512  # ft_size = 512
QA, QB = 8, 6

feature_layer_scale = 1 << QA           # ft (input -> 512), accumulator values
hidden_layer_scale = 1 << QB            # Not directly used for L1 weights scaling if L1Bias changes
final_output_scale = 1 << (QA + QB)   # l1 output, l2 bias

def write_array(name, arr, dtype="int16_t"):
    flat = arr.flatten()
    with open(f"{name}.cpp", "w") as f:
        f.write(f"#include <cstdint>\n\n")
        f.write(f"{dtype} {name}[{len(flat)}] = {{\n")
        for i, x in enumerate(flat):
            f.write(f"{int(round(x))}, ")
            if (i + 1) % 8 == 0 and (i + 1) < len(flat):
                f.write("\n")
        f.write("\n};\n")

class NNUE(nn.Module):
    def __init__(self, input_size, ft_size):
        super(NNUE, self).__init__()
        M = ft_size
        N = 32
        K = 1
        self.ft = nn.Linear(768, M)
        self.l1 = nn.Linear(2 * M, N)
        self.l2 = nn.Linear(N, K)

    def forward(self, white_features, black_features, stm):
        w = self.ft(white_features)
        b = self.ft(black_features)
        stm = stm.unsqueeze(1)
        accumulator = (stm * torch.cat([w, b], dim=1)) + ((1 - stm) * torch.cat([b, w], dim=1))
        l1_x = torch.clamp(accumulator, 0.0, 1.0)
        l2_x = torch.clamp(self.l1(l1_x), 0.0, 1.0)
        return self.l2(l2_x)

model = NNUE(INPUT_SIZE, HL_SIZE_MODEL)
checkpoint = torch.load("last_chess_nnue_checkpoint.pth", map_location="cpu")
state_dict = checkpoint["model_state_dict"]
# Extract and quantize weights
ft_w = state_dict["ft.weight"].numpy().T * feature_layer_scale      # [768, 512]
ft_b = state_dict["ft.bias"].numpy() * feature_layer_scale          # [512]

# L1 weights are multiplied by accumulator values (QA fixed), result should be QA+QB fixed
# So, L1 weights themselves should be QB fixed. This is consistent with original code.
l1_w = state_dict["l1.weight"].numpy() * hidden_layer_scale         # [32, 1024]

# CORRECTED: L1 biases should be scaled by final_output_scale (QA+QB), not hidden_layer_scale (QB)
l1_b = state_dict["l1.bias"].numpy() * final_output_scale           # [32]

# CORRECTED: L2 weights should be scaled by 1 (or 0-bit fixed point)
# The L1 output (hidden[i] in C++) is already QA+QB fixed point.
# Multiplying by L2 weights scaled by 1 keeps it in QA+QB fixed point.
l2_w = state_dict["l2.weight"].numpy().flatten() * 1.0             # [32]

# L2 bias remains scaled by final_output_scale (QA+QB)
l2_b = state_dict["l2.bias"].numpy()[0] * final_output_scale       # scalar

# Write weights to .cpp files
write_array("FeatureWeights", ft_w, "int16_t")  # int16_t FeatureWeights[768 * 512]
write_array("FeatureBiases", ft_b, "int16_t")   # int16_t FeatureBiases[512]

write_array("L1Weights", l1_w, "int16_t")       # int16_t L1Weights[32 * 1024]
write_array("L1Biases", l1_b, "int32_t")        # int32_t L1Biases[32] (Changed to int32_t as it will hold a larger scaled value)

write_array("L2Weights", l2_w, "int16_t")       # int16_t L2Weights[32]
write_array("L2Bias", np.array([l2_b]), "int32_t") # int32_t L2Bias[1] (Already int32_t)


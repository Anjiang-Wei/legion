templatefile = "bsub_stencil_part.lsf"

lines = []

with open(templatefile, "r") as fin:
    lines = fin.readlines()

configs = {
    1: [(1, 4, 2, 2), (4, 1, 2, 2)], # 4 GPUs
    2: [(1, 8, 4, 2), (2, 4, 4, 2), (8, 1, 4, 2)], # 8 GPUs
    4: [(1, 16, 4, 4), (2, 8, 4, 4), (8, 2, 4, 4), (16, 1, 4, 4)], # 16 GPUs
    8: [(1, 32, 8, 4), (2, 16, 8, 4), (4, 8, 8, 4), (16, 2, 8, 4), (32, 1, 8, 4)], # 32 GPUs
    16: [(1, 64, 8, 8), (2, 32, 8, 8), (4, 16, 8, 8), (32, 2, 8, 8), (64, 1, 8, 8)], # 64 GPUs
    32: [(1, 128, 16, 8), (2, 64, 16, 8), (4, 32, 16, 8), (8, 16, 16, 8), (32, 4, 16, 8), (64, 2, 16, 8), (128, 1, 16, 8)], # 128 GPUs
    64: [(1, 256, 16, 16), (2, 128, 16, 16), (4, 64, 16, 16), (8, 32, 16, 16), (32, 8, 16, 16), (64, 4, 16, 16), (128, 2, 16, 16), (256, 1, 16, 16)], # 256 GPUs
}

for k in configs.keys():
    v = configs[k]
    for item in v:
        NX_ARG, NY_ARG, NTX_ARG, NTY_ARG = item[0], item[1], item[2], item[3]
        new_lines = [line.replace("NX_ARG", str(NX_ARG)).replace("NY_ARG", str(NY_ARG)).replace("NTX_ARG", str(NTX_ARG)).replace("NTY_ARG", str(NTY_ARG)) for line in lines]
        with open(f"part/bsub_stencil_part_{k}_{NX_ARG}_{NY_ARG}.lsf", "w") as fout:
            fout.writelines(new_lines)

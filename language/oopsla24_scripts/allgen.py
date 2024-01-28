import math
templatefile = "bsub_stencil_all.lsf"

template = []

with open(templatefile, "r") as fin:
    template = fin.readlines()

chapel = {
    4: [2, 2],
    8: [4, 2],
    16: [4, 4],
    32: [8, 4],
    64: [8, 8],
    128: [16, 8],
    256: [16, 16],
}

def compute_factorization(gpus):
    res = []
    factor1 = 1
    while factor1 <= gpus:
        factor2 = int(gpus / factor1)
        res.append((factor1, factor2))
        factor1 *= 2
    return res

def compute_ours(factor_x, factor_y, gpus):
    # return two integers that have the closest ratio to factor_x / factor_y, and the product is gpus
    all_factorization = compute_factorization(gpus)
    optimal_ratio = factor_x / factor_y
    minimum_diff = 1e10
    minimum_factor = (-1, -1)
    for x, y in all_factorization:
        ratio = x / y
        if abs(ratio - optimal_ratio) < minimum_diff:
            minimum_factor = (x, y)
            minimum_diff = abs(ratio - optimal_ratio)
    return minimum_factor

def gen(tile_start=1250):
    tile = tile_start
    # {domain_x}_${domain_y}_${tile}_${part_x}_${part_y}_${c_o}_${dim}
    idx = 1
    for gpus in [4, 8, 16, 32, 64, 128, 256]:
        assert gpus % 4 == 0
        nodes = int(gpus / 4)
        with open(f"all/bsub_stencil_all_{tile_start}_{nodes}.lsf", "w") as fout:
            fout.writelines(template)
            for factor_x in [1, 2, 4, 8, 16, 32, 64, 128, 256]:
                factor_y = int(256 / factor_x)
                our_x, our_y = compute_ours(factor_x, factor_y, gpus)
                chapel_x, chapel_y = chapel[gpus]
                assert chapel_x * chapel_y == our_x * our_y and our_x * our_y == gpus
                fout.write(f"run \t{factor_x}\t{factor_y}\t{tile}\t{our_x}\t{our_y}\t o \t 1 \n")
                if our_x > 2 and our_y > 1: # guarantee that 2D block and 1D block are the same
                    fout.write(f"run \t{factor_x}\t{factor_y}\t{tile}\t{our_x}\t{our_y}\t o \t 2 \n")
                fout.write(f"run \t{factor_x}\t{factor_y}\t{tile}\t{chapel_x}\t{chapel_y}\t c \t 1 \n")
        tile = math.floor(tile_start * (2 ** (0.5 * idx)))
        idx += 1

def genall():
    for tile_start in [50, 100, 250, 500, 750, 1000, 1250]:
        gen(tile_start)

if __name__ == "__main__":
    genall()

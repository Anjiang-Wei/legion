import math
templatefile = "bsub_stencil_map.lsf"

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

def gen():
    tile = 1250
    for gpus in [4, 8, 16, 32, 64, 128, 256]:
        assert gpus % 4 == 0
        nodes = int(gpus / 4)
        with open(f"map/bsub_stencil_map_{nodes}.lsf", "w") as fout:
            fout.writelines(template)
            for factor_x in [1, 2, 4, 8, 16, 32, 64, 128, 256]:
                factor_y = int(256 / factor_x)
                our_x, our_y = compute_ours(factor_x, factor_y, gpus)
                chapel_x, chapel_y = chapel[gpus]
                assert chapel_x * chapel_y == our_x * our_y and our_x * our_y == gpus
                fout.write(f"run {factor_x} {factor_y} {chapel_x} {chapel_y} {our_x} {our_y} {tile}\n")
        # tile = math.ceil(tile * math.sqrt(2))

if __name__ == "__main__":
    # for gpus in [4, 8, 16, 32, 64, 128, 256]:
    #     for factor_x in [1, 2, 4, 8, 16, 32, 64, 128, 256]:
    #         factor_y = int(256 / factor_x)
    #         print(gpus, factor_x, factor_y, compute_ours(factor_x, factor_y, gpus))
    gen()
    print("Please split the node 64 run!")

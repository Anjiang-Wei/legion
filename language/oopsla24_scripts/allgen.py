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

def gen():
    tile = 500
    # {domain_x}_${domain_y}_${tile}_${part_x}_${part_y}_${c_o}_${dim}
    for gpus in [4, 8, 16, 32, 64, 128, 256]:
        assert gpus % 4 == 0
        nodes = int(gpus / 4)
        with open(f"all/bsub_stencil_all_{nodes}.lsf", "w") as fout:
            fout.writelines(template)
            for factor_x in [1, 2, 4, 8, 16, 32, 64, 128, 256]:
                factor_y = int(256 / factor_x)
                our_x, our_y = compute_ours(factor_x, factor_y, gpus)
                chapel_x, chapel_y = chapel[gpus]
                assert chapel_x * chapel_y == our_x * our_y and our_x * our_y == gpus
                for dim in [1, 2]:
                    fout.write(f"run {factor_x} {factor_y} {tile} {our_x}    {our_y}    o  1 \n")
                    fout.write(f"run {factor_x} {factor_y} {tile} {our_x}    {our_y}    o  2 \n")
                    fout.write(f"run {factor_x} {factor_y} {tile} {chapel_x} {chapel_y} c  1 \n")
                # if our_x <= 2 or our_y == 1: # skip because 2D block and 1D block are the same
                #     continue
        tile = math.ceil(tile * math.sqrt(2))

if __name__ == "__main__":
    # for gpus in [4, 8, 16, 32, 64, 128, 256]:
    #     for factor_x in [1, 2, 4, 8, 16, 32, 64, 128, 256]:
    #         factor_y = int(256 / factor_x)
    #         print(gpus, factor_x, factor_y, compute_ours(factor_x, factor_y, gpus))
    gen()

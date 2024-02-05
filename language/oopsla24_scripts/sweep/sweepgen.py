import math
templatefile = "bsub_stencil_sweep.lsf"

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


def compute_domain(tile_current, y_over_x):
    domain_x = math.floor(math.sqrt(tile_current / y_over_x))
    domain_y = math.ceil(math.sqrt(tile_current * y_over_x))
    return domain_x, domain_y

def inter_node(partx, party):
    if partx > 2 and party > 1:
        return True
    else:
        return False

# run  node  tileidx ratioidx c_o dim  tilestart tilecurrent domain_x domain_y partx party
def gen(node, tile_idx, tile_start):
    tile_current = tile_start * node
    with open(f"swp/bsub_stencil_swp_{node}_{tile_idx}.lsf", "w") as fout:
        fout.writelines(template)
        for ratioidx, ratio in enumerate([1, 2, 4, 8, 16, 32, 64, 128, 256, 512]): # domain_y : domain_x
            domain_x, domain_y = compute_domain(tile_current, ratio)
            our_x, our_y = compute_ours(1, ratio, node * 4)
            chapel_x, chapel_y = chapel[node * 4]
            assert our_x * our_y == node * 4 and chapel_x * chapel_y == our_x * our_y
            fout.write(f"run \t{node} \t {tile_idx} \t {ratioidx} \t o \t 1 \t {tile_start} \t {tile_current} \t {domain_x} \t {domain_y} \t {our_x} \t {our_y} \n")
            if inter_node(our_x, our_y):
                fout.write(f"run \t{node}\t {tile_idx} \t {ratioidx} \t o \t 2 \t {tile_start} \t {tile_current} \t {domain_x} \t {domain_y} \t {our_x} \t {our_y} \n")
            fout.write(f"run \t{node} \t {tile_idx} \t {ratioidx} \t c \t 1 \t {tile_start} \t {tile_current} \t {domain_x} \t {domain_y} \t {chapel_x} \t {chapel_y} \n")
            if inter_node(chapel_x, chapel_y):
                fout.write(f"run \t{node} \t {tile_idx} \t {ratioidx} \t c \t 2 \t {tile_start} \t {tile_current} \t {domain_x} \t {domain_y} \t {chapel_x} \t {chapel_y} \n")


def genall():
    for node in [1, 2, 4, 8, 16, 32, 64]:
        for tileidx, tile_start in enumerate([1000 * 1000, 5000 * 5000, 10000 * 10000, \
                                            10000 * 10000 * 2, 10000 * 10000 * 3, 10000 * 10000 * 4]):
            gen(node, tileidx, tile_start)

if __name__ == "__main__":
    genall()

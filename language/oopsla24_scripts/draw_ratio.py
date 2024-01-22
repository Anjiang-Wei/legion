import pprint
import math
# node_num,domain_x,domain_y,gpu_x,gpu_y,tag,time
# 1,1,256,1,4,o,0.2542
# 1,1,256,2,2,c,0.261

ratio_file = "ratio.csv"

node_enum = [1, 2, 4, 8, 16, 32, 64]
domain_enum = [(1, 256), (2, 128), (4, 64), (8, 32), (16, 16), (32, 8), (64, 4), (128, 2), (256, 1)]

chapel_res = {
    # node_num --> {(domain_x, domain_y) --> time}
}

our_res = {
    # node_num --> {(domain_x, domain_y) --> time}
}

improve = {
    # node_num --> {(domain_x, domain_y) --> percentage improvement}
}

def update_map(my_map, k, domain_x, domain_y, time):
    if k not in my_map.keys():
        my_map[k] = {}
    my_map[k][(domain_x, domain_y)] = time

def readfile():
    with open(ratio_file, "r") as fin:
        lines = fin.readlines()[1:]
        for line in lines:
            node_num, domain_x, domain_y, gpu_x, gpu_y, tag, time  = line.split(",")
            node_num, domain_x, domain_y, gpu_x, gpu_y, tag, time = int(node_num), int(domain_x), int(domain_y), int(gpu_x), int(gpu_y), tag, float(time)
            if tag == "c":
                update_map(chapel_res, node_num, domain_x, domain_y, time)
            elif tag == "o":
                update_map(our_res, node_num, domain_x, domain_y, time)
            else:
                update_map(chapel_res, node_num, domain_x, domain_y, time)
                update_map(our_res, node_num, domain_x, domain_y, time)
    # pprint.pprint(chapel_res)

def compute_improve():
    for node_num in node_enum:
        for domain_x, domain_y in domain_enum:
            diff = chapel_res[node_num][(domain_x, domain_y)] - our_res[node_num][(domain_x, domain_y)]
            if diff < 0 and abs(diff <= 0.001): # floating point error
                diff = 0
            assert diff >= 0, f"{diff}, {node_num}, {domain_x}, {domain_y}"
            diff_perc = diff / chapel_res[node_num][(domain_x, domain_y)] * 100
            update_map(improve, node_num, domain_x, domain_y, diff_perc)
    pprint.pprint(improve)

def report_avg_imp():
    for node_num in node_enum:
        total_imp = 1
        for domain_x, domain_y in domain_enum:
            imp = improve[node_num][(domain_x, domain_y)]
            total_imp *= 1 + imp / 100
        print(f"{node_num}: {total_imp**(1.0 / len(domain_enum))}")

if __name__ == "__main__":
    readfile()
    compute_improve()
    report_avg_imp()

Task * GPU,CPU; # for any task, run on GPU if supported

Region * * GPU FBMEM; # for any task, any region, if mapped onto GPU, use FBMEM as default
Region * * CPU SYSMEM; # if mapped onto CPU, use SYSMEM as default
# Region * rp_all_shared_p GPU ZCMEM;
# Region * rp_all_ghost_p GPU ZCMEM;

# for $task, $region, $mem_type, specify $list_of_layout_constraints
Layout * * * SOA C_order; # Other choices: AOS F_order Exact Align==128 Compact

# mcpu = Machine(CPU);
# mgpu = Machine(GPU);

# def linearcyclic(Task task) {
#     return mgpu[task.ipoint[0] % mgpu.size[0], task.ipoint[0] / mgpu.size[0]];
# }

# def cyclic_shard_cpu(Task task) {
#     return mcpu[task.ipoint[0] % mgpu.size[0], 0]; # only one CPU
# }

# IndexTaskMap GPU linearcyclic;
# IndexTaskMap CPU cyclic_shard_cpu;
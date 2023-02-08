Task * GPU,CPU; # for any task, run on GPU if supported

Region * * GPU FBMEM; # for any task, any region, if mapped onto GPU, use FBMEM as default
Region * * CPU SYSMEM; # if mapped onto CPU, use SYSMEM as default

# for $task, $region, if mapped onto $proc_type, specify $mem_type to use
# https://github.com/Anjiang-Wei/legion/blob/circuit_example/language/examples/circuit_bishop.rg#L24-L62
# for region names, we need to use the region names that are passed in (instead of what is defined in task)
Region * rp_shared GPU ZCMEM;
Region * rp_ghost GPU ZCMEM; # if the task is not mapped to GPU, it won't take effect

# for $task, $region, $mem_type, specify $list_of_layout_constraints
Layout * * * SOA C_order; # Other choices: AOS F_order Exact Align==128 Compact

mcpu = Machine(CPU); # nodes * processors

mgpu = Machine(GPU); # nodes * processors

def linearblock(Task task) {
    # task.ispace is a n-dim tuple (in this case n=1) indicating launch domain, not used here
    return mgpu[task.ipoint[0] / mgpu.size[1], task.ipoint[0] % mgpu.size[1]]; # return one point in a machine model (or generally, can be a subset of points on the same node)
}

def block_shard_cpu(Task task) {
    return mcpu[task.ipoint[0] / mcpu.size[1], 0]; # only one CPU per node
}

# specify $task_name(s) and sharding+slicing function
IndexTaskMap calculate_new_currents,distribute_charge,update_voltages linearblock;
IndexTaskMap init_piece,init_pointers,print_summary block_shard_cpu;

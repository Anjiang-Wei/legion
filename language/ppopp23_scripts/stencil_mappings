Task * GPU,CPU; # for any task, run on GPU if supported

Region * * GPU FBMEM; # for any task, any region, if mapped onto GPU, use FBMEM as default
Region * * CPU SYSMEM; # if mapped onto CPU, use SYSMEM as default

Layout * * * SOA F_order;

mcpu = Machine(CPU);
mgpu = Machine(GPU);

def linearblock(Task task) {
    return mgpu[task.ipoint[0] / mgpu.size[1], task.ipoint[0] % mgpu.size[1]];
}

def block_shard_cpu(Task task) {
    return mcpu[task.ipoint[0] / mgpu.size[1], 0]; # only one CPU
}

IndexTaskMap stencil,increment linearblock;
IndexTaskMap check,print_time block_shard_cpu;

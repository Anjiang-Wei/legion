import sys

def replace(lines, pre, post):
    res = []
    for line in lines:
        if "return this;" in line:
            res.append(line.replace("return this;", "return getptr();"))
            continue
        if "{func_name, this}" in line: # fix a special case in funcdefnode
            res.append(line.replace("{func_name, this}", "{func_name, getptr()}"))
            continue
        if line.startswith(pre):
            res.append(line.replace(pre, post))
            continue
        res.append(line.replace(" "+pre, " "+post).replace("<"+pre, "<"+post).replace("("+pre, "("+post))
    return res

def detect_files(fname_list):
    res = []
    for f in fname_list:
        if f != "parser.y":
            with open(f, "r") as fin:
                lines = fin.readlines()
                res += detect(lines)
    return sorted(list(set(res)))

def detect(lines):
    # [...]MSpace*, [...]Node*
    res = ["MSpaceOp", "Node"]
    init = ["MSpace", "Node"]
    for line in lines:
        if "class" in line:
            if init[0] in line or init[1] in line:
                words = line.split()
                for word in words:
                    if init[0] in word or init[1] in word:
                        word = word.strip()
                        word = word.strip(";") # avoid class MSpace;
                        word = word.strip(":") # avoid MSpace1: MSpaceBase
                        if "shared" not in word:
                            res.append(word)
    return list(set(res))



def process(fname, all_nodes):
    res = []
    with open(fname, "r") as fin:
        res = fin.readlines()
        for node in all_nodes:
            res = replace(res, node+"*", "std::shared_ptr<" + node+">")
            res = replace(res, "new " + node, "std::make_shared<" + node + ">")
    with open(fname, "w") as fout:
        fout.writelines(res)


if __name__ == "__main__":
    fname_list = []
    for i in range(1, len(sys.argv)):
        fname = sys.argv[i]
        fname_list.append(fname)
    all_nodes = detect_files(fname_list)
    print(fname_list)
    print(all_nodes)
    print(len(all_nodes))
    for i in range(len(fname_list)):
        process(fname_list[i], all_nodes)


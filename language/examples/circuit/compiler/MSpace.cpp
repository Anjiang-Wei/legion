#ifndef __MSPACE
#define __MSPACE

#include "tree.hpp"
#include "myalgorithm.h"

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <iterator>
#include <vector>
#include <string>
#include <string.h>
#include <unordered_map>
#include <stack>
#include <assert.h>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

// #define DEBUG_MSPACE true

std::string vec2str(std::vector<int> my_vector)
{
    std::stringstream result;
    std::copy(my_vector.begin(), my_vector.end(), std::ostream_iterator<int>(result, " "));
    return result.str();
}


class MSpaceOp //: public std::enable_shared_from_this<MSpaceOp>
{
public:
    APIEnum trans_op;
    virtual ~MSpaceOp() {}
    virtual void print() {};
    virtual std::vector<int> trans_dim(const std::vector<int>&) 
    {
        std::cout << "trans_dim method TBD:" << APIName[this->trans_op] << std::endl;
        assert(false);
        return std::vector<int>{};
    }
    virtual std::vector<int> trans(const std::vector<int>&)
    {
        std::cout << "trans method TBD:" << APIName[this->trans_op] << std::endl;
        assert(false);
        return std::vector<int>{};
    }
    virtual std::vector<int> trans_forward(const std::vector<int>&)
    {
        printf("trans_forward method TBD: %s\n", APIName[this->trans_op]);
        assert(false);
        return std::vector<int>{};
    }
    /*
    virtual std::shared_ptr<MSpaceOp> getptr()
    {
        return shared_from_this();
    }*/
};

class SplitMSpace : public MSpaceOp
{
public:
    int split_dim;
    int split_factor;
    SplitMSpace() {}
    SplitMSpace(int split_dim_, int split_factor_)
    {
        trans_op = SPLIT;
        split_dim = split_dim_;
        split_factor = split_factor_;
    }
    std::vector<int> trans(const std::vector<int>& old_point)
    {
        // shrink the dimension of point, closer to original machine model
        std::vector<int> result;
        for (int i = 0; i < split_dim; i++)
        {
            result.push_back(old_point[i]);
        }
        // result.push_back(old_point[split_dim] * split_factor + old_point[split_dim + 1]);
        result.push_back(old_point[split_dim] + old_point[split_dim + 1] * split_factor);
        for (int i = split_dim + 2; i < (int) old_point.size(); i++)
        {
            result.push_back(old_point[i]);
        }
        return result;
    }

    std::vector<int> trans_forward(const std::vector<int>& old_point)
    {
        // add 1 dimension to split the point
        std::vector<int> result;
        for (int i = 0; i < split_dim; i++)
        {
            result.push_back(old_point[i]);
        }
        result.push_back(old_point[split_dim] % split_factor);
        result.push_back(old_point[split_dim] / split_factor);
        for (int i = split_dim + 1; i < (int) old_point.size(); i++)
        {
            result.push_back(old_point[i]);
        }
        return result;
    }

    std::vector<int> trans_dim(const std::vector<int>& old_dim)
    {
        // the new machine model has 1 more dim
        std::vector<int> result;
        for (int i = 0; i < split_dim; i++)
        {
            result.push_back(old_dim[i]);
        }
        // result.push_back(old_dim[split_dim] / split_factor);
        // result.push_back(split_factor);
        result.push_back(split_factor);
        result.push_back(old_dim[split_dim] / split_factor);
        for (size_t i = split_dim + 1; i < old_dim.size(); i++)
        {
            result.push_back(old_dim[i]);
        }
        return result;
    }
};

class MergeMSpace : public MSpaceOp
{
public:
    int dim1;
    int dim2;
    int dim2_volume;
    MergeMSpace() {}
    MergeMSpace(int dim1_, int dim2_)
    {
        trans_op = MERGE;
        dim1 = dim1_;
        dim2 = dim2_;
    }

    std::vector<int> trans(const std::vector<int>& old_point)
    {
        // turn the point back to original machine model via splitting old_point[dim1] by dim2_volume
        std::vector<int> result;
        for (size_t i = 0; i < old_point.size(); i++)
        {
            if ((int) i == dim1)
            {
                result.push_back(old_point[dim1] / dim2_volume);
            }
            else
            {
                result.push_back(old_point[i]);
            }
        }
        result.insert(result.begin() + dim2, old_point[dim1] % dim2_volume);
        if (result.size() != old_point.size() + 1)
        {
            std::cout << "MergeMSpace trans fails" << std::endl;
            assert(false);
        }
        return result;
    }

    std::vector<int> trans_forward(const std::vector<int>& old_point)
    {
        // merge the old_point[dim1] and old_point[dim2] dimension into dim1
        std::vector<int> result;
        for (size_t i = 0; i < old_point.size(); i++)
        {
            if ((int) i == dim1)
            {
                result.push_back(old_point[dim1] * dim2_volume + old_point[dim2]);
            }
            else if ((int) i == dim2)
            {
               continue;
            }
            else
            {
                result.push_back(old_point[i]);
            }
        }
        assert(result.size() == old_point.size() - 1);
        return result;
    }

    std::vector<int> trans_dim(const std::vector<int>& old_dim)
    {
        // merge dim1 and dim2 into dim1
        std::vector<int> result;
        for (size_t i = 0; i < old_dim.size(); i++)
        {
            if ((int) i == dim1)
            {
                result.push_back(old_dim[dim1] * old_dim[dim2]);
            }
            else if ((int) i == dim2)
            {
                dim2_volume = old_dim[dim2];
            }
            else
            {
                result.push_back(old_dim[i]);
            }
        }
        assert(result.size() == old_dim.size() - 1);
        return result;
    }
};

class SwapMSpace : public MSpaceOp
{
public:
    int dim1;
    int dim2;
    SwapMSpace() {}
    SwapMSpace(int dim1_, int dim2_)
    {
        trans_op = SWAP;
        dim1 = dim1_;
        dim2 = dim2_;
    }

    std::vector<int> trans(const std::vector<int>& old_point)
    {
        std::vector<int> result(old_point);
        size_t x = result[dim1];
        result[dim1] = result[dim2];
        result[dim2] = x;
        return result;
    }

    std::vector<int> trans_forward(const std::vector<int>& old_point)
    {
        std::vector<int> result(old_point);
        size_t x = result[dim1];
        result[dim1] = result[dim2];
        result[dim2] = x;
        return result;
    }

    std::vector<int> trans_dim(const std::vector<int>& old_dim)
    {
        std::vector<int> result(old_dim);
        size_t x = result[dim1];
        result[dim1] = result[dim2];
        result[dim2] = x;
        return result;
    }
};

class SliceMSpace : public MSpaceOp
{
public:
    int dim;
    int low, high;
    SliceMSpace() {}
    SliceMSpace(int dim_, int low_, int high_)
    {
        trans_op = SLICE;
        dim = dim_;
        low = low_;
        high = high_;
    }

    std::vector<int> trans(const std::vector<int>& old_point)
    {
        std::vector<int> result(old_point);
        assert(old_point[dim] <= (high-low));
        result[dim] = result[dim] + low;
        return result;
    }

    std::vector<int> trans_forward(const std::vector<int>& old_point)
    {
        std::vector<int> result(old_point);
        if (old_point[dim] < low)
        {
            printf("old_point[%d]=%d < %d, not existing in the machine model using SliceMSpace\n",
                                        dim, old_point[dim], low);
            assert(false);
        }
        result[dim] = result[dim] - low;
        return result;
    }

    std::vector<int> trans_dim(const std::vector<int>& old_dim)
    {
        std::vector<int> result(old_dim);
        result[dim] = high - low + 1;
        return result;
    }
};

class ReverseMSpace : public MSpaceOp
{
public:
    int dim;
    int dim_volume;
    ReverseMSpace() {}
    ReverseMSpace(int dim_)
    {
        trans_op = REVERSE;
        dim = dim_;
    }

    std::vector<int> trans(const std::vector<int>& old_point)
    {
        std::vector<int> result(old_point);
        result[dim] = dim_volume - 1 - result[dim];
        return result;
    }

    std::vector<int> trans_forward(const std::vector<int>& old_point)
    {
        std::vector<int> result(old_point);
        result[dim] = dim_volume - 1 - result[dim];
        return result;
    }

    std::vector<int> trans_dim(const std::vector<int>& old_dim)
    {
        dim_volume = old_dim[dim];
        return old_dim;
    }
};

class BalSplitMSpace : public MSpaceOp
{
public:
    int dim;
    int added_dim_num;
    std::vector<int> factor_result;
    std::vector<int> new_dims;
    BalSplitMSpace() {}
    BalSplitMSpace(int dim_, int added_dim_num_)
    {
        trans_op = BALANCE_SPLIT;
        dim = dim_;
        added_dim_num = added_dim_num_;
        if (added_dim_num != 2 && added_dim_num != 3)
        {
            std::cout << "Unsupported Balanced Split Dimension: " << added_dim_num << std::endl;
        }
    }
    std::vector<int> trans(const std::vector<int>& old_point)
    {
        // back to a point in original machine model via merging by factor of new_dims[]
        std::vector<int> result;
        for (size_t i = 0; i < old_point.size(); i++)
        {
            if ((int) i != dim)
            {
                result.push_back(old_point[i]);
            }
            else
            {
                if (added_dim_num == 2)
                {
                    // result.push_back(old_point[i] * new_dims[1] + old_point[i+1]);
                    result.push_back(old_point[i] + new_dims[0] * old_point[i+1]);
                    i += 1;
                }
                else if (added_dim_num == 3)
                {
                    // result.push_back(old_point[i] * new_dims[1] * new_dims[2]
                    //                 + old_point[i+1] * new_dims[2]
                    //                 + old_point[i+2]);
                    result.push_back(old_point[i]
                                    + old_point[i+1] * new_dims[0]
                                    + old_point[i+2] * new_dims[0] * new_dims[1]);
                    i += 2;
                }
            }
        }
        return result;
    }

    std::vector<int> trans_forward(const std::vector<int>& old_point)
    {
        // add added_dim_num (new_dims[]) dimension to split the old_point[dim]
        std::vector<int> result;
        for (size_t i = 0; i < old_point.size(); i++)
        {
            if ((int) i != dim)
            {
                result.push_back(old_point[i]);
            }
            else
            {
                if (added_dim_num == 2)
                {
                    result.push_back(old_point[i] % new_dims[0]);
                    result.push_back(old_point[i] / new_dims[0]);
                }
                else if (added_dim_num == 3)
                {
                    result.push_back(old_point[i] % new_dims[0]);
                    result.push_back((old_point[i] / new_dims[0]) % new_dims[1]);
                    result.push_back((old_point[i] / new_dims[0]) / new_dims[1]);
                }
            }
        }
        return result;
    }


    static void generate_prime_factor(int big_number, std::vector<int>& factors)
    {
        auto generate_factors = [&](int factor)
        {
            while (big_number % factor == 0)
            {
                factors.push_back(factor);
                big_number /= factor;
            }
        };
        generate_factors(2);
        generate_factors(3);
        generate_factors(5);
        generate_factors(7);
        generate_factors(11);
        generate_factors(13);
        generate_factors(17);
    }
    std::vector<int> trans_dim(const std::vector<int>& old_dim)
    {
        // replace old_dim[dim] with new_dims
        generate_prime_factor(old_dim[dim], factor_result);
        auto factor_it = factor_result.begin();
        new_dims.resize(added_dim_num, 1);
        int original_size = old_dim[dim];

        while (original_size > 1)
        {
            auto min_it = std::min_element(new_dims.begin(), new_dims.end());
            auto factor = *factor_it++;
            (*min_it) *= factor;
            original_size /= factor;
        }
        std::sort(new_dims.begin(), new_dims.end());
        std::vector<int> result;
        for (size_t i = 0; i < old_dim.size(); i++)
        {
            if ((int) i != dim)
            {
                result.push_back(old_dim[i]);
            }
            else
            {
                for (int j = 0; j < added_dim_num; j++)
                {
                    result.push_back(new_dims[j]);
                }
            }
        }
        return result;
    }
};

class AutoSplitMSpace : public MSpaceOp
{
public:
    int dim; // the dimension of the machine model to be splitted
    std::vector<int> input_dim; // launch domain
    std::vector<int> factor_result; // the factorization of workload constant
    std::vector<int> new_dims; // dimension sizes for the newly-splitted machine model
    std::vector<int> newdim_preprod; // 1, new_dims[0], new_dims[0] * new_dims[1], ...
    AutoSplitMSpace() {}
    AutoSplitMSpace(int dim_, std::vector<int> input_dim_)
    {
        trans_op = AUTO_SPLIT;
        dim = dim_; input_dim = input_dim_;
    }

    std::vector<int> trans(const std::vector<int>& old_point)
    {
        // turning old_point back to a point in original machine model via merging by newdim_preprod[]
        std::vector<int> result;
        for (size_t i = 0; i < old_point.size(); i++)
        {
            if ((int) i != dim)
            {
                result.push_back(old_point[i]);
            }
            else
            {
                int added_dim_num = new_dims.size();
                int new_result = 0;
                for (int kk = 0; kk < added_dim_num; kk++)
                {
                    new_result += newdim_preprod[kk] * old_point[i+kk];
                }
                result.push_back(new_result);
                i += (added_dim_num - 1);
            }
        }
        return result;
    }

    std::vector<int> trans_forward(const std::vector<int>& old_point)
    {
        // split the old_point[dim] into new_dims[]
        std::vector<int> result;
        for (size_t i = 0; i < old_point.size(); i++)
        {
            if ((int) i != dim)
            {
                result.push_back(old_point[i]);
            }
            else
            {
                int point_i = old_point[i];
                for (int kk = 0; kk < new_dims.size(); kk++)
                {
                   result.push_back(point_i % new_dims[kk]);
                   point_i = point_i / new_dims[kk];
                }
            }
        }
        return result;
    }

    inline void compute_preprod()
    {
        newdim_preprod.push_back(1);
        int prior = 1;
        for (int i = 0; i < new_dims.size(); i++)
        {
            prior = new_dims[i] * prior;
            newdim_preprod.push_back(prior);
        }
    }
    std::vector<int> trans_dim(const std::vector<int>& old_dim)
    {
        // replace old_dim[dim] with new_dims[]
        new_dims = precise_enumerate(old_dim[dim], input_dim);
        assert(new_dims.size() == input_dim.size());
        compute_preprod();
        std::vector<int> result;
        for (size_t i = 0; i < old_dim.size(); i++)
        {
            if ((int) i != dim)
            {
                result.push_back(old_dim[i]);
            }
            else
            {
                for (int j = 0; j < new_dims.size(); j++)
                {
                    result.push_back(new_dims[j]);
                }
            }
        }
        return result;
    }
};

class GreedySplitMSpace : public AutoSplitMSpace
{
public:
    GreedySplitMSpace() {}
    GreedySplitMSpace(int dim_, std::vector<int> input_dim_)
    {
        trans_op = GREEDY_SPLIT;
        dim = dim_; input_dim = input_dim_;
    }
    std::vector<int> trans_dim(const std::vector<int>& old_dim)
    {
        new_dims = greedy(old_dim[dim], input_dim);
        assert(new_dims.size() == input_dim.size());
        compute_preprod();
        std::vector<int> result;
        for (size_t i = 0; i < old_dim.size(); i++)
        {
            if ((int) i != dim)
            {
                result.push_back(old_dim[i]);
            }
            else
            {
                for (int j = 0; j < new_dims.size(); j++)
                {
                    result.push_back(new_dims[j]);
                }
            }
        }
        return result;
    }
};

class MSpace : public ExprNode
{
public:
    ProcessorEnum proc_type;
    // size_t num_nodes;
    // size_t num_processors;
    std::vector<int> each_dim;
    MSpaceOp* trans_op;
    MSpace* prev_machine;
    std::vector<Processor> local_procs;
    ~MSpace()
    {
        if (trans_op != NULL)
        {
            delete trans_op;
        }
        // deleting prev_machine already handled by tree.cpp
    }

    MSpace()
    {
        type = MSpaceType;
        prev_machine = NULL;
        trans_op = NULL;
    }
    void set_proc_type(ProcessorEnum proc_type_)
    {
        extern Processor::Kind MyProc2LegionProc(ProcessorEnum);
        proc_type = proc_type_;
        const Machine machine = Machine::get_machine();
        size_t node_num = machine.get_address_space_count();
        Machine::ProcessorQuery all_procs(machine);
        all_procs.only_kind(MyProc2LegionProc(proc_type_));
        all_procs.local_address_space();
        std::vector<Processor> procs(all_procs.begin(), all_procs.end());
        local_procs = procs;
        each_dim = std::vector<int>{(int) node_num, (int) procs.size()};
    }
    int get_volume()
    {
        int res = 1;
        for (size_t i = 0; i < each_dim.size(); i++)
        {
            res *= each_dim[i];
        }
        return res;
    }
    std::vector<int> get_size()
    {
        return each_dim;
    }
    void print()
    {
        printf("Processor %s: %s\n", ProcessorEnumName[proc_type], vec2str(each_dim).c_str());
    }
    Node* run()
    {
        return this;
    }
    bool has_mem(int index, MemoryEnum mem)
    {
        extern Memory::Kind MyMem2LegionMem(MemoryEnum);
        const Machine machine = Machine::get_machine();
        Machine::MemoryQuery visible_memories(machine);
        visible_memories.has_affinity_to(local_procs[index]);
        visible_memories.only_kind(MyMem2LegionMem(mem));
        if (visible_memories.count() > 0)
        {
            auto target_memory = visible_memories.first();
            if (target_memory.exists())
            {
                return true;
            }
        }
        return false;
    }
    MSpace(MSpace* old, APIEnum api, int int1)
    {
        type = MSpaceType;
        proc_type = old->proc_type;
        prev_machine = old;
        if (api == REVERSE)
        {
            // printf("reverse %d\n", int1);
            if (int1 >= (int) old->each_dim.size())
            {
                std::cout << "Reverse's arguments have exceeded the allowed dimensions: " << old->each_dim.size()-1 << std::endl;
                assert(false);
            }
            trans_op = new ReverseMSpace(int1);
        }
        else
        {
            printf("Unsupported API\n");
            assert(false);
        }
        each_dim = trans_op->trans_dim(old->each_dim);
    }
    MSpace(MSpace* old, APIEnum api, int int1_, int int2_)
    {
        size_t int1 = (size_t) int1_;
        size_t int2 = (size_t) int2_;
        type = MSpaceType;
        proc_type = old->proc_type;
        prev_machine = old;
        if (api == SPLIT)
        {
            // printf("split %d %d\n", int1, int2);
            if (old->each_dim[int1] % int2 != 0)
            {
                std::cout << old->each_dim[int1] << " must be divisible by " << int2 << std::endl;
            }
            trans_op = new SplitMSpace(int1, int2);
        }
        else if (api == SWAP)
        {
            // printf("swap %d %d\n", int1, int2);
            if (int1 == int2)
            {
                std::cout << "Swap's arguments should be different integers" << std::endl;
                assert(false);
            }
            if (int1 >= old->each_dim.size() || int2 >= old->each_dim.size())
            {
                std::cout << "Swap's arguments have exceeded the allowed dimensions: " << old->each_dim.size()-1 << std::endl;
                assert(false);
            }
            trans_op = new SwapMSpace(int1, int2);
        }
        else if (api == MERGE)
        {
            // printf("merge %d %d\n", int1, int2);
            if (int1 == int2)
            {
                std::cout << "Merge's arguments should be different integers" << std::endl;
                assert(false);
            }
            if (int1 >= old->each_dim.size() || int2 >= old->each_dim.size())
            {
                std::cout << "Merge's arguments have exceeded the allowed dimensions: " << old->each_dim.size()-1 << std::endl;
                assert(false);
            }
            trans_op = new MergeMSpace(int1, int2);
        }
        else if (api == BALANCE_SPLIT)
        {
            // printf("balance_split %d %d\n", int1, int2);
            if (int1 >= old->each_dim.size())
            {
                std::cout << "balance_split's arguments have exceeded the allowed dimensions: " << old->each_dim.size()-1 << std::endl;
                assert(false);
            }
            trans_op = new BalSplitMSpace(int1, int2);
        }
        else
        {
            printf("Unsupported API\n");
            assert(false);
        }
        each_dim = trans_op->trans_dim(old->each_dim);
    }
    MSpace(MSpace* old, APIEnum api, int int1, const std::vector<int>& tupleint2)
    {
        type = MSpaceType;
        proc_type = old->proc_type;
        prev_machine = old;
        if (api == AUTO_SPLIT)
        {
            trans_op = new AutoSplitMSpace(int1, tupleint2);
        }
        else if (api == GREEDY_SPLIT)
        {
            trans_op = new GreedySplitMSpace(int1, tupleint2);
        }
        else
        {
            printf("Unsupported API signature, which should be used in auto_split or greedy_split\n");
            assert(false);
        }
        each_dim = trans_op->trans_dim(old->each_dim);
    }
    MSpace(MSpace* old, APIEnum api, int int1, int int2, int int3)
    {
        type = MSpaceType;
        proc_type = old->proc_type;
        prev_machine = old;
        if (api == SLICE)
        {
            // printf("slice %d %d %d\n", int1, int2, int3);
            if (int1 >= (int) old->each_dim.size())
            {
                std::cout << "Slice's arguments have exceeded the allowed dimensions: " << old->each_dim.size()-1 << std::endl;
                assert(false);
            }
            if (int2 >= old->each_dim[int1] || int3 >= old->each_dim[int1])
            {
                std::cout << "Slice out of bound: " << old->each_dim[int1] << std::endl;
                assert(false);
            }
            trans_op = new SliceMSpace(int1, int2, int3);
        }
        else
        {
            printf("Unsupported API\n");
            assert(false);
        }
        each_dim = trans_op->trans_dim(old->each_dim);
    }

    inline void all_append(std::vector<std::vector<int>>& points, int number, bool range)
    {
        int points_size = points.size();
        for (int i = 0; i < points_size; i++)
        {
            if (range)
            {
                for (int j = 0; j < number; j++)
                {
                    std::vector<int> new_point = points[i];
                    new_point.push_back(j);
                    points.push_back(new_point);
                    if (j == number - 1) // last iteration
                    {
                        points.erase(points.begin() + i);
                    }
                }
            }
            else
            {
                points[i].push_back(number);
            }
        }
    }

    bool get_node_proc(const std::vector<int>& machine_point,
                       std::vector<int>& result1, std::vector<std::vector<int>>& result2)
    {
        // can deal with set of points, e.g., m[0, *]
        bool is_one_point = true;
        std::vector<int> one_point;
        std::vector<std::vector<int>> set_points;
        for (int i = 0; i < machine_point.size(); i++)
        {
            if (machine_point[i] >= 0)
            {
                if (is_one_point)
                {
                    one_point.push_back(machine_point[i]);
                }
                else
                {
                    all_append(set_points, machine_point[i], false);
                }
            }
            else
            {
                assert(machine_point[i] == -1); // representing *
                if (is_one_point)
                {
                    set_points.push_back(one_point);
                }
                is_one_point = false;
                all_append(set_points, this->each_dim[i], true);
            }
        }
        if (is_one_point)
        {
            result1 = get_node_proc(one_point);
        }
        else
        {
            for (auto each_point: set_points)
            {
                result2.push_back(get_node_proc(each_point));
            }
        }
        return is_one_point;
    }
    
    std::vector<int> get_node_proc(const std::vector<int>& machine_point)
    {
        MSpace* mspace = this;
        std::vector<int> current_point = machine_point;
        #ifdef DEBUG_MSPACE
            std::cout << "start get_node_proc: " << vec2str(current_point) << std::endl;
        #endif
        while(mspace->prev_machine != NULL)
        {
            current_point = mspace->trans_op->trans(current_point);
            #ifdef DEBUG_MSPACE
                std::cout << vec2str(current_point) << ", ";
            #endif
            mspace = mspace->prev_machine;
        }
        #ifdef DEBUG_MSPACE
            std::cout << "end get_node_proc" << std::endl;
        #endif
        assert(mspace->each_dim.size() == 2);
        assert(current_point.size() == 2);
        if (current_point[0] >= mspace->each_dim[0])
        {
            std::cout << "Sharding node index out of bound!" << std::endl;
            std::cout << current_point[0]  << " >= " << mspace->each_dim[0] << std::endl;
            assert(false);
        }
        if (current_point[1] >= mspace->each_dim[1])
        {
            std::cout << "Slice processor index out of bound!" << std::endl;
            std::cout << current_point[1]  << " >= " << mspace->each_dim[1] << std::endl;
            assert(false);
        }
        return current_point;
    }

    std::vector<int> legion2mspace(const std::vector<int>& dim2_point)
    {
        // assert(dim2_point.size() == 2); // always true
        #ifdef DEBUG_MSPACE
            std::cout << "start legion2mspace: " << vec2str(dim2_point) << std::endl;
        #endif
        MSpace* mspace = this;
        std::vector<int> current_point = dim2_point;
        std::stack<MSpaceOp*> opstack;
        while (mspace->prev_machine != NULL)
        {
            opstack.push(mspace->trans_op);
            mspace = mspace->prev_machine;
        }
        while (!opstack.empty())
        {
            MSpaceOp* op = opstack.top();
            current_point = op->trans_forward(current_point);
            #ifdef DEBUG_MSPACE
               std::cout << vec2str(current_point) << ", ";
            #endif
            opstack.pop();
        }
        assert(this->each_dim.size() == current_point.size());
        for (int i = 0; i < current_point.size(); i++)
        {
            if (current_point[i] >= this->each_dim[i])
            {
                printf("%d >= %d in legion2mspace: index out of bound!\n");
                assert(false);
            }
        }
        return current_point;
    }

};


#endif

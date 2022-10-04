/* Copyright 2022 Stanford University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define USE_LOGGING_MAPPER

#include "dsl_mapper.h"

#include "mappers/logging_wrapper.h"
#include "mappers/default_mapper.h"

#include "compiler/y.tab.c"
#include "compiler/lex.yy.c"

#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <chrono>

using namespace Legion;
using namespace Legion::Mapping;

static Logger log_mapper("nsmapper");
legion_equality_kind_t myop2legion(BinOpEnum myop);

namespace Legion {
  namespace Internal {
    /**
     * \class UserShardingFunctor
     * The cyclic sharding functor just round-robins the points
     * onto the available set of shards
     */
    class UserShardingFunctor : public ShardingFunctor {
    private:
      std::string taskname;
    public:
      UserShardingFunctor(std::string mode);
      UserShardingFunctor(const UserShardingFunctor &rhs);
      virtual ~UserShardingFunctor(void);
    public:
      UserShardingFunctor& operator=(const UserShardingFunctor &rhs);
    public:
      virtual ShardID shard(const DomainPoint &point,
                            const Domain &full_space,
                            const size_t total_shards);
    };
  }
}

class NSMapper : public DefaultMapper
{
public:
  NSMapper(MapperRuntime *rt, Machine machine, Processor local, const char *mapper_name);

public:
  static std::string get_policy_file();
  void parse_policy_file(const std::string &policy_file);
  static void register_user_sharding_functors(Runtime *runtime);
private:
  Processor select_initial_processor_by_kind(const Task &task, Processor::Kind kind);
  bool validate_processor_mapping(MapperContext ctx, const Task &task, Processor proc, bool strict=true);
  template <typename Handle>
  void maybe_append_handle_name(const MapperContext ctx,
                                const Handle &handle,
                                std::vector<std::string> &names);
  void get_handle_names(const MapperContext ctx,
                        const RegionRequirement &req,
                        std::vector<std::string> &names);
  // InFlightTask represents a task currently being executed.
  struct InFlightTask {
    // Unique identifier of the task instance.
    std::pair<Legion::Domain, size_t> id; // for index launch
    Legion::UniqueID id2; // for non-index launch task
    // An event that will be triggered when the task finishes.
    Legion::Mapping::MapperEvent event;
    // A clock measurement from when the task was scheduled.
    std::chrono::high_resolution_clock::time_point schedTime;
  };
  // backPressureQueue maintains state for each processor about how many
  // tasks that are marked to be backpressured are executing on the processor.
  std::map<Legion::Processor, std::deque<InFlightTask>> backPressureQueue;

public:
  virtual Processor default_policy_select_initial_processor(MapperContext ctx,
                                                            const Task &task);
  virtual void default_policy_select_target_processors(MapperContext ctx,
                                                       const Task &task,
                                                       std::vector<Processor> &target_procs);
  virtual LogicalRegion default_policy_select_instance_region(MapperContext ctx,
                                                              Memory target_memory,
                                                              const RegionRequirement &req,
                                                              const LayoutConstraintSet &constraints,
                                                              bool force_new_instances,
                                                              bool meets_constraints);
  virtual void map_task(const MapperContext ctx,
                        const Task &task,
                        const MapTaskInput &input,
                        MapTaskOutput &output);
  virtual void select_sharding_functor(
                                const MapperContext                ctx,
                                const Task&                        task,
                                const SelectShardingFunctorInput&  input,
                                      SelectShardingFunctorOutput& output);
  virtual void slice_task(const MapperContext      ctx,
                          const Task&              task,
                          const SliceTaskInput&    input,
                          SliceTaskOutput&   output) override;
  virtual void select_task_options(const MapperContext    ctx,
                                  const Task&            task,
                                        TaskOptions&     output);
  void custom_policy_select_constraints(const Task &task,
                                        const std::vector<std::string> &region_names,
                                        const Legion::Mapping::MapperContext ctx,
                                        Legion::LayoutConstraintSet &constraints,
                                        const Legion::Memory &target_memory,
                                        const Legion::RegionRequirement &req);
  MapperSyncModel get_mapper_sync_model() const override;
  void report_profiling(const Legion::Mapping::MapperContext ctx,
                        const Legion::Task& task,
                        const TaskProfilingInfo& input) override;

  void select_tasks_to_map(const Legion::Mapping::MapperContext ctx,
                           const SelectMappingInput& input,
                                 SelectMappingOutput& output) override;
  int get_proc_idx(const Processor proc) const;
  Processor idx_to_proc(int proc_idx, const Processor::Kind proc_kind) const;

protected:
  void map_task_post_function(const MapperContext   &ctx,
                              const Task            &task,
                              const std::string     &task_name,
                              const Processor::Kind &proc_kind,
                              MapTaskOutput         &output);
  Memory query_best_memory_for_proc(const Processor& proc,
                                    const Memory::Kind& mem_target_kind);
  void custom_slice_task(const Task &task,
                          const std::vector<Processor> &local_procs,
                          const std::vector<Processor> &remote_procs,
                          const SliceTaskInput &input,
                               SliceTaskOutput &output) const;
  template<int DIM>
  static void custom_decompose_points(
                          std::vector<int> &index_launch_space,
                          const DomainT<DIM,coord_t> &point_space,
                          const std::vector<Processor> &targets,
                          bool recurse, bool stealable,
                          std::vector<TaskSlice> &slices, std::string taskname);

private:
  std::unordered_map<TaskID, Processor::Kind> cached_task_policies;

  std::unordered_set<std::string> has_region_policy;
  using HashFn2 = PairHash<TaskID, uint32_t>;
  std::unordered_map<std::pair<TaskID, uint32_t>, Memory::Kind, HashFn2> cached_region_policies;
  std::unordered_map<std::pair<TaskID, uint32_t>, std::string, HashFn2> cached_region_names;
  std::unordered_map<std::pair<TaskID, uint32_t>, LayoutConstraintSet, HashFn2> cached_region_layout;
  std::map<std::pair<Legion::Processor, Memory::Kind>, Legion::Memory> cached_affinity_proc2mem;

public:
  static Tree2Legion tree_result;
  static std::unordered_map<std::string, ShardingID> task2sid;
};



Tree2Legion NSMapper::tree_result;
std::unordered_map<std::string, ShardingID> NSMapper::task2sid;

std::string NSMapper::get_policy_file()
{
  auto args = Runtime::get_input_args();
  for (auto idx = 0; idx < args.argc; ++idx)
  {
    if (strcmp(args.argv[idx], "-mapping") == 0)
    {
      if (idx + 1 >= args.argc) break;
      return args.argv[idx + 1];
    }
  }
  log_mapper.error("Policy file is missing");
  exit(-1);
}

std::string processor_kind_to_string(Processor::Kind kind)
{
  switch (kind)
  {
    case Processor::LOC_PROC: return "CPU";
    case Processor::TOC_PROC: return "GPU";
    case Processor::IO_PROC: return "IO";
    case Processor::PY_PROC: return "PY";
    case Processor::PROC_SET: return "PROC";
    case Processor::OMP_PROC: return "OMP";
    default:
    {
      assert(false);
      return "Unknown Kind";
    }
  }
}

std::string memory_kind_to_string(Memory::Kind kind)
{
  switch (kind)
  {
    case Memory::SYSTEM_MEM: return "SYSMEM";
    case Memory::GPU_FB_MEM: return "FBMEM";
    case Memory::REGDMA_MEM: return "RDMEM";
    case Memory::Z_COPY_MEM: return "ZCMEM";
    case Memory::SOCKET_MEM: return "SOCKETMEM";
    default:
    {
      assert(false);
      return "Unknown Kind";
    }
  }
}

void NSMapper::register_user_sharding_functors(Runtime *runtime)
{
  int i = 1;
  for (auto v: tree_result.task2func)
  {
    runtime->register_sharding_functor(i, new Legion::Internal::UserShardingFunctor(v.first));
    task2sid.insert({v.first, i});
    log_mapper.debug("%s inserted", v.first.c_str());
    i += 1;
  }
}

void NSMapper::parse_policy_file(const std::string &policy_file)
{
  log_mapper.debug("Policy file: %s", policy_file.c_str());
  tree_result = Tree2Legion(policy_file);
  // tree_result.print();
}

inline Processor NSMapper::idx_to_proc(int proc_idx, const Processor::Kind proc_kind) const
{
    switch (proc_kind)
    {
        case Processor::LOC_PROC:
        {
            assert(proc_idx < this->local_cpus.size());
            return this->local_cpus[proc_idx];
        }
        case Processor::TOC_PROC:
        {
            assert(proc_idx < this->local_gpus.size());
            return this->local_gpus[proc_idx];
        }
        case Processor::IO_PROC:
        {
            assert(proc_idx < this->local_ios.size());
            return this->local_ios[proc_idx];
        }
        case Processor::PY_PROC:
        {
            assert(proc_idx < this->local_pys.size());
            return this->local_pys[proc_idx];
        }
        case Processor::PROC_SET:
        {
            assert(proc_idx < this->local_procsets.size());
            return this->local_procsets[proc_idx];
        }
        case Processor::OMP_PROC:
        {
            assert(proc_idx < this->local_omps.size());
            return this->local_omps[proc_idx];
        }
        default:
        {
          assert(false);
        }
    }
    assert(false);
    return this->local_cpus[0];
}

int NSMapper::get_proc_idx(const Processor proc) const
{
    int proc_idx = 0;
    switch (proc.kind())
    {
        case Processor::LOC_PROC:
        {
            proc_idx = std::find(this->local_cpus.begin(), this->local_cpus.end(), proc) - this->local_cpus.begin();
            assert(proc_idx < this->local_cpus.size()); // it must find
            break;
        }
        case Processor::TOC_PROC:
        {
            proc_idx = std::find(this->local_gpus.begin(), this->local_gpus.end(), proc) - this->local_gpus.begin();
            assert(proc_idx < this->local_gpus.size()); // it must find
            break;
        }
        case Processor::IO_PROC:
        {
            proc_idx = std::find(this->local_ios.begin(), this->local_ios.end(), proc) - this->local_ios.begin();
            break;
        }
        case Processor::PY_PROC:
        {
            proc_idx = std::find(this->local_pys.begin(), this->local_pys.end(), proc) - this->local_pys.begin();
            break;
        }
        case Processor::PROC_SET:
        {
            proc_idx = std::find(this->local_procsets.begin(), this->local_procsets.end(), proc) - this->local_procsets.begin();
            break;
        }
        case Processor::OMP_PROC:
        {
            proc_idx = std::find(this->local_omps.begin(), this->local_omps.end(), proc) - this->local_omps.begin();
            break;
        }
        default:
        {
          assert(false);
        }
    }
    return proc_idx;
}

Processor NSMapper::select_initial_processor_by_kind(const Task &task, Processor::Kind kind)
{
  Processor result;
  switch (kind)
  {
    case Processor::LOC_PROC:
    {
      result = local_cpus.front();
      break;
    }
    case Processor::TOC_PROC:
    {
      result = !local_gpus.empty() ? local_gpus.front() : local_cpus.front();
      break;
    }
    case Processor::IO_PROC:
    {
      result = !local_ios.empty() ? local_ios.front() : local_cpus.front();
      break;
    }
    case Processor::PY_PROC:
    {
      result = !local_pys.empty() ? local_pys.front() : local_cpus.front();
      break;
    }
    case Processor::PROC_SET:
    {
      result = !local_procsets.empty() ? local_procsets.front() : local_cpus.front();
      break;
    }
    case Processor::OMP_PROC:
    {
      result = !local_omps.empty() ? local_omps.front() : local_cpus.front();
      break;
    }
    default:
    {
      assert(false);
    }
  }

  auto kind_str = processor_kind_to_string(kind);
  if (result.kind() != kind)
  {
    log_mapper.warning(
      "Unsatisfiable policy: task %s requested %s, which does not exist",
      task.get_task_name(), kind_str.c_str());
  }
  else
  {
    log_mapper.debug(
      "Task %s is initially mapped to %s",
      task.get_task_name(), kind_str.c_str()
    );
  }
  return result;
}

bool NSMapper::validate_processor_mapping(MapperContext ctx, const Task &task, Processor proc, bool strict)
{
  std::vector<VariantID> variants;
  runtime->find_valid_variants(ctx, task.task_id, variants, proc.kind());
  if (variants.empty())
  {
    if (strict)
    {
      auto kind_str = processor_kind_to_string(proc.kind());
      log_mapper.error(
        "Invalid policy: task %s requested %s, but has no valid task variant for the kind",
        task.get_task_name(), kind_str.c_str());
      exit(-1);
    }
    else
    {
      return false;
    }
  }
  return true;
}

Processor NSMapper::default_policy_select_initial_processor(MapperContext ctx, const Task &task)
{
  // todo: add support for selecting another node designated by DSL policy
  {
    auto finder = cached_task_policies.find(task.task_id);
    if (finder != cached_task_policies.end())
    {
      auto result = select_initial_processor_by_kind(task, finder->second);
      validate_processor_mapping(ctx, task, result);
      log_mapper.debug() << task.get_task_name() << " mapped by cache: " << processor_kind_to_string(result.kind()).c_str();
      return result;
    }
  }
  std::string task_name = task.get_task_name();
  {
    std::vector<Processor::Kind> proc_kind_vec;
    if (tree_result.task_policies.count(task_name) > 0)
    {
      proc_kind_vec = tree_result.task_policies.at(task_name);
    }
    else if (tree_result.task_policies.count("*") > 0)
    {
      proc_kind_vec = tree_result.task_policies.at("*");
    }
    for (size_t i = 0; i < proc_kind_vec.size(); i++)
    {
      auto result = select_initial_processor_by_kind(task, proc_kind_vec[i]);
      if (result.kind() != proc_kind_vec[i])
      {
        log_mapper.debug("Mapping %s onto %s cannot satisfy, try next",
          task_name.c_str(), processor_kind_to_string(proc_kind_vec[i]).c_str());
        continue;
      }
      // default policy validation should not be strict, allowing fallback
      bool success = validate_processor_mapping(ctx, task, result, false);
      if (success)
      {
        log_mapper.debug() << task_name << " mapped to " << processor_kind_to_string(result.kind()).c_str();
        cached_task_policies[task.task_id] = result.kind();
        return result;
      }
      else
      {
        log_mapper.debug("Mapping %s onto %s cannot satisfy with validation, try next",
          task_name.c_str(), processor_kind_to_string(proc_kind_vec[i]).c_str());
      }
    }
  }
  log_mapper.debug("%s falls back to the default policy", task_name.c_str());
  return DefaultMapper::default_policy_select_initial_processor(ctx, task);
}

void NSMapper::default_policy_select_target_processors(MapperContext ctx,
                                                       const Task &task,
                                                       std::vector<Processor> &target_procs)
{
    if (tree_result.should_fall_back(task.get_task_name()) == false)
    {
        std::vector<std::vector<int>> res;
        if (!task.is_index_space)
        {
            res = tree_result.runsingle(&task, this);
        }
        else
        {
            res = tree_result.runindex(&task);
        }
        int node_idx = res[0][0];
        assert(task.target_proc.address_space() == node_idx);
        for (int i = 0; i < res.size(); i++)
        {
            assert(res[i][0] == node_idx); // must be on the same node
            target_procs.push_back(idx_to_proc(res[i][1], task.target_proc.kind()));
        }
    }

    // if (!task.is_index_space && task.target_proc.kind() == task.orig_proc.kind()) {
       // target_procs.push_back(task.orig_proc);
    // }
    else
    {
        target_procs.push_back(task.target_proc);
    }
}

LogicalRegion NSMapper::default_policy_select_instance_region(MapperContext ctx,
                                                              Memory target_memory,
                                                              const RegionRequirement &req,
                                                              const LayoutConstraintSet &constraints,
                                                              bool force_new_instances,
                                                              bool meets_constraints)
{
  return req.region;
}

template <typename Handle>
void NSMapper::maybe_append_handle_name(const MapperContext ctx,
                                        const Handle &handle,
                                        std::vector<std::string> &names)
{
  const void *result = nullptr;
  size_t size = 0;
  if (runtime->retrieve_semantic_information(
        ctx, handle, LEGION_NAME_SEMANTIC_TAG, result, size, true, true))
    names.push_back(std::string(static_cast<const char*>(result)));
}

void NSMapper::get_handle_names(const MapperContext ctx,
                                const RegionRequirement &req,
                                std::vector<std::string> &names)
{
  maybe_append_handle_name(ctx, req.region, names);

  if (runtime->has_parent_logical_partition(ctx, req.region))
  {
    auto parent = runtime->get_parent_logical_partition(ctx, req.region);
    maybe_append_handle_name(ctx, parent, names);
  }

  if (req.region != req.parent)
    maybe_append_handle_name(ctx, req.parent, names);
}

Memory NSMapper::query_best_memory_for_proc(const Processor& proc, const Memory::Kind& mem_target_kind)
{
  if (cached_affinity_proc2mem.count({proc, mem_target_kind}) > 0)
  {
    return cached_affinity_proc2mem.at({proc, mem_target_kind});
  }
  Machine::MemoryQuery visible_memories(machine);
  // visible_memories.local_address_space()
  visible_memories.same_address_space_as(proc)
                  .only_kind(mem_target_kind);
  if (mem_target_kind == Memory::SOCKET_MEM || mem_target_kind == Memory::GPU_FB_MEM)
  {
      visible_memories.best_affinity_to(proc);
  }
  else
  // else if (mem_target_kind == Memory::Z_COPY_MEM)
  {
      visible_memories.has_affinity_to(proc);
  }
  if (visible_memories.count() > 0)
  {
    Memory result = visible_memories.first();
    if (result.exists())
    {
      cached_affinity_proc2mem.insert({{proc, mem_target_kind}, result});
      return result;
    }
  }
  return Memory::NO_MEMORY;
}

void NSMapper::map_task_post_function(const MapperContext   &ctx,
                                      const Task            &task,
                                      const std::string     &task_name,
                                      const Processor::Kind &proc_kind,
                                      MapTaskOutput         &output)
{
  if (tree_result.memory_collect.size() > 0)
  {
    for (size_t i = 0; i < task.regions.size(); i++)
    {
      auto &rg = task.regions[i];
      if (rg.privilege == READ_ONLY)
      {
        std::vector<std::string> path;
        get_handle_names(ctx, rg, path);
        if (tree_result.should_collect_memory(task_name, path))
        {
          output.untracked_valid_regions.insert(i);
        }
      }
    }
  }
  if (tree_result.query_max_instance(task_name, proc_kind) > 0)
  {
    output.task_prof_requests.add_measurement<ProfilingMeasurements::OperationStatus>();
  }
  return;
}

void NSMapper::map_task(const MapperContext      ctx,
                        const Task&              task,
                        const MapTaskInput&      input,
                              MapTaskOutput&     output)
{
  Processor::Kind target_proc_kind = task.target_proc.kind();
  std::string task_name = task.get_task_name();

  VariantInfo chosen = default_find_preferred_variant(task, ctx,
                    true/*needs tight bound*/, true/*cache*/, target_proc_kind);
  output.chosen_variant = chosen.variant;
  output.task_priority = default_policy_select_task_priority(ctx, task);
  output.postmap_task = false;

  // if (chosen.is_inner)
  // {
  //   log_mapper.debug(
  //     "is_inner = true, falling back to the default policy",
  //     task_name.c_str());
  //   DefaultMapper::map_task(ctx, task, input, output);
  //   map_task_post_function(ctx, task, task_name, target_proc_kind, output);
  //   return;
  // }

  default_policy_select_target_processors(ctx, task, output.target_procs);
  Processor target_processor = output.target_procs[0];
  log_mapper.debug("%s map_task for selecting memory will use %s as processor", 
    task_name.c_str(), processor_kind_to_string(target_processor.kind()).c_str());

  // const TaskLayoutConstraintSet &layout_constraints =
  //   runtime->find_task_layout_constraints(ctx, task.task_id, output.chosen_variant);

  for (uint32_t idx = 0; idx < task.regions.size(); ++idx)
  {
    auto &req = task.regions[idx];
    if (req.privilege == LEGION_NO_ACCESS || req.privilege_fields.empty()) continue;
    if ((req.tag & DefaultMapper::VIRTUAL_MAP) != 0 && req.privilege != LEGION_REDUCE)
    {
      PhysicalInstance virt_inst = PhysicalInstance::get_virtual_instance();
      output.chosen_instances[idx].push_back(virt_inst);
      continue;
    }

    bool found_policy = false;
    Memory::Kind target_kind = Memory::NO_MEMKIND;
    Memory target_memory = Memory::NO_MEMORY;
    std::string region_name;
    LayoutConstraintSet layout_constraints;

    auto cache_key = std::make_pair(task.task_id, idx);
    auto finder = cached_region_policies.find(cache_key);
    if (finder != cached_region_policies.end())
    {
      found_policy = true;
      target_kind = finder->second;
      region_name = cached_region_names.find(cache_key)->second;
      layout_constraints = cached_region_layout.find(cache_key)->second;
      target_memory = query_best_memory_for_proc(target_processor, target_kind);
      assert(target_memory.exists());
      log_mapper.debug() << "cached_region_policies: " << memory_kind_to_string(target_kind);
    }

    if (!found_policy)
    {
      std::vector<std::string> path;
      get_handle_names(ctx, req, path);
      log_mapper.debug() << "found_policy = false; path.size() = " << path.size(); // use index for regent
      for (int i = 0; i < path.size(); i++)
          log_mapper.debug() << path[i];
      std::vector<Memory::Kind> memory_list = tree_result.query_memory_list(task_name, path, target_proc_kind);
      for (auto &mem_kind: memory_list)
      {
        log_mapper.debug() << "querying " << target_processor.id << 
          " for memory " << memory_kind_to_string(mem_kind);
        Memory target_memory_try = query_best_memory_for_proc(target_processor, mem_kind);
        if (target_memory_try.exists())
        {
          log_mapper.debug() << target_memory_try.id << " best affinity to " 
              << task.target_proc.id << " in task " << task.task_id;
          target_kind = mem_kind;
          target_memory = target_memory_try;
          found_policy = true;
          custom_policy_select_constraints(task, path, ctx, layout_constraints, target_memory, req);
          auto key = std::make_pair(task.task_id, idx);
          region_name =  task_name + "_region_" + std::to_string(idx);
          cached_region_policies[key] = target_kind;
          cached_region_names[key] = region_name;
          cached_region_layout[key] = layout_constraints;
          break;
        }
      }
    }

    if (!found_policy)
    {
      log_mapper.debug(
        "Cannot find a policy for memory: region %u of task %s cannot be mapped, falling back to the default policy",
        idx, task.get_task_name());
      auto mem_constraint =
        find_memory_constraint(ctx, task, output.chosen_variant, idx);
      target_memory =
        default_policy_select_target_memory(ctx, task.target_proc, req, mem_constraint);
    }
    // target memories need to have affinity to all processors; otherwise error
    // I only need to create one instance
    // std::vector<Processor>                      target_procs
    // chosen_instances is still one

    std::set<FieldID> missing_fields = req.privilege_fields;
    if (req.privilege == LEGION_REDUCE)
    {
      log_mapper.debug() << "privilege = LEGION_REDUCE";
      std::vector<PhysicalInstance> valid_instances_reduce;
      valid_instances_reduce.resize(missing_fields.size());
      size_t footprint;
      size_t instance_idx = 0;
      for (std::set<FieldID>::const_iterator it =
            missing_fields.begin(); it !=
            missing_fields.end(); it++, instance_idx++)
      {
        layout_constraints.field_constraint.field_set.clear();
        layout_constraints.field_constraint.field_set.push_back(*it);
        if (!default_make_instance(ctx, target_memory, layout_constraints,
                    valid_instances_reduce[instance_idx], TASK_MAPPING, true /*force_new_instances*/,
                    true/*meets*/, req, &footprint))
        {
          default_report_failed_instance_creation(task, idx,
            target_processor, target_memory, footprint);
        }
      }
      output.chosen_instances[idx] = valid_instances_reduce;
      continue;
    }
    size_t footprint;
    PhysicalInstance valid_instance;
    if (!default_make_instance(ctx, target_memory, layout_constraints, 
        valid_instance, TASK_MAPPING, req.privilege == LEGION_REDUCE /*force_new*/, 
        true, req, &footprint))
    {
      default_report_failed_instance_creation(task, idx,
              target_processor, target_memory, footprint);
    }
    output.chosen_instances[idx].push_back(valid_instance);
  }
  map_task_post_function(ctx, task, task_name, target_proc_kind, output);
}

void NSMapper::report_profiling(const MapperContext ctx,
                                  const Task& task,
                                  const TaskProfilingInfo& input) {
  // We should only get profiling responses if we've enabled backpressuring.
  std::string task_name = task.get_task_name();
  Processor::Kind proc_kind  = task.orig_proc.kind();
  assert(tree_result.query_max_instance(task_name, proc_kind) > 0);
  bool is_index_launch = task.is_index_space;// && task.get_slice_domain().get_volume() > 1;
  // taco_iassert(this->enableBackpressure);
  // We should only get profiling responses for tasks that are supposed to be backpressured.
  // taco_iassert((task.tag & BACKPRESSURE_TASK) != 0);
  auto prof = input.profiling_responses.get_measurement<ProfilingMeasurements::OperationStatus>();
  // All our tasks should complete successfully.
  assert(prof->result == Realm::ProfilingMeasurements::OperationStatus::COMPLETED_SUCCESSFULLY);
  // Clean up after ourselves.
  delete prof;
  // Backpressured tasks are launched in a loop, and are kept on the originating processor.
  // So, we'll use orig_proc to index into the queue.
  auto& inflight = this->backPressureQueue[task.orig_proc];
  MapperEvent event;
  // Find this task in the queue.
  for (auto it = inflight.begin(); it != inflight.end(); it++) {
    is_index_launch = false;
    // adhoc solution: task.get_slice_domain() is not supported on this specific version of Legion
    // todo: fix this adhoc solution
    if (is_index_launch)
    { // if (it->id == task.get_unique_id()) {
      // if (it->id == std::make_pair(task.get_slice_domain(), task.get_context_index())) { 
      //   event = it->event;
      //   inflight.erase(it);
      //   break;
      // }
    }
    else
    {
      if (it->id2 == task.get_unique_id()) {
        event = it->event;
        inflight.erase(it);
        break;
      }
    }
  }
  // Assert that we found a valid event.
  assert(event.exists());
  // Finally, trigger the event for anyone waiting on it.
  this->runtime->trigger_mapper_event(ctx, event);
}

// In select_tasks_to_map, we attempt to perform backpressuring on tasks that
// need to be backpressured.

void NSMapper::select_tasks_to_map(const MapperContext ctx,
                                      const SelectMappingInput& input,
                                           SelectMappingOutput& output) {
  if (NSMapper::tree_result.task2limit.size() == 0)
  {
    DefaultMapper::select_tasks_to_map(ctx, input, output);
  }
  else
  {
    // Mark when we are potentially scheduling tasks.
    auto schedTime = std::chrono::high_resolution_clock::now();
    // Create an event that we will return in case we schedule nothing.
    MapperEvent returnEvent;
    // Also maintain a time point of the best return event. We want this function
    // to get invoked as soon as any backpressure task finishes, so we'll use the
    // completion event for the earliest one.
    auto returnTime = std::chrono::high_resolution_clock::time_point::max();

    // Find the depth of the deepest task.
    int max_depth = 0;
    for (std::list<const Task*>::const_iterator it =
        input.ready_tasks.begin(); it != input.ready_tasks.end(); it++)
    {
      int depth = (*it)->get_depth();
      if (depth > max_depth)
        max_depth = depth;
    }
    unsigned count = 0;
    // Only schedule tasks from the max depth in any pass.
    for (std::list<const Task*>::const_iterator it =
        input.ready_tasks.begin(); (count < max_schedule_count) &&
                                   (it != input.ready_tasks.end()); it++)
    {
      auto task = *it;
      bool schedule = true;
      std::string task_name = task->get_task_name();
      bool is_index_launch = task->is_index_space; // && task->get_slice_domain().get_volume() > 1;
      Processor::Kind proc_kind = task->orig_proc.kind();
      int max_num = tree_result.query_max_instance(task_name, proc_kind);
      if (max_num > 0)
      {
        // See how many tasks we have in flight. Again, we use the orig_proc here
        // rather than target_proc to match with our heuristics for where serial task
        // launch loops go.
        std::deque<InFlightTask> inflight = this->backPressureQueue[task->orig_proc];
        if (inflight.size() == max_num) {
          // We've hit the cap, so we can't schedule any more tasks.
          schedule = false;
          // As a heuristic, we'll wait on the first mapper event to
          // finish, as it's likely that one will finish first. We'll also
          // try to get a task that will complete before the current best.
          auto front = inflight.front();
          if (front.schedTime < returnTime) {
            returnEvent = front.event;
            returnTime = front.schedTime;
          }
        } else {
          // Otherwise, we can schedule the task. Create a new event
          // and queue it up on the processor.
          is_index_launch = false;
          // adhoc solution: task->get_slice_domain() is not supported on this specific commit of Legion
          // todo: fix this problem
          if (is_index_launch)
          {
            // this->backPressureQueue[task->orig_proc].push_back({
            //   .id = std::make_pair(task->get_slice_domain(), task->get_context_index()),
            //   // .id = task->get_unique_id(),
            //   .event = this->runtime->create_mapper_event(ctx),
            //   .schedTime = schedTime,
            // });
          }
          else
          {
              InFlightTask a;
              // a.id = Domain::NO_DOMAIN,
              a.id2 = task->get_unique_id();
              a.event = this->runtime->create_mapper_event(ctx);
              a.schedTime = schedTime;
              this->backPressureQueue[task->orig_proc].push_back(a);
          }
        }
      }
      // Schedule tasks that are valid and have the target depth.
      if (schedule && (*it)->get_depth() == max_depth)
      {
        output.map_tasks.insert(*it);
        count++;
      }
    }
    // If we didn't schedule any tasks, tell the runtime to ask us again when
    // our return event triggers.
    if (output.map_tasks.empty()) {
      assert(returnEvent.exists());
      output.deferral_event = returnEvent;
    }
  }
}


Mapper::MapperSyncModel NSMapper::get_mapper_sync_model() const {
  // If we're going to attempt to backpressure tasks, then we need to use
  // a sync model with high gaurantees.
  if (NSMapper::tree_result.task2limit.size() > 0) {
    return SERIALIZED_NON_REENTRANT_MAPPER_MODEL;
  }
  // Otherwise, we can do whatever the default mapper is doing.
  return DefaultMapper::get_mapper_sync_model();
}


void NSMapper::select_sharding_functor(
                                const MapperContext                ctx,
                                const Task&                        task,
                                const SelectShardingFunctorInput&  input,
                                      SelectShardingFunctorOutput& output)
{
  auto finder1 = task2sid.find(task.get_task_name());
  auto finder2 = task2sid.find("*");
  if (finder1 != task2sid.end())
  {
    output.chosen_functor = finder1->second;
    log_mapper.debug("select_sharding_functor user-defined for task %s: %d",
      task.get_task_name(), output.chosen_functor);
  }
  else if (finder2 != task2sid.end())
  {
    output.chosen_functor = finder2->second;
    log_mapper.debug("select_sharding_functor default user-defined for task %s: %d",
      task.get_task_name(), output.chosen_functor);
  }
  else
  {
    assert(tree_result.should_fall_back(task.get_task_name()) == true);
    log_mapper.debug("No sharding functor found in select_sharding_functor %s, fall back to default", task.get_task_name());
    output.chosen_functor = 0; // default functor
  }
}

void NSMapper::select_task_options(const MapperContext    ctx,
                                  const Task&            task,
                                        TaskOptions&     output)
//--------------------------------------------------------------------------
{
  log_mapper.debug("NSMapper select_task_options in %s", get_mapper_name());
  output.initial_proc = default_policy_select_initial_processor(ctx, task);
  output.inline_task = false;
  output.stealable = stealing_enabled;
  // This is the best choice for the default mapper assuming
  // there is locality in the remote mapped tasks
  output.map_locally = map_locally;
  // Control replicate the top-level task in multi-node settings
  // otherwise we do no control replication
#ifdef DEBUG_CTRL_REPL
  if (task.get_depth() == 0)
#else
  if ((total_nodes > 1) && (task.get_depth() == 0))
#endif
    output.replicate = replication_enabled;
    //output.replicate = false; // no replication for now..
  else
    output.replicate = false;
}

template<int DIM>
/*static*/ void NSMapper::custom_decompose_points(
                        std::vector<int> &index_launch_space,
                        const DomainT<DIM,coord_t> &point_space,
                        const std::vector<Processor> &targets,
                        bool recurse, bool stealable,
                        std::vector<TaskSlice> &slices,
                        std::string taskname)
//--------------------------------------------------------------------------
{
  log_mapper.debug() << "custom_decompose_points, dim=" << DIM 
    << " point_space.volume()=" << point_space.volume()
    << " point_space=[" << point_space.bounds.lo[0] << "," << point_space.bounds.hi[0] << "]";
  slices.reserve(point_space.volume());
  
  for (Realm::IndexSpaceIterator<DIM,coord_t> it(point_space); it.valid; it.step()) 
  {
    for (Legion::PointInRectIterator<DIM,coord_t> itr(it.rect); itr(); itr++)
    {
      const Point<DIM,coord_t> point = *itr;
      std::vector<int> index_point;
      log_mapper.debug("slice point: ");
      for (int i = 0; i < DIM; i++)
      {
        index_point.push_back(point[i]);
        log_mapper.debug() << point[i] << " ,";
      }
      size_t slice_res = 
        (size_t) tree_result.runindex(taskname, index_point, index_launch_space,targets[0].kind())[0][1];
      log_mapper.debug("--> %ld", slice_res);
      if (slice_res >= targets.size())
      {
        log_mapper.error("%ld >= %ld, targets out of bound!", slice_res, targets.size());
        assert(false);
      }
      // Construct the output slice for Legion.
      Legion::DomainT<DIM, Legion::coord_t> slice;
      slice.bounds.lo = point;
      slice.bounds.hi = point;
      slice.sparsity = point_space.sparsity;
      if (!slice.dense()) { slice = slice.tighten(); }
      if (slice.volume() > 0) 
      {
        TaskSlice ts;
        ts.domain = slice;
        ts.proc = targets[slice_res];
        ts.recurse = recurse;
        ts.stealable = stealable;
        slices.push_back(ts);
      }
    }
  }
}


void NSMapper::custom_slice_task(const Task &task,
                                const std::vector<Processor> &local,
                                const std::vector<Processor> &remote,
                                const SliceTaskInput& input,
                                      SliceTaskOutput &output) const
  //--------------------------------------------------------------------------
{
  // The two-level decomposition doesn't work so for now do a
  // simple one-level decomposition across all the processors.
  Machine::ProcessorQuery all_procs(machine);
  all_procs.only_kind(local[0].kind());
  all_procs.local_address_space();
  size_t node_num = Machine::get_machine().get_address_space_count();
  log_mapper.debug("how many nodes? %ld", node_num);
  log_mapper.debug("how many processors? local=%ld, remote=%ld", local.size(), remote.size());
  log_mapper.debug("node_id = %d", node_id);
  // if ((task.tag & SAME_ADDRESS_SPACE) != 0 || same_address_space)
  // {
  //   log_mapper.debug("local_address_space executed");
  //   all_procs.local_address_space();
  // }
  std::vector<Processor> procs(all_procs.begin(), all_procs.end());
  log_mapper.debug("Inside custom_slice_task for %s, procs=%ld, dim=%d",
    task.get_task_name(), procs.size(), input.domain.get_dim());
  
  std::vector<int> launch_space;
  Domain task_index_domain = task.index_domain;
  switch (task_index_domain.get_dim())
  {
#define DIMFUNC(DIM) \
    case DIM: \
      { \
        const DomainT<DIM,coord_t> is = task_index_domain; \
        for (int i = 0; i < DIM; i++) \
        { \
          launch_space.push_back(is.bounds.hi[i] - is.bounds.lo[i] + 1); \
        } \
        log_mapper.debug("launch_space for %s: ", task.get_task_name()); \
        for (int i = 0; i < DIM; i++) \
        { \
          log_mapper.debug("%lld, ", is.bounds.hi[i] - is.bounds.lo[i] + 1); \
        } \
        break; \
      }
    LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
    default:
      assert(false);
  }
  
  switch (input.domain.get_dim())
  {
#define BLOCK(DIM) \
    case DIM: \
      { \
        DomainT<DIM,coord_t> partial_point_space = input.domain; \
        custom_decompose_points<DIM>(launch_space, partial_point_space, procs, \
              false/*recurse*/, stealing_enabled, output.slices, task.get_task_name()); \
        break; \
      }
    LEGION_FOREACH_N(BLOCK)
#undef BLOCK
    default: // don't support other dimensions right now
      assert(false);
  }
}

void NSMapper::slice_task(const MapperContext      ctx,
                          const Task&              task, 
                          const SliceTaskInput&    input,
                          SliceTaskOutput&   output)
{
  if (tree_result.should_fall_back(std::string(task.get_task_name())))
  {
    log_mapper.debug("Use default slice_task for %s", task.get_task_name());
    DefaultMapper::slice_task(ctx, task, input, output);
    return;
  }
  log_mapper.debug("Customize slice_task for %s", task.get_task_name());
  // Whatever kind of processor we are is the one this task should
  // be scheduled on as determined by select initial task
  Processor::Kind target_kind =
    task.must_epoch_task ? local_proc.kind() : task.target_proc.kind();
  log_mapper.debug("%d,%d:%d", target_kind, local_proc.kind(), task.target_proc.kind());
  switch (target_kind)
  {
    case Processor::LOC_PROC:
      {
        log_mapper.debug("%d: CPU here", target_kind);
        custom_slice_task(task, local_cpus, remote_cpus, input, output);
        break;
      }
    case Processor::TOC_PROC:
      {
        log_mapper.debug("%d: GPU here", target_kind);
        custom_slice_task(task, local_gpus, remote_gpus, input, output);
        break;
      }
    case Processor::IO_PROC:
      {
        log_mapper.debug("%d: IO here", target_kind);
        custom_slice_task(task, local_ios, remote_ios, input, output);
        break;
      }
    case Processor::PY_PROC:
      {
        log_mapper.debug("%d: PY here", target_kind);
        custom_slice_task(task, local_pys, remote_pys, input, output);
        break;
      }
    case Processor::PROC_SET:
      {
        log_mapper.debug("%d: PROC here", target_kind);
        custom_slice_task(task, local_procsets, remote_procsets, input, output);
        break;
      }
    case Processor::OMP_PROC:
      {
        log_mapper.debug("%d: OMP here", target_kind);
        custom_slice_task(task, local_omps, remote_omps, input, output);
        break;
      }
    default:
      assert(false); // unimplemented processor kind
  }
}

NSMapper::NSMapper(MapperRuntime *rt, Machine machine, Processor local, const char *mapper_name)
  : DefaultMapper(rt, machine, local, mapper_name)
{
  std::string policy_file = get_policy_file();
  parse_policy_file(policy_file);
}

static void create_mappers(Machine machine, Runtime *runtime, const std::set<Processor> &local_procs)
{
  log_mapper.debug("Inside create_mappers local_procs.size() = %ld", local_procs.size());
  bool backpressure = false;
  for (std::set<Processor>::const_iterator it = local_procs.begin();
        it != local_procs.end(); it++)
  {
    NSMapper* mapper = new NSMapper(runtime->get_mapper_runtime(), machine, *it, "ns_mapper");
    if (it == local_procs.begin())
    {
      NSMapper::register_user_sharding_functors(runtime);
      backpressure = (mapper->tree_result.task2limit.size() > 0);
    }
#ifdef USE_LOGGING_MAPPER
    runtime->replace_default_mapper(new Mapping::LoggingWrapper(mapper), *it);
#else
    runtime->replace_default_mapper(mapper, *it);
#endif
    if (backpressure)
    {
      break;
    }
  }
}

void register_mappers()
{
  Runtime::add_registration_callback(create_mappers);
}

namespace Legion {
  namespace Internal {
        /////////////////////////////////////////////////////////////
    // User Sharding Functor
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    UserShardingFunctor::UserShardingFunctor(std::string mode)
      : ShardingFunctor(), taskname(mode)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    UserShardingFunctor::UserShardingFunctor(
                                            const UserShardingFunctor &rhs)
      : ShardingFunctor()
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    UserShardingFunctor::~UserShardingFunctor(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    UserShardingFunctor& UserShardingFunctor::operator=(
                                              const UserShardingFunctor &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    ShardID UserShardingFunctor::shard(const DomainPoint &point,
                                         const Domain &full_space,
                                         const size_t total_shards)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_LEGION
      assert(point.get_dim() == full_space.get_dim());
#endif
      log_mapper.debug("shard dim: %d, total_shards: %ld", point.get_dim(), total_shards);
      // size_t node_num = Machine::get_machine().get_address_space_count();
      switch (point.get_dim())
      {
#define DIMFUNC(DIM) \
        case DIM: \
          { \
            const DomainT<DIM,coord_t> is = full_space; \
            const Point<DIM,coord_t> p1 = point; \
            std::vector<int> index_point, launch_space; \
            for (int i = 0; i < DIM; i++) \
            { \
              index_point.push_back(p1[i]); \
              launch_space.push_back(is.bounds.hi[i] - is.bounds.lo[i] + 1); \
            } \
            log_mapper.debug("shard point for %s: ", taskname.c_str()); \
            for (int i = 0; i < DIM; i++) \
            { \
              log_mapper.debug("%lld, ", point[i]); \
            } \
            log_mapper.debug(" --> node %d\n", (int) NSMapper::tree_result.runindex(taskname, index_point, launch_space)[0][0]); \
            return NSMapper::tree_result.runindex(taskname, index_point, launch_space)[0][0]; \
          }
        LEGION_FOREACH_N(DIMFUNC)
#undef DIMFUNC
        default:
          assert(false);
      }
      log_mapper.debug("shard: should never reach");
      assert(false);
      return 0;
    }
  }
}


void NSMapper::custom_policy_select_constraints(const Task &task,
                                      const std::vector<std::string> &region_names,
                                      const Legion::Mapping::MapperContext ctx,
                                      Legion::LayoutConstraintSet &constraints,
                                      const Legion::Memory &target_memory,
                                      const Legion::RegionRequirement &req)
{
  Memory::Kind target_memory_kind = target_memory.kind();

  ConstraintsNode dsl_constraint;

  ConstraintsNode* dsl_constraint_pt = tree_result.query_constraint(task.get_task_name(), region_names, target_memory_kind);
  if (dsl_constraint_pt != NULL)
  {
    dsl_constraint = *dsl_constraint_pt;
    log_mapper.debug() << "dsl_constraint specified by the user";
  }

  Legion::IndexSpace is = req.region.get_index_space();
  Legion::Domain domain = runtime->get_index_space_domain(ctx, is);
  int dim = domain.get_dim();
  std::vector<Legion::DimensionKind> dimension_ordering(dim + 1);

  if (dsl_constraint.reverse)
  {
    log_mapper.debug() << "dsl_constraint.reverse = true";
    if (dsl_constraint.aos)
    {
      log_mapper.debug() << "dsl_constraint.aos = true";
      for (int i = 0; i < dim; ++i)
      {
        dimension_ordering[dim - i] =
          static_cast<Legion::DimensionKind>(static_cast<int>(LEGION_DIM_X) + i);
      }
      dimension_ordering[0] = LEGION_DIM_F;
    }
    else
    {
      log_mapper.debug() << "dsl_constraint.aos = false";
      for (int i = 0; i < dim; ++i)
      {
        dimension_ordering[dim - i - 1] =
          static_cast<Legion::DimensionKind>(static_cast<int>(LEGION_DIM_X) + i);
      }
      dimension_ordering[dim] = LEGION_DIM_F; // soa
    }
  }
  else
  {
    log_mapper.debug() << "dsl_constraint.reverse = false";
    if (dsl_constraint.aos)
    {
      log_mapper.debug() << "dsl_constraint.aos = true";
      for (int i = 1; i < dim + 1; ++i)
      {
        dimension_ordering[i] =
          static_cast<Legion::DimensionKind>(static_cast<int>(LEGION_DIM_X) + i);
      }
      dimension_ordering[0] = LEGION_DIM_F; // aos
    }
    else
    {
      log_mapper.debug() << "dsl_constraint.aos = false";
      for (int i = 0; i < dim; ++i)
      {
        dimension_ordering[i] =
          static_cast<Legion::DimensionKind>(static_cast<int>(LEGION_DIM_X) + i);
      }
      dimension_ordering[dim] = LEGION_DIM_F; // soa
    }
  }
  constraints.add_constraint(Legion::OrderingConstraint(dimension_ordering, false/*contiguous*/));
  // If we were requested to have an alignment, add the constraint.
  if (dsl_constraint.align)
  {
    log_mapper.debug() << "dsl_constraint.align = true";
    for (auto it : req.privilege_fields) {
      constraints.add_constraint(Legion::AlignmentConstraint(it,
        myop2legion(dsl_constraint.align_op), dsl_constraint.align_int));
    }
  }

  // Exact Region Constraint
  bool special_exact = dsl_constraint.exact;
  /*
     SpecializedConstraint(SpecializedKind kind = LEGION_AFFINE_SPECIALIZE
                           ReductionOpID redop = 0,
                           bool no_access = false,
                           bool exact = false
  */
  if (dsl_constraint.compact)
  {
    // sparse instance; we use SpecializedConstraint, which unfortunately has to override the default mapper
    log_mapper.debug() << "dsl_constraint.compact = true";
    assert(req.privilege != LEGION_REDUCE);
    constraints.add_constraint(SpecializedConstraint(LEGION_COMPACT_SPECIALIZE, 0, false,
                                                     special_exact));
  }
  else if (req.privilege == LEGION_REDUCE)
  {
    // Make reduction fold instances.
    constraints.add_constraint(SpecializedConstraint(LEGION_AFFINE_REDUCTION_SPECIALIZE, req.redop, false,
                                                     special_exact))
               .add_constraint(MemoryConstraint(target_memory.kind()));
  }
  else
  {
    // Our base default mapper will try to make instances of containing
    // all fields (in any order) to encourage
    // maximum re-use by any tasks which use subsets of the fields
    constraints.add_constraint(SpecializedConstraint(LEGION_AFFINE_SPECIALIZE, 0, false,
                                                     special_exact))
               .add_constraint(MemoryConstraint(target_memory.kind()));
    if (constraints.field_constraint.field_set.size() == 0)
    {
      // Normal instance creation
      std::vector<FieldID> fields;
      default_policy_select_constraint_fields(ctx, req, fields);
      constraints.add_constraint(FieldConstraint(fields,false/*contiguous*/,false/*inorder*/));
    }
  }
}

legion_equality_kind_t myop2legion(BinOpEnum myop)
{
  // BIGGER,	SMALLER,	GE,	LE,	EQ,	NEQ,:
  switch (myop)
  {
    case BIGGER:
      return LEGION_GT_EK;
    case SMALLER:
      return LEGION_LT_EK;
    case GE:
      return LEGION_GE_EK;
    case LE:
      return LEGION_LE_EK;
    case EQ:
      return LEGION_EQ_EK;
    case NEQ:
      return LEGION_NE_EK;
    default:
      break;
  }
  assert(false);
  return LEGION_EQ_EK;
}

#include "compiler/tree.cpp"

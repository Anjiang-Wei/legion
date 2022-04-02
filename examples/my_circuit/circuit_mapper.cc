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

#include "circuit_mapper.h"

#include "mappers/default_mapper.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>

using namespace Legion;
using namespace Legion::Mapping;

static Logger log_mapper("nsmapper");

template <typename T1, typename T2>
struct PairHash
{
  using VAL = std::pair<T1, T2>;
  std::size_t operator()(VAL const& pair) const noexcept
  {
    return std::hash<T1>{}(pair.first) << 1 ^ std::hash<T2>{}(pair.second);
  }
};

class NSMapper : public DefaultMapper
{
public:
  NSMapper(MapperRuntime *rt, Machine machine, Processor local, const char *mapper_name);

public:
  static std::string get_policy_file();
  void parse_policy_file(const std::string &policy_file);

private:
  Processor select_initial_processor_by_kind(const Task &task, Processor::Kind kind);
  void validate_processor_mapping(MapperContext ctx, const Task &task, Processor proc);
  template <typename Handle>
  void maybe_append_handle_name(const MapperContext ctx,
                                const Handle &handle,
                                std::vector<std::string> &names);
  void get_handle_names(const MapperContext ctx,
                        const RegionRequirement &req,
                        std::vector<std::string> &names);

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

private:
  std::unordered_map<std::string, Processor::Kind> task_policies;
  std::unordered_map<TaskID, Processor::Kind> cached_task_policies;

  std::unordered_set<std::string> has_region_policy;
  using HashFn1 = PairHash<std::string, std::string>;
  std::unordered_map<std::pair<std::string, std::string>, Memory::Kind, HashFn1> region_policies;
  using HashFn2 = PairHash<TaskID, uint32_t>;
  std::unordered_map<std::pair<TaskID, uint32_t>, Memory::Kind, HashFn2> cached_region_policies;
  std::unordered_map<std::pair<TaskID, uint32_t>, std::string, HashFn2> cached_region_names;
};

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

Processor::Kind parse_processor_kind(const std::string &kind_string)
{
  if ("CPU" == kind_string) return Processor::LOC_PROC;
  else if ("GPU" == kind_string) return Processor::TOC_PROC;
  else
  {
    log_mapper.error(
      "Unknown processor kind %s (supported kinds: CPU, GPU)",
      kind_string.c_str());
    exit(-1);
  }
}

Memory::Kind parse_memory_kind(const std::string &kind_string)
{
  if ("SYSMEM" == kind_string) return Memory::SYSTEM_MEM;
  else if ("FBMEM" == kind_string) return Memory::GPU_FB_MEM;
  else if ("RDMEM" == kind_string) return Memory::REGDMA_MEM;
  else if ("ZCMEM" == kind_string) return Memory::Z_COPY_MEM;
  else
  {
    log_mapper.error(
      "Unknown processor kind %s (supported kinds: SYSMEM, FBMEM, RDMEM, ZCMEM)",
      kind_string.c_str());
    exit(-1);
  }
}

std::string processor_kind_to_string(Processor::Kind kind)
{
  switch (kind)
  {
    case Processor::LOC_PROC: return "CPU";
    case Processor::TOC_PROC: return "GPU";
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
    default:
    {
      assert(false);
      return "Unknown Kind";
    }
  }
}

void NSMapper::parse_policy_file(const std::string &policy_file)
{
  std::ifstream ifs;
  ifs.open(policy_file, std::ifstream::in);
  log_mapper.debug("Policy file: %s", policy_file.c_str());

  while (ifs.good()) {
    std::string token;
    ifs >> token;
    if ("task" == token)
    {
      std::string task_name; ifs >> task_name;
      std::string kind_string; ifs >> kind_string;
      log_mapper.debug(
        "Found task policy: map %s to %s", task_name.c_str(), kind_string.c_str());
      task_policies[task_name] = parse_processor_kind(kind_string);
    }
    else if ("region" == token)
    {
      std::string task_name; ifs >> task_name;
      std::string region_name; ifs >> region_name;
      std::string kind_string; ifs >> kind_string;
      log_mapper.debug(
        "Found region policy: map %s.%s to %s",
        task_name.c_str(), region_name.c_str(), kind_string.c_str());
      region_policies[std::make_pair(task_name, region_name)] = parse_memory_kind(kind_string);
      has_region_policy.insert(task_name);
    }
    else if (!token.empty())
    {
      log_mapper.error("Unknown token %s", token.c_str());
      exit(-1);
    }
  }
  ifs.close();
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
    default:
    {
      assert(false);
    }
  }

  auto kind_str = processor_kind_to_string(kind);
  if (result.kind() != kind)
    log_mapper.warning(
      "Unsatisfiable policy: task %s requested %s, which does not exist",
      task.get_task_name(), kind_str.c_str());
  else
    log_mapper.debug(
      "Task %s is initially mapped to %s",
      task.get_task_name(), kind_str.c_str()
    );
  return result;
}

void NSMapper::validate_processor_mapping(MapperContext ctx, const Task &task, Processor proc)
{
  std::vector<VariantID> variants;
  runtime->find_valid_variants(ctx, task.task_id, variants, proc.kind());
  if (variants.empty())
  {
    auto kind_str = processor_kind_to_string(proc.kind());
    log_mapper.error(
      "Invalid policy: task %s requested %s, but has no valid task variant for the kind",
      task.get_task_name(), kind_str.c_str());
    exit(-1);
  }
}

Processor NSMapper::default_policy_select_initial_processor(MapperContext ctx, const Task &task)
{
  {
    auto finder = cached_task_policies.find(task.task_id);
    if (finder != cached_task_policies.end())
    {
      auto result = select_initial_processor_by_kind(task, finder->second);
      validate_processor_mapping(ctx, task, result);
      return result;
    }
  }
  {
    auto finder = task_policies.find(task.get_task_name());
    if (finder != task_policies.end())
    {
      auto result = select_initial_processor_by_kind(task, finder->second);
      validate_processor_mapping(ctx, task, result);
      cached_task_policies[task.task_id] = result.kind();
      return result;
    }
  }
  log_mapper.debug(
    "No processor policy is given for task %s, falling back to the default policy",
    task.get_task_name());
  return DefaultMapper::default_policy_select_initial_processor(ctx, task);
}

void NSMapper::default_policy_select_target_processors(MapperContext ctx,
                                                       const Task &task,
                                                       std::vector<Processor> &target_procs)
{
  target_procs.push_back(task.target_proc);
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

void NSMapper::map_task(const MapperContext      ctx,
                        const Task&              task,
                        const MapTaskInput&      input,
                              MapTaskOutput&     output)
{
  if (has_region_policy.find(task.get_task_name()) == has_region_policy.end())
  {
    log_mapper.debug(
      "No memory policy is given for task %s, falling back to the default policy",
      task.get_task_name());
    DefaultMapper::map_task(ctx, task, input, output);
    return;
  }

  Processor::Kind target_kind = task.target_proc.kind();
  VariantInfo chosen = default_find_preferred_variant(task, ctx,
                    true/*needs tight bound*/, true/*cache*/, target_kind);
  output.chosen_variant = chosen.variant;
  output.task_priority = default_policy_select_task_priority(ctx, task);
  output.postmap_task = false;
  default_policy_select_target_processors(ctx, task, output.target_procs);

  if (chosen.is_inner)
  {
    log_mapper.debug(
      "Unsupported variant is chosen for task %s, falling back to the default policy",
      task.get_task_name());
    DefaultMapper::map_task(ctx, task, input, output);
    return;
  }

  const TaskLayoutConstraintSet &layout_constraints =
    runtime->find_task_layout_constraints(ctx, task.task_id, output.chosen_variant);

  for (uint32_t idx = 0; idx < task.regions.size(); ++idx)
  {
    auto &req = task.regions[idx];
    if (req.privilege == LEGION_NO_ACCESS || req.privilege_fields.empty()) continue;

    bool found_policy = false;
    Memory::Kind target_kind;
    Memory target_memory = Memory::NO_MEMORY;
    std::string region_name;

    auto cache_key = std::make_pair(task.task_id, idx);
    auto finder = cached_region_policies.find(cache_key);
    if (finder != cached_region_policies.end())
    {
      found_policy = true;
      target_kind = finder->second;
      region_name = cached_region_names.find(cache_key)->second;
    }

    if (!found_policy)
    {
      std::vector<std::string> path;
      get_handle_names(ctx, req, path);
      for (auto &name : path)
      {
        auto finder = region_policies.find(std::make_pair(task.get_task_name(), name));
        if (finder != region_policies.end())
        {
          target_kind = finder->second;
          found_policy = true;
          auto key = std::make_pair(task.task_id, idx);
          cached_region_policies[key] = target_kind;
          cached_region_names[key] = name;
          region_name = name;
          break;
        }
      }
    }

    if (found_policy)
    {
      Machine::MemoryQuery visible_memories(machine);
      visible_memories.has_affinity_to(task.target_proc);
      visible_memories.only_kind(target_kind);
      if (visible_memories.count() > 0)
        target_memory = visible_memories.first();
    }

    if (target_memory.exists())
    {
      auto kind_str = memory_kind_to_string(target_kind);
      log_mapper.debug(
          "Region %u of task %s (%s) is mapped to %s",
          idx, task.get_task_name(), region_name.c_str(), kind_str.c_str());
    }
    else
    {
      log_mapper.debug(
        "Unsatisfiable policy: region %u of task %s, falling back to the default policy",
        idx, task.get_task_name());
      auto mem_constraint =
        find_memory_constraint(ctx, task, output.chosen_variant, idx);
      target_memory =
        default_policy_select_target_memory(ctx, task.target_proc, req, mem_constraint);
    }

    auto missing_fields = req.privilege_fields;
    if (req.privilege == LEGION_REDUCE)
    {
      size_t footprint;
      if (!default_create_custom_instances(ctx, task.target_proc,
              target_memory, req, idx, missing_fields,
              layout_constraints, true,
              output.chosen_instances[idx], &footprint))
      {
        default_report_failed_instance_creation(task, idx,
              task.target_proc, target_memory, footprint);
      }
      continue;
    }

    std::vector<PhysicalInstance> valid_instances;

    for (auto &instance : input.valid_instances[idx])
      if (instance.get_location() == target_memory)
        valid_instances.push_back(instance);

    runtime->filter_instances(ctx, task, idx, output.chosen_variant,
                              valid_instances, missing_fields);

    bool check = runtime->acquire_and_filter_instances(ctx, valid_instances);
    assert(check);

    output.chosen_instances[idx] = valid_instances;

    if (missing_fields.empty()) continue;

    size_t footprint;
    if (!default_create_custom_instances(ctx, task.target_proc,
            target_memory, req, idx, missing_fields,
            layout_constraints, true,
            output.chosen_instances[idx], &footprint))
    {
      default_report_failed_instance_creation(task, idx,
              task.target_proc, target_memory, footprint);
    }
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
  for (std::set<Processor>::const_iterator it = local_procs.begin();
        it != local_procs.end(); it++)
  {
    NSMapper* mapper = new NSMapper(runtime->get_mapper_runtime(), machine, *it, "ns_mapper");
    runtime->replace_default_mapper(mapper, *it);
  }
}

void register_mappers()
{
  Runtime::add_registration_callback(create_mappers);
}

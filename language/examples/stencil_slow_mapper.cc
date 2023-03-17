/* Copyright 2021 Stanford University
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

#include "stencil_slow_mapper.h"

#include "mappers/default_mapper.h"
#include "dsl_mapper.cc"

#define SPMD_SHARD_USE_IO_PROC 1

using namespace Legion;
using namespace Legion::Mapping;

///
/// Sharding Functor
///

enum ShardingFunctorIDs {
  SID_LINEAR = 2022,
};

class LinearShardingFunctor : public ShardingFunctor {
public:
  LinearShardingFunctor();
  LinearShardingFunctor(const LinearShardingFunctor &rhs);
  virtual ~LinearShardingFunctor(void);
public:
  LinearShardingFunctor& operator=(const LinearShardingFunctor &rhs);
public:
  template<int DIM>
  size_t linearize_point(const Realm::IndexSpace<DIM,coord_t> &is,
                         const Realm::Point<DIM,coord_t> &point) const;
public:
  virtual ShardID shard(const DomainPoint &point,
                        const Domain &full_space,
                        const size_t total_shards);
};

//--------------------------------------------------------------------------
LinearShardingFunctor::LinearShardingFunctor()
  : ShardingFunctor()
//--------------------------------------------------------------------------
{
}

//--------------------------------------------------------------------------
LinearShardingFunctor::LinearShardingFunctor(
                                           const LinearShardingFunctor &rhs)
  : ShardingFunctor()
//--------------------------------------------------------------------------
{
  // should never be called
  assert(false);
}

//--------------------------------------------------------------------------
LinearShardingFunctor::~LinearShardingFunctor(void)
//--------------------------------------------------------------------------
{
}

//--------------------------------------------------------------------------
LinearShardingFunctor& LinearShardingFunctor::operator=(
                                           const LinearShardingFunctor &rhs)
//--------------------------------------------------------------------------
{
  // should never be called
  assert(false);
  return *this;
}

//--------------------------------------------------------------------------
template<int DIM>
size_t LinearShardingFunctor::linearize_point(
                               const Realm::IndexSpace<DIM,coord_t> &is,
                               const Realm::Point<DIM,coord_t> &point) const
//--------------------------------------------------------------------------
{
  Realm::AffineLinearizedIndexSpace<DIM,coord_t> linearizer(is);
  return linearizer.linearize(point);
}

//--------------------------------------------------------------------------
ShardID LinearShardingFunctor::shard(const DomainPoint &point,
                                     const Domain &full_space,
                                     const size_t total_shards)
//--------------------------------------------------------------------------
{
#ifdef DEBUG_LEGION
  assert(point.get_dim() == full_space.get_dim());
#endif
  size_t domain_size = full_space.get_volume();
  switch (point.get_dim())
  {
    case 1:
      {
        const DomainT<1,coord_t> is = full_space;
        const Point<1,coord_t> p1 = point;
        return linearize_point<1>(is, p1)  * total_shards / domain_size;
      }
    case 2:
      {
        const DomainT<2,coord_t> is = full_space;
        const Point<2,coord_t> p2 = point;
        return linearize_point<2>(is, p2)  * total_shards / domain_size;
      }
    case 3:
      {
        const DomainT<3,coord_t> is = full_space;
        const Point<3,coord_t> p3 = point;
        return linearize_point<3>(is, p3)  * total_shards / domain_size;
      }
    default:
      assert(false);
  }
  return 0;
}

///
/// Mapper
///

static LegionRuntime::Logger::Category log_stencil("stencil");

class StencilMapper : public DefaultMapper
{
public:
  StencilMapper(MapperRuntime *rt, Machine machine, Processor local,
                const char *mapper_name,
                std::vector<Processor>* procs_list);
private:
  std::vector<Processor>& procs_list;
};

StencilMapper::StencilMapper(MapperRuntime *rt, Machine machine, Processor local,
                             const char *mapper_name,
                             std::vector<Processor>* _procs_list)
  : DefaultMapper(rt, machine, local, mapper_name)
  , procs_list(*_procs_list)
{
}

static void create_mappers2(Machine machine, Runtime *runtime, const std::set<Processor> &local_procs)
{
  // log_mapper.debug("Inside create_mappers local_procs.size() = %ld", local_procs.size());
  bool use_logging_wrapper = false;
  bool use_dsl_mapper = false;
  auto args = Runtime::get_input_args();
  NSMapper::backpressure = false;
  NSMapper::use_semantic_name = false;
  NSMapper::untrackValidRegions = false;
  NSMapper::select_source_by_bandwidth = false;
  for (auto idx = 0; idx < args.argc; ++idx)
  {
    if (strcmp(args.argv[idx], "-wrapper") == 0)
    {
      use_logging_wrapper = true;
    }
    // todo: in the final version, change tm to be the formal name of DSLMapper
    if (strcmp(args.argv[idx], "-tm:enable_backpressure") == 0)
    {
      NSMapper::backpressure = true;
    }
    if (strcmp(args.argv[idx], "-tm:untrack_valid_regions") == 0)
    {
      NSMapper::untrackValidRegions = true;
    }
    if (strcmp(args.argv[idx], "-tm:use_semantic_name") == 0)
    {
      NSMapper::use_semantic_name = true;
    }
    if (strcmp(args.argv[idx], "-tm:select_source_by_bandwidth") == 0)
    {
      NSMapper::select_source_by_bandwidth = true;
    }
    if (strcmp(args.argv[idx], "-dslmapper") == 0)
    {
      use_dsl_mapper = true;
    }
  }
  if (use_dsl_mapper)
  {
    for (std::set<Processor>::const_iterator it = local_procs.begin();
        it != local_procs.end(); it++)
    {
      NSMapper* mapper = NULL;
      if (it == local_procs.begin())
      {
        mapper = new NSMapper(runtime->get_mapper_runtime(), machine, *it, "ns_mapper", true);
        mapper->register_user_sharding_functors(runtime);
        // todo: change back to this in final version
        // backpressure = (mapper->tree_result.task2limit.size() > 0);
      }
      else
      {
        mapper = new NSMapper(runtime->get_mapper_runtime(), machine, *it, "ns_mapper", false);
      }
      if (use_logging_wrapper)
      {
        runtime->replace_default_mapper(new Mapping::LoggingWrapper(mapper), (NSMapper::backpressure ? (Processor::NO_PROC) : (*it)));
      }
      else
      {
        runtime->replace_default_mapper(mapper, (NSMapper::backpressure ? (Processor::NO_PROC) : (*it)));
      }
      if (NSMapper::backpressure)
      {
        break;
      }
    }
    return;
  }

  std::vector<Processor>* procs_list = new std::vector<Processor>();

  Machine::ProcessorQuery procs_query(machine);
  procs_query.only_kind(Processor::TOC_PROC);
  for (Machine::ProcessorQuery::iterator it = procs_query.begin();
        it != procs_query.end(); it++)
    procs_list->push_back(*it);

  for (std::set<Processor>::const_iterator it = local_procs.begin();
        it != local_procs.end(); it++)
  {
    StencilMapper* mapper = new StencilMapper(runtime->get_mapper_runtime(),
                                              machine, *it, "stencil_mapper",
                                              procs_list);
    if (use_logging_wrapper)
    {
      runtime->replace_default_mapper(new Mapping::LoggingWrapper(mapper), *it);
    }
    else
    {
      runtime->replace_default_mapper(mapper, *it);
    }
  }
}

void register_mappers2()
{
  // LinearShardingFunctor *sharding_functor = new LinearShardingFunctor();
  // Runtime::preregister_sharding_functor(SID_LINEAR, sharding_functor);

  Runtime::add_registration_callback(create_mappers2);
}

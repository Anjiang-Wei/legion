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

#include "cholesky_mapper.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <vector>

#include "mappers/default_mapper.h"
#include "dsl_mapper.cc"

using namespace Legion;
using namespace Legion::Mapping;


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
}

void register_mappers2() // avoid conflict definition in DSL mapper
{
  Runtime::add_registration_callback(create_mappers2);
}
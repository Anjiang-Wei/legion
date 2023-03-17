-- Copyright 2021 Stanford University
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

-- Inspired by https://github.com/ParRes/Kernels/tree/master/LEGION/Stencil

import "regent"

local c = regentlib.c

local map_locally = false
do
  local cstring = terralib.includec("string.h")
  for _, arg in ipairs(arg) do
    if cstring.strcmp(arg, "-map_locally") == 0 then
      map_locally = true
      break
    end
  end
end

do
  local root_dir = arg[0]:match(".*/") or "./"

  local include_path = ""
  local include_dirs = terralib.newlist()
  include_dirs:insert("-I")
  include_dirs:insert(root_dir)
  for path in string.gmatch(os.getenv("INCLUDE_PATH"), "[^;]+") do
    include_path = include_path .. " -I " .. path
    include_dirs:insert("-I")
    include_dirs:insert(path)
  end

  local mapper_cc = root_dir .. "stencil_slow_mapper.cc"
  if os.getenv('OBJNAME') then
    local out_dir = os.getenv('OBJNAME'):match('.*/') or './'
    mapper_so = out_dir .. "libstencil_slow_mapper.so"
  elseif os.getenv('SAVEOBJ') == '1' then
    mapper_so = root_dir .. "libstencil_slow_mapper.so"
  else
    mapper_so = os.tmpname() .. ".so" -- root_dir .. "stencil_mapper.so"
  end
  local cxx = os.getenv('CXX') or 'c++'

  local cxx_flags = os.getenv('CXXFLAGS') or ''
  -- cxx_flags = cxx_flags .. " -O2 -Wall -Werror"
  cxx_flags = cxx_flags .. " -O2 -Wall"
  if map_locally then cxx_flags = cxx_flags .. " -DMAP_LOCALLY " end
  if os.execute('test "$(uname)" = Darwin') == 0 then
    cxx_flags =
      (cxx_flags ..
         " -dynamiclib -single_module -undefined dynamic_lookup -fPIC")
  else
    cxx_flags = cxx_flags .. " -shared -fPIC"
  end

  local cmd = (cxx .. " " .. cxx_flags .. " " .. include_path .. " " ..
                 mapper_cc .. " -o " .. mapper_so)
  if os.execute(cmd) ~= 0 then
    print("Error: failed to compile " .. mapper_cc)
    assert(false)
  end
  regentlib.linklibrary(mapper_so)
  cmapper = terralib.includec("stencil_slow_mapper.h", include_dirs)
end

local min = regentlib.fmin
local max = regentlib.fmax

fspace point {
  input : double,
  output : double,
}

task make_tile_partition(points : region(ispace(int2d), point),
                         tiles : ispace(int2d),
                         n : int64, nt : int64)
  var coloring = c.legion_domain_point_coloring_create()
  for i in tiles do
    var lo = int2d { x = i.x * n / nt, y = i.y * n / nt }
    var hi = int2d { x = (i.x + 1) * n / nt - 1, y = (i.y + 1) * n / nt - 1 }
    var rect = rect2d { lo = lo, hi = hi }
    c.legion_domain_point_coloring_color_domain(coloring, i, rect)
  end
  var p = partition(disjoint, points, coloring, tiles)
  c.legion_domain_point_coloring_destroy(coloring)
  return p
end

task make_interior_partition(points : region(ispace(int2d), point),
                         tiles : ispace(int2d),
                         n : int64, nt : int64, radius : int64)
  var coloring = c.legion_domain_point_coloring_create()
  for i in tiles do
    var lo = int2d { x = max(radius, i.x * n / nt), y = max(radius, i.y * n / nt) }
    var hi = int2d { x = min(n - radius, (i.x + 1) * n / nt) - 1, y = min(n - radius, (i.y + 1) * n / nt) - 1 }
    var rect = rect2d { lo = lo, hi = hi }
    c.legion_domain_point_coloring_color_domain(coloring, i, rect)
  end
  var p = partition(disjoint, points, coloring, tiles)
  c.legion_domain_point_coloring_destroy(coloring)
  return p
end

task make_bloated_partition(points : region(ispace(int2d), point),
                            tiles : ispace(int2d),
                            private : partition(disjoint, points, tiles),
                            radius : int)
  var coloring = c.legion_domain_point_coloring_create()
  for i in tiles do
    var pts = private[i]
    var rect = pts.bounds
    var bloated = rect2d { lo = rect.lo - { x = radius, y = radius },
                           hi = rect.hi + { x = radius, y = radius } }
    c.legion_domain_point_coloring_color_domain(coloring, i, bloated)
  end
  var p = partition(aliased, points, coloring, tiles)
  c.legion_domain_point_coloring_destroy(coloring)
  return p
end

local function off(i, x, y)
  return rexpr int2d { x = i.x + x, y = i.y + y } end
end

local function make_stencil_pattern(points, index, off_x, off_y, radius)
  local value
  for i = 1, radius do
    local neg = off_x < 0 or off_y < 0
    local coeff = ((neg and -1) or 1)/(2*i*radius)
    local x, y = off_x*i, off_y*i
    local component = rexpr coeff*points[ [off(index, x, y)] ].input end
    if value then
      value = rexpr value + component end
    else
      value = rexpr component end
    end
  end
  return value
end

local function make_stencil(radius)
  local task st(private : region(ispace(int2d), point), ghost : region(ispace(int2d), point))
  where reads writes(private.output), reads(ghost.input) do
    for i in private do
      private[i].output = private[i].output +
        [make_stencil_pattern(ghost, i,  0, -1, radius)] +
        [make_stencil_pattern(ghost, i, -1,  0, radius)] +
        [make_stencil_pattern(ghost, i,  1,  0, radius)] +
        [make_stencil_pattern(ghost, i,  0,  1, radius)]
    end
  end
  return st
end

local RADIUS = 2
local stencil = make_stencil(RADIUS)

task increment(points : region(ispace(int2d), point))
where reads writes(points.input) do
  for i in points do
    points[i].input = points[i].input + 1
  end
end

task check(points : region(ispace(int2d), point), tsteps : int64, init : int64)
where reads(points.{input, output}) do
  var expect_in = init + tsteps
  var expect_out = init
  for i in points do
    if points[i].input ~= expect_in then
      for i2 in points do
        c.printf("input (%lld,%lld): %.0f should be %lld\n",
                 i2.x, i2.y, points[i2].input, expect_in)
      end
    end
    regentlib.assert(points[i].input == expect_in, "test failed")
    if points[i].output ~= expect_out then
      for i2 in points do
        c.printf("output (%lld,%lld): %.0f should be %lld\n",
                 i2.x, i2.y, points[i2].output, expect_out)
      end
    end
    regentlib.assert(points[i].output == expect_out, "test failed")
  end
end

task main()
  var nbloated : int64 = 12 -- Grid size along each dimension, including border.
  var nt : int64 = 4
  var tsteps : int64 = 10
  var init : int64 = 1000

  var n = nbloated -- Continue to use bloated grid size below.
  regentlib.assert(n >= nt, "grid too small")

  var radius : int64 = RADIUS
  var grid = ispace(int2d, { x = n, y = n })
  var tiles = ispace(int2d, { x = nt, y = nt })

  var points = region(grid, point)
  var private = make_tile_partition(points, tiles, n, nt)
  var interior = make_interior_partition(points, tiles, n, nt, radius)
  var ghost = make_bloated_partition(points, tiles, interior, radius)

  fill(points.{input, output}, init)

  for t = 0, tsteps do
    for i in tiles do
      stencil(interior[i], ghost[i])
    end
    for i in tiles do
      increment(private[i])
    end
  end

  for i in tiles do
    check(interior[i], tsteps, init)
  end
end
-- regentlib.start(main)

if os.getenv('SAVEOBJ') == '1' then
  local root_dir = arg[0]:match(".*/") or "./"
  local out_dir = (os.getenv('OBJNAME') and os.getenv('OBJNAME'):match('.*/')) or root_dir
  local link_flags = terralib.newlist({"-L" .. out_dir, "-lstencil_slow_mapper"})
  -- if USE_FOREIGN then
  --   link_flags:insert("-lstencil")
  -- end

  if os.getenv('STANDALONE') == '1' then
    os.execute('cp ' .. os.getenv('LG_RT_DIR') .. '/../bindings/regent/' ..
        regentlib.binding_library .. ' ' .. out_dir)
  end

  local exe = os.getenv('OBJNAME') or "introexp"
  regentlib.saveobj(main, exe, "executable", cmapper.register_mappers2, link_flags)
else
  regentlib.start(main, cmapper.register_mappers2)
end

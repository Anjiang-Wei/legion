-- Copyright 2022 Stanford University
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

import "regent"

task main()
  regentlib.assert(max(3, 5) == 5, "test failed")
  regentlib.assert(min(3, 5) == 3, "test failed")
  regentlib.assert(max(4.5, 5.5) == 5.5, "test failed")
  regentlib.assert(min(4.5, 5.5) == 4.5, "test failed")
  regentlib.assert(max(4.5, 7) == 7, "test failed")
  regentlib.assert(min(4.5, 7) == 4.5, "test failed")
end
regentlib.start(main)

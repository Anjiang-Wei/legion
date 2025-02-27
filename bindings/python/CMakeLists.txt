#------------------------------------------------------------------------------#
# Copyright 2022 Stanford University
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.1)
project(LegionBindings_python)

# Only search if were building stand-alone and not as part of Legion
if(NOT Legion_SOURCE_DIR)
  find_package(Legion REQUIRED)
endif()

add_executable(legion_python
  main.cc
)
set_target_properties(legion_python PROPERTIES OUTPUT_NAME "legion_python${INSTALL_SUFFIX}")
target_link_libraries(legion_python Legion::Legion)
if(BINDINGS_DEFAULT_MODULE)
  target_compile_definitions(legion_python PRIVATE "BINDINGS_DEFAULT_MODULE=${BINDINGS_DEFAULT_MODULE}")
endif()
install(TARGETS legion_python
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Allow passing extra args to `setup.py` on install like
# `--single-version-externally-managed --root=/`
# to work around setuptools changes.
if (NOT DEFINED Legion_PYTHON_EXTRA_INSTALL_ARGS)
  set(Legion_PYTHON_EXTRA_INSTALL_ARGS "")
endif()
# Use setup.py script to install the Python bindings library
install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/setup.py install --cmake-build-dir ${Legion_BINARY_DIR}/runtime --prefix \"\${CMAKE_INSTALL_PREFIX}\" ${Legion_PYTHON_EXTRA_INSTALL_ARGS} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello.py
  DESTINATION ${CMAKE_INSTALL_DATADIR}/Legion/python/examples
)

if(Legion_ENABLE_TESTING)
  add_test(NAME legion_python COMMAND ${Legion_TEST_LAUNCHER} $<TARGET_FILE:legion_python> hello ${Legion_TEST_ARGS} -ll:py 1 -ll:cpu 0)
  set_tests_properties(
    legion_python
    PROPERTIES
      ENVIRONMENT PYTHONPATH="${CMAKE_INSTALL_FULL_DATADIR}/Legion/python:${CMAKE_INSTALL_FULL_DATADIR}/Legion/python/examples")
endif()

set (_PYTHON_PREFIX Python2)

set (_Python2_REQUIRED_VERSION_MAJOR 2)

include (${CMAKE_CURRENT_LIST_DIR}/FindPython/Support.cmake)

if (COMMAND __Python2_add_library)
  macro (Python2_add_library)
    __Python2_add_library (Python2 ${ARGV})
  endmacro()
endif()

unset (_PYTHON_PREFIX)

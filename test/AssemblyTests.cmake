set(CLANG_SUPPORTED_VERSION "5.0.0")
set(GCC_SUPPORTED_VERSION "5.5.0")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL ${CLANG_SUPPORTED_VERSION})
    message (WARNING
      "Unsupported Clang version " ${CMAKE_CXX_COMPILER_VERSION}
      ". Expected is " ${CLANG_SUPPORTED_VERSION}
      ". Assembly tests may be broken.")
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL ${GCC_SUPPORTED_VERSION})
    message (WARNING
      "Unsupported GCC version " ${CMAKE_CXX_COMPILER_VERSION}
      ". Expected is " ${GCC_SUPPORTED_VERSION}
      ". Assembly tests may be broken.")
  endif()
else()
  message (WARNING "Unsupported compiler. Assembly tests may be broken.")
endif()

include(split_list)

set(ASM_TEST_FLAGS "")
check_cxx_compiler_flag(-O3 BENCHMARK_HAS_O3_FLAG)
if (BENCHMARK_HAS_O3_FLAG)
  list(APPEND ASM_TEST_FLAGS -O3)
endif()

check_cxx_compiler_flag(-g0 BENCHMARK_HAS_G0_FLAG)
if (BENCHMARK_HAS_G0_FLAG)
  list(APPEND ASM_TEST_FLAGS -g0)
endif()

check_cxx_compiler_flag(-fno-stack-protector BENCHMARK_HAS_FNO_STACK_PROTECTOR_FLAG)
if (BENCHMARK_HAS_FNO_STACK_PROTECTOR_FLAG)
  list(APPEND ASM_TEST_FLAGS -fno-stack-protector)
endif()

split_list(ASM_TEST_FLAGS)
string(TOUPPER "${CMAKE_CXX_COMPILER_ID}" ASM_TEST_COMPILER)

# Create a compiler version to match in FileCheck's script
string(REPLACE "." "_" ASM_TEST_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
string(REGEX MATCH "^[0-9]+" ASM_TEST_COMPILER_VER1 "${ASM_TEST_COMPILER_VERSION}")
string(REGEX MATCH "^[0-9]+\_[0-9]+" ASM_TEST_COMPILER_VER2 "${ASM_TEST_COMPILER_VERSION}")

macro(add_filecheck_test name)
  cmake_parse_arguments(ARG "" "" "CHECK_PREFIXES" ${ARGV})

  # The main problem here is that the -S flag does not work as clang/gcc in other 
  # compilers like nvc++ so just add the necessary flags
  add_library(${name} OBJECT ${name}.cc)
  target_link_libraries(${name} PRIVATE benchmark::benchmark)
  set_target_properties(${name} PROPERTIES COMPILE_FLAGS "${ASM_TEST_FLAGS}")

  set(ASM_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${name}.s")

  # Replace the compiler-dependent assembly generation + python script
  # with objdump + sed script, which seems cleaner
  add_custom_target(
      assembly_${name} ALL
      COMMAND 
        objdump -d --no-show-raw-insn -M att $<TARGET_OBJECTS:${name}> | 
        sed -E -f ${PROJECT_SOURCE_DIR}/tools/strip_asm.sed 
        > ${ASM_OUTPUT_FILE} 
      DEPENDS
        ${name}
      BYPRODUCTS 
        ${ASM_OUTPUT_FILE}
       )
  add_dependencies(assembly_${name} ${name})
  if (NOT ARG_CHECK_PREFIXES)
    set(ARG_CHECK_PREFIXES "CHECK" )
  endif()
  foreach(prefix ${ARG_CHECK_PREFIXES})
    add_test(NAME run_${name}_${prefix}
        COMMAND
          ${LLVM_FILECHECK_EXE} ${name}.cc
          --allow-unused-prefixes
          --input-file=${ASM_OUTPUT_FILE}
          --check-prefixes=CHECK,CHECK-${ASM_TEST_COMPILER},CHECK-${ASM_TEST_COMPILER}-${ASM_TEST_COMPILER_VERSION},CHECK-${ASM_TEST_COMPILER}-${ASM_TEST_COMPILER_VER1},CHECK-${ASM_TEST_COMPILER}-${ASM_TEST_COMPILER_VER2}      
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endforeach()
endmacro()


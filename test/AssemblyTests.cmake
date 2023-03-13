set(CLANG_SUPPORTED_VERSION "5.0.0")
set(GCC_SUPPORTED_VERSION "5.5.0")
set(NVHPC_SUPPORTED_VERSION "23.1")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL ${CLANG_SUPPORTED_VERSION})
    message (WARNING
      "Unsupported Clang version " ${CMAKE_CXX_COMPILER_VERSION}
      ". Expected is " ${CLANG_SUPPORTED_VERSION}
      ". Assembly tests may be broken.")
  else()
    set(BENCHMARK_ASSEMBLY_TESTS_SUPPORTED TRUE)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL ${GCC_SUPPORTED_VERSION})
    message (WARNING
      "Unsupported GCC version " ${CMAKE_CXX_COMPILER_VERSION}
      ". Expected is " ${GCC_SUPPORTED_VERSION}
      ". Assembly tests may be broken.")
  else()
    set(BENCHMARK_ASSEMBLY_TESTS_SUPPORTED TRUE)
  endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "NVHPC" ) 
  set(BENCHMARK_ASSEMBLY_TESTS_SUPPORTED FALSE)

else()
  set(BENCHMARK_ASSEMBLY_TESTS_SUPPORTED FALSE)
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
# Create a compiler + major version to match against in tests
string(REGEX MATCH "^[0-9]+" ASM_TEST_COMPILER_VER1 "${ASM_TEST_COMPILER_VERSION}")

# Find objdump in the current host
find_program( OBJDUMP objdump )

macro(add_filecheck_test name)
  cmake_parse_arguments(ARG "" "" "CHECK_PREFIXES" ${ARGV})
  add_library(${name} OBJECT ${name}.cc)
  target_link_libraries(${name} PRIVATE benchmark::benchmark)
  set_target_properties(${name} PROPERTIES COMPILE_FLAGS "-S ${ASM_TEST_FLAGS}")
  set(ASM_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${name}.s")
  add_custom_target(copy_${name} ALL
      COMMAND ${PROJECT_SOURCE_DIR}/tools/strip_asm.py
        $<TARGET_OBJECTS:${name}>
        ${ASM_OUTPUT_FILE}
      BYPRODUCTS ${ASM_OUTPUT_FILE})
      add_dependencies(copy_${name} ${name})

  if ( DEFINED OBJDUMP )
    # The main problem here is that the -S flag does not work as clang/gcc in other 
    # compilers like nvc++ so just add the necessary flags
    add_library(${name}_normalized OBJECT ${name}.cc)
    target_link_libraries(${name}_normalized PRIVATE benchmark::benchmark)
    set_target_properties(${name}_normalized PROPERTIES COMPILE_FLAGS "${ASM_TEST_FLAGS}")
          
    # Add a normalized version with objdump + sed script
    add_custom_target(
        assembly_${name} ALL
        COMMAND 
          ${OBJDUMP} -dClG --no-show-raw-insn --section=.text -M att 
            $<TARGET_OBJECTS:${name}_normalized> | 
          sed -E -f ${PROJECT_SOURCE_DIR}/tools/strip_asm.sed 
          > ${ASM_OUTPUT_FILE}.normalized 
        DEPENDS
          ${name}
        BYPRODUCTS 
          ${ASM_OUTPUT_FILE}.normalized
          )
    add_dependencies(assembly_${name} ${name}_normalized)
  endif()
  if (NOT ARG_CHECK_PREFIXES)
    set(ARG_CHECK_PREFIXES "CHECK")
  endif()
  # At this time these tests will verified work on clang and gcc
  if ( BENCHMARK_ASSEMBLY_TESTS_SUPPORTED )
    foreach(prefix ${ARG_CHECK_PREFIXES})
    add_test(NAME run_${name}_${prefix}
      COMMAND
        ${LLVM_FILECHECK_EXE} ${name}.cc
        --input-file=${ASM_OUTPUT_FILE}
        --allow-unused-prefixes 
        --check-prefixes=CHECK,CHECK-${ASM_TEST_COMPILER}     
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach()
  endif()
  if ( DEFINED OBJDUMP ) 
    # Add new test with normalized names
    add_test(NAME run_normalized_${name}
        COMMAND
          ${LLVM_FILECHECK_EXE} ${name}.cc
          --dump-input=always
          --allow-unused-prefixes 
          --input-file=${ASM_OUTPUT_FILE}.normalized
          --check-prefixes=NORM,NORM-${ASM_TEST_COMPILER},NORM-${ASM_TEST_COMPILER}-${ASM_TEST_COMPILER_VERSION},NORM-${ASM_TEST_COMPILER}-${ASM_TEST_COMPILER_VER1},NORM-${ASM_TEST_COMPILER}-${ASM_TEST_COMPILER_VER2}      
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
endmacro()


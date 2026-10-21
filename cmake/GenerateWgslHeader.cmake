if(NOT DEFINED WGSL_INPUTS OR NOT DEFINED WGSL_OUTPUT OR NOT DEFINED WGSL_VARNAME)
    message(FATAL_ERROR "WGSL_INPUTS, WGSL_OUTPUT, and WGSL_VARNAME must be set")
endif()

string(REPLACE "|" ";" WGSL_INPUTS "${WGSL_INPUTS}")

set(contents "")
foreach(wgsl_file IN LISTS WGSL_INPUTS)
    file(READ "${wgsl_file}" chunk)
    string(APPEND contents "${chunk}\n")
endforeach()

get_filename_component(output_dir "${WGSL_OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
file(WRITE "${WGSL_OUTPUT}"
    "#pragma once\n\nstatic const char ${WGSL_VARNAME}[] = R\"WGSL(\n${contents})WGSL\";\n")

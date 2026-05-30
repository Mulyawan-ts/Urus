# Verify that `urusc --emit-c FILE` is byte-deterministic across runs.
#
# This is a foundation invariant: reproducible builds, sane CI diffs, and
# the upcoming sanitizer matrix all rely on the compiler not emitting
# different C source for the same input. If a hash map iteration order,
# a fresh PID, or a `rand()` ever leaks into the emitted bytes, this
# test catches it before it ships.
#
# Required parameters: URUSC, FILE, WORKING_DIR

get_filename_component(TEST_NAME ${FILE} NAME_WE)
set(OUT1 "${WORKING_DIR}/${TEST_NAME}_det1.c")
set(OUT2 "${WORKING_DIR}/${TEST_NAME}_det2.c")

execute_process(
    COMMAND ${URUSC} ${FILE} --emit-c
    OUTPUT_FILE ${OUT1}
    RESULT_VARIABLE R1
    ERROR_VARIABLE E1
)
if(NOT R1 EQUAL 0)
    message(FATAL_ERROR "First urusc run failed:\n${E1}")
endif()

execute_process(
    COMMAND ${URUSC} ${FILE} --emit-c
    OUTPUT_FILE ${OUT2}
    RESULT_VARIABLE R2
    ERROR_VARIABLE E2
)
if(NOT R2 EQUAL 0)
    message(FATAL_ERROR "Second urusc run failed:\n${E2}")
endif()

file(SHA256 ${OUT1} H1)
file(SHA256 ${OUT2} H2)

if(NOT "${H1}" STREQUAL "${H2}")
    # On mismatch, show a short diff hint so the failure is debuggable
    # without re-running by hand.
    file(READ ${OUT1} C1 LIMIT 4096)
    file(READ ${OUT2} C2 LIMIT 4096)
    message(FATAL_ERROR
        "Codegen non-deterministic for ${FILE}:\n"
        "  run 1 sha256: ${H1}\n"
        "  run 2 sha256: ${H2}\n"
        "First 4 KB of run 1:\n${C1}\n"
        "---\nFirst 4 KB of run 2:\n${C2}")
endif()

file(REMOVE ${OUT1} ${OUT2})

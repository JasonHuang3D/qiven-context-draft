function(qiven_context_draft_enable_strong_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    endif()
endfunction()

add_executable(qiven-context-draft-cognition cognition_value_tree.cpp)
target_link_libraries(qiven-context-draft-cognition PRIVATE qiven::context-draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-cognition)
add_test(NAME qiven-context-draft-cognition COMMAND qiven-context-draft-cognition)

add_executable(qiven-context-draft-memory-leak memory_leak_test.cpp)
target_link_libraries(qiven-context-draft-memory-leak PRIVATE qiven::context-draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-memory-leak)
set_target_properties(qiven-context-draft-memory-leak PROPERTIES CXX_EXTENSIONS OFF FOLDER "Tests")
add_test(NAME qiven-context-draft.memory-leak COMMAND qiven-context-draft-memory-leak)

add_executable(qiven-context-draft-grant-regression grant_regression.cpp)
target_link_libraries(qiven-context-draft-grant-regression PRIVATE qiven::context-draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-grant-regression)

set_target_properties(
    qiven-context-draft-grant-regression
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "Tests"
)

add_test(NAME qiven-context-draft-grant-regression COMMAND qiven-context-draft-grant-regression)

add_executable(qiven-context-draft-persistence persistence_fencing.cpp)
target_link_libraries(qiven-context-draft-persistence PRIVATE qiven::context-draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-persistence)
add_test(NAME qiven-context-draft-persistence COMMAND qiven-context-draft-persistence)

add_executable(qiven-context-draft-store store_contract.cpp)
target_link_libraries(qiven-context-draft-store PRIVATE qiven::context-draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-store)
add_test(NAME qiven-context-draft-store COMMAND qiven-context-draft-store)

set_target_properties(
    qiven-context-draft-cognition
    qiven-context-draft-persistence
    qiven-context-draft-store
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "Tests"
)

add_executable(qiven-context-draft-participant participant_model.cpp)
target_link_libraries(qiven-context-draft-participant PRIVATE qiven::context-draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-participant)
set_target_properties(qiven-context-draft-participant
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "Tests"
)
add_test(NAME qiven-context-draft-participant COMMAND qiven-context-draft-participant)

add_executable(qiven-context-draft-control-plane control_plane.cpp)
target_link_libraries(qiven-context-draft-control-plane PRIVATE qiven::context-draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-control-plane)
set_target_properties(qiven-context-draft-control-plane
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "Tests"
)
add_test(NAME qiven-context-draft-control-plane COMMAND qiven-context-draft-control-plane)

add_executable(qiven-context-draft-tool-retry tool_and_retry.cpp)
target_link_libraries(qiven-context-draft-tool-retry PRIVATE qiven::context-draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-tool-retry)
set_target_properties(qiven-context-draft-tool-retry
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "Tests"
)
add_test(NAME qiven-context-draft-tool-retry COMMAND qiven-context-draft-tool-retry)

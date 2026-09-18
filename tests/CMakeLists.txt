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

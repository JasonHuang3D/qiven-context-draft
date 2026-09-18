add_executable(qiven-context-draft-loop cognition_loop.cpp)
target_link_libraries(qiven-context-draft-loop PRIVATE qiven::context-draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-loop)

set_target_properties(
    qiven-context-draft-loop
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "Apps"
)

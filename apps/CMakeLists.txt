add_executable(qiven-context-draft-loop cognition_loop.cpp)
target_link_libraries(qiven-context-draft-loop PRIVATE qiven::context_draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-loop)

set_target_properties(
    qiven-context-draft-loop
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "Apps"
)

add_executable(qiven-context-draft-artifact-trial artifact_trial.cpp)
target_link_libraries(qiven-context-draft-artifact-trial PRIVATE qiven::context_draft)
qiven_context_draft_enable_strong_warnings(qiven-context-draft-artifact-trial)

set_target_properties(
    qiven-context-draft-artifact-trial
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "Apps"
)

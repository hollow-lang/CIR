add_library(example SHARED example.cpp)
set_target_properties(example PROPERTIES
        OUTPUT_NAME example
        POSITION_INDEPENDENT_CODE ON
)
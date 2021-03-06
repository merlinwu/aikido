set(sources
  SO2Angular.cpp
  RnEuclidean.cpp
  SO3Angular.cpp
  Weighted.cpp
  defaults.cpp
)

add_library("${PROJECT_NAME}_distance" SHARED ${sources})
target_include_directories("${PROJECT_NAME}_distance" SYSTEM
  PUBLIC ${DART_INCLUDE_DIRS}
)
target_link_libraries("${PROJECT_NAME}_distance"
  PUBLIC
    "${PROJECT_NAME}_statespace"
    ${DART_LIBRARIES}
)
target_compile_options("${PROJECT_NAME}_distance"
  PUBLIC ${AIKIDO_CXX_STANDARD_FLAGS}
)

add_component(${PROJECT_NAME} distance)
add_component_targets(${PROJECT_NAME} distance "${PROJECT_NAME}_distance")
add_component_dependencies(${PROJECT_NAME} distance statespace)

coveralls_add_sources(${sources})
format_add_sources(${sources})

#==============================================================================
# Dependencies
#
if(NOT DART_utils-urdf_FOUND)
  message(STATUS "Looking for DART component 'dart-utils-urdf' - NOT found, "
      "to use perception, please install dart-utils-urdf")
  return()
endif()

find_package(geometry_msgs QUIET)
aikido_check_package(geometry_msgs "perception" "geometry_msgs")

find_package(roscpp QUIET)
aikido_check_package(roscpp "perception" "roscpp")

find_package(std_msgs QUIET)
aikido_check_package(std_msgs "perception" "std_msgs")

find_package(visualization_msgs QUIET)
aikido_check_package(visualization_msgs "perception" "visualization_msgs")

# TODO: Workaround because find_package(tf) imports "tests" and "run_tests"
# targets that conflict with the targets defined by Aikido.
find_path(tf_INCLUDE_DIRS tf PATHS "/opt/ros/indigo/include"
  PATH_SUFFIXES "interactive_markeres/interactive_marker_client.h")
find_library(tf_LIBRARIES tf PATHS "/opt/ros/indigo/lib")
evaluate_condition(tf_FOUND tf_INCLUDE_DIRS AND tf_LIBRARIES)
aikido_check_package(tf "perception" "tf")

#==============================================================================
# Libraries
#
set(sources
  AprilTagsModule.cpp
  shape_conversions.cpp
  YamlAprilTagsDatabase.cpp
)

add_library("${PROJECT_NAME}_perception" SHARED ${sources})
target_include_directories("${PROJECT_NAME}_perception" SYSTEM
  PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${DART_INCLUDE_DIRS}
    ${geometry_msgs_INCLUDE_DIRS}
    ${roscpp_INCLUDE_DIRS}
    ${std_msgs_INCLUDE_DIRS}
    ${tf_INCLUDE_DIRS}
  PRIVATE
    ${visualization_msgs_INCLUDE_DIRS}
)
target_link_libraries("${PROJECT_NAME}_perception"
  PUBLIC
    "${PROJECT_NAME}_util"
    ${DART_LIBRARIES}
    ${geometry_msgs_LIBRARIES}
    ${roscpp_LIBRARIES}
    ${std_msgs_LIBRARIES}
    ${tf_LIBRARIES}
  PRIVATE
    ${visualization_msgs_LIBRARIES}
)
target_compile_options("${PROJECT_NAME}_perception"
  PUBLIC ${AIKIDO_CXX_STANDARD_FLAGS}
)

add_component(${PROJECT_NAME} perception)
add_component_targets(${PROJECT_NAME} perception "${PROJECT_NAME}_perception")

coveralls_add_sources(${sources})

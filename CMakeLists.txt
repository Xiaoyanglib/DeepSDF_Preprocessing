############################################################
cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
project("DeepSDF_preprocess")
############################################################

set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/third-party")
#find_package(CLI11 CONFIG)
find_package(Eigen3 REQUIRED)
#find_package(nanoflann)
find_package(Pangolin REQUIRED)

add_subdirectory(third-party/cnpy)

set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "Configurations" FORCE)

set(SOURCES
    src/ShaderProgram.cpp
    src/Utils.cpp)

add_executable(PreprocessMesh src/PreprocessMesh.cpp ${SOURCES})
target_include_directories(PreprocessMesh PRIVATE 
	third-party/cnpy 
	third-party/cli11/include
	third-party/glew/include 
	third-party/nanoflann/include
	third-party/zlib/include
)

target_link_libraries(PreprocessMesh PRIVATE 
	pango_core 
	pango_display 
	pango_geometry 
	pango_glgeometry 
	cnpy 
	Eigen3::Eigen)
set_target_properties(PreprocessMesh PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_executable(SampleVisibleMeshSurface src/SampleVisibleMeshSurface.cpp ${SOURCES})
target_include_directories(SampleVisibleMeshSurface PRIVATE 
	third-party/cnpy
	third-party/cli11/include 
	third-party/glew/include 
	third-party/nanoflann/include
	third-party/zlib/include
)

target_link_libraries(SampleVisibleMeshSurface PRIVATE 
	pango_core 
	pango_display 
	pango_geometry 
	pango_glgeometry 
	cnpy 
	Eigen3::Eigen)
set_target_properties(SampleVisibleMeshSurface PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

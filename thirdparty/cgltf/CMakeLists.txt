find_package(cgltf)

find_path(CGLTF_INCLUDE_DIRS "cgltf.h" "cgltf_write.h")
add_library(cgltf STATIC ${CGLTF_INCLUDE_DIRS} "cgltf.cpp")

target_compile_features(cgltf PUBLIC cxx_std_20)
target_include_directories(cgltf PUBLIC ${CGLTF_INCLUDE_DIRS})
set_target_properties(cgltf PROPERTIES FOLDER thirdparty)
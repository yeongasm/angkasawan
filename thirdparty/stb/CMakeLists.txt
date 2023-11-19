find_package(stb)

find_path(STB_INCLUDE_DIRS "stb_image.h" "stb_image_write.h")
add_library(stb STATIC ${STB_INCLUDE_DIRS} "stb.cpp")

target_compile_features(stb PUBLIC cxx_std_20)
target_include_directories(stb PUBLIC ${STB_INCLUDE_DIRS})
set_target_properties(stb PROPERTIES FOLDER thirdparty)
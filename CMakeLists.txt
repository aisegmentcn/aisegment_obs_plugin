project(wx_seg_obs VERSION 1.0)
#find_package(obs REQUIRED)
add_library(wx_seg_obs MODULE)
add_library(OBS::wx_seg_obs ALIAS wx_seg_obs)

#if(OS_WINDOWS)
#    target_link_options(wx_seg_obs PRIVATE "LINKER:/LTCG")
#endif()

if (MSVC)
#add_definitions(/MP /d2FH4- /std:c++latest )
add_definitions(/std:c++latest) #(/std:c++latest /MT)
endif()

#SET(CMAKE_EXE_LINKER_FLAGS /NODEFAULTLIB:"libcmt.lib;libxxx2.lib;[...]")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Md")

if(MSVC)
	set_property(TARGET wx_seg_obs APPEND PROPERTY LINK_FLAGS "/NODEFAULTLIB:libcmt")
endif()


include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/3rd/lh320_windows_1.2.2/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/3rd/opencv/include"
)


target_sources(wx_seg_obs PRIVATE  
        src/wxsegobs.cpp 
        src/plugin-main.cpp 
        src/plugin-macros.generated.h  
        ) 


target_link_libraries(wx_seg_obs PRIVATE OBS::libobs)

#if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
target_link_libraries(wx_seg_obs PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/lh320_windows_1.2.2/lib/x64/wx_video_seg.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/opencv/x64/vc16/staticlib/opencv_world454.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/opencv/x64/vc16/staticlib/zlib.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/opencv/x64/vc16/staticlib/libpng.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/opencv/x64/vc16/staticlib/libtiff.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/opencv/x64/vc16/staticlib/IlmImf.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/opencv/x64/vc16/staticlib/libjpeg-turbo.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/opencv/x64/vc16/staticlib/libwebp.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/opencv/x64/vc16/staticlib/libopenjp2.lib"
)

#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd")
set_target_properties(wx_seg_obs PROPERTIES FOLDER "plugins" PREFIX "")

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "wx_seg_obs")
endif()

setup_plugin_target(wx_seg_obs)


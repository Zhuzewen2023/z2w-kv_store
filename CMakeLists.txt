cmake_minimum_required(VERSION 3.10)

project(z2w-kv_store VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(COMMON_SOURCES
    kv_mem.c
    kv_store_array.c
    ntyco_server.c
    kv_store.cpp
)
set(TEST_SOURCES test_kv.c)

#```cmake
#add_library(your_lib_precompiled STATIC IMPORTED)
#```
#- `add_library`: 这是CMake中用于添加一个库目标的命令。
#- `your_lib_precompiled`: 这是你给这个导入的库目标起的名字，
#在后续的CMake命令中（如`target_link_libraries`）会用到这个名字。
#- `STATIC`: 表示这是一个静态库。如果库是动态库（共享库），则应使用`SHARED`。
#- `IMPORTED`: 这个关键字表示这个库是外部的、预先编译好的，
#而不是由当前CMake项目构建的。
add_library(ntyco STATIC IMPORTED)

#```cmake
#set_target_properties(your_lib_precompiled PROPERTIES
#    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/your_lib/lib/your_lib.a
#)
#```
#- `set_target_properties`: 这个命令用于设置目标（这里是`your_lib_precompiled`）的属性。
#- `PROPERTIES`: 后面跟着要设置的属性列表。
#- `IMPORTED_LOCATION`: 这是一个属性，用于指定预编译库文件的实际路径。
#- `${CMAKE_SOURCE_DIR}`: 这是CMake变量，表示项目根目录的路径。
set_target_properties(ntyco PROPERTIES 
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/NtyCo/libntyco.a
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/NtyCo/core"
    )

#### 3. 设置头文件包含目录
#```cmake
#target_include_directories(your_lib_precompiled INTERFACE
#    ${CMAKE_SOURCE_DIR}/libs/your_lib/include
#)
#```
#- `target_include_directories`: 这个命令用于指定目标（库或可执行文件）
#的头文件包含目录。
#- `INTERFACE`: 表示这个包含目录是接口性质的，
#即任何链接了这个库（`your_lib_precompiled`）的目标（比如你的可执行文件）都会自动添加这个包含目录。这样，你在使用这个库时，就可以直接包含它的头文件，而不需要手动添加头文件路径。
#- `${CMAKE_SOURCE_DIR}/libs/your_lib/include`: 
#这是头文件所在的目录路径。假设你的库的头文件都放在这个目录下。
# target_include_directories(ntyco INTERFACE ${CMAKE_SOURCE_DIR}/NtyCo/core)

add_executable(kv ${COMMON_SOURCES})

add_executable(kv_test ${TEST_SOURCES})
### 4. 将预编译库链接到可执行文件
#```cmake
#target_link_libraries(main_app PRIVATE your_lib_precompiled)
#```
#- `target_link_libraries`: 这个命令用于将库链接到目标（这里是可执行文件`main_app`）。
#- `main_app`: 这是你的可执行文件目标（由`add_executable(main_app ...)`命令创建）。
#- `PRIVATE`: 表示这个链接关系是私有的。
#这意味着`your_lib_precompiled`只会链接到`main_app`，
#而不会传播给其他可能依赖于`main_app`的目标。
#如果这个库是`main_app`内部使用，并且不需要暴露给其他目标，使用`PRIVATE`是合适的。
#- `your_lib_precompiled`: 这是之前导入的预编译库目标。
target_link_libraries(kv PRIVATE ntyco pthread dl)




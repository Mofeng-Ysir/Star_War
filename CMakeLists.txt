cmake_minimum_required(VERSION 3.16)

# 如果你的环境变量没配置好，可以取消下面这行的注释并修改路径
# set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64")

project(QtSpaceShooter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 解决多个 CL.EXE 进程同时写入 PDB 文件的问题
if(MSVC)
    # 使用 /FS 允许多个编译器实例共享单个 PDB 文件
    string(APPEND CMAKE_CXX_FLAGS " /FS")
    # 或者禁用并行编译（更保险）
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

# --- 关键修改 1: 在源文件名前加上 src/ 前缀 ---
add_executable(QtSpaceShooter
    src/main.cpp
    src/MainWindow.cpp
    src/MenuWidget.cpp
    src/LevelSelectWidget.cpp
    src/PlaneSelectWidget.cpp
    src/GameWidget.cpp
    src/HighScoreWidget.cpp
    src/ScoreManager.cpp
    src/LevelManager.cpp
    src/DataManager.cpp
    src/BossStrategy.cpp
    src/CollisionSystem.cpp
    src/ShopWidget.cpp
    src/EquipmentWidget.cpp
)

# --- 关键修改 2: 把 src 目录加入包含路径 ---
# 这样你在代码里写 #include "GameWidget.h" 时，编译器知道去 src 里找
target_include_directories(QtSpaceShooter PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(QtSpaceShooter PRIVATE 
    Qt6::Widgets 
    Qt6::Multimedia 
    Qt6::MultimediaWidgets
)

# 为目标直接添加 /FS 编译选项
target_compile_options(QtSpaceShooter PRIVATE /FS)

# 复制资源文件夹 (保持不变，从根目录复制到构建目录)
add_custom_command(TARGET QtSpaceShooter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:QtSpaceShooter>/assets
)
cmake_minimum_required(VERSION 3.16)

# 如果你的环境变量没配置好，可以取消下面这行的注释并修改路径
# set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64")

project(QtSpaceShooter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

# --- 关键修改 1: 在源文件名前加上 src/ 前缀 ---
add_executable(QtSpaceShooter
    src/main.cpp
    src/common.h
    src/MainWindow.cpp       src/MainWindow.h
    src/MenuWidget.cpp       src/MenuWidget.h
    src/LevelSelectWidget.cpp src/LevelSelectWidget.h
    src/PlaneSelectWidget.cpp src/PlaneSelectWidget.h
    src/GameWidget.cpp       src/GameWidget.h
    src/HighScoreWidget.cpp  src/HighScoreWidget.h
    src/ScoreManager.cpp     src/ScoreManager.h
    src/LevelManager.cpp     src/LevelManager.h
    src/DataManager.cpp      src/DataManager.h
    src/BossStrategy.cpp     src/BossStrategy.h
    src/CollisionSystem.cpp  src/CollisionSystem.h
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

# 复制资源文件夹 (保持不变，从根目录复制到构建目录)
add_custom_command(TARGET QtSpaceShooter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:QtSpaceShooter>/assets
)
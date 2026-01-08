cmake_minimum_required(VERSION 3.16)

#set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64") # 根据你的实际环境修改

project(QtSpaceShooter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets)

add_executable(QtSpaceShooter
    main.cpp
    MainWindow.cpp MainWindow.h
    MenuWidget.cpp MenuWidget.h
    LevelSelectWidget.cpp LevelSelectWidget.h
    GameWidget.cpp GameWidget.h
    HighScoreWidget.cpp HighScoreWidget.h
    ScoreManager.cpp ScoreManager.h
    LevelManager.cpp LevelManager.h
    BossStrategy.cpp BossStrategy.h      # 新增
    CollisionSystem.cpp CollisionSystem.h # 新增
    common.h
)

target_link_libraries(QtSpaceShooter PRIVATE 
    Qt6::Widgets 
    Qt6::Multimedia 
    Qt6::MultimediaWidgets
)

# 复制资源文件夹到构建目录
add_custom_command(TARGET QtSpaceShooter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:QtSpaceShooter>/assets
)
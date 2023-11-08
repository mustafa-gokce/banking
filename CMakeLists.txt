cmake_minimum_required(VERSION 3.22.1)
project(banking)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

add_executable(banking_client
        src/client/main.cpp)
target_link_libraries(banking_client
        Qt::Core
        Qt::Gui
        Qt::Widgets
        zmq
        msgpackc
)

add_executable(banking_server
        src/server/main.cpp)
target_link_libraries(banking_server
        zmq
        msgpackc
        sqlite3
)

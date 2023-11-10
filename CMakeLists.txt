# set minimum cmake version
cmake_minimum_required(VERSION 3.22.1)

# set project name
project(banking)

# set compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# find packages
find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED
)

# create client executable
add_executable(client
        client.cpp
        Tools.h
        Tools.cpp
)
target_link_libraries(client
        Qt::Core
        Qt::Gui
        Qt::Widgets
        zmq
        msgpackc
)

# create server executable
add_executable(server
        server.cpp
        Tools.h
        Tools.cpp
)
target_link_libraries(server
        zmq
        msgpackc
        sqlite3
)

# create symlink to database
add_custom_command(TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/banking.sqlite
        ${CMAKE_CURRENT_BINARY_DIR}/banking.sqlite
)
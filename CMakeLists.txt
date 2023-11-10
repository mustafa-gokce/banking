# set minimum cmake version
cmake_minimum_required(VERSION 3.22.1)

# set project name
project(banking)

# create client executable
add_executable(client
        client.cpp
        src/Tools.cpp
        src/Tools.h
        src/Client.cpp
        src/Client.h
)
target_link_libraries(client
        zmq
        msgpackc
)

# create server executable
add_executable(server
        server.cpp
        src/Tools.cpp
        src/Tools.h
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
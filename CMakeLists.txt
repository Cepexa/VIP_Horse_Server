cmake_minimum_required(VERSION 3.10)
project(ClientServerBinary)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread)

# Указываем, где искать заголовочные файлы
add_subdirectory(libs)
include_directories(${CMAKE_SOURCE_DIR}/vendor/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Собираем все подкаталоги include
###
file(GLOB_RECURSE PROJECT_INCLUDE_DIRS LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/include/*")

# Фильтруем только директории, удаляя файлы
set(DIR_LIST "")
foreach(dir ${PROJECT_INCLUDE_DIRS})
    if (IS_DIRECTORY ${dir})
        list(APPEND DIR_LIST ${dir})
    endif()
endforeach()

include_directories(${DIR_LIST})
###

# Добавляем статическую библиотеку libpqxx
link_directories(vendor/lib)

file(GLOB_RECURSE SOURCE_FILES 
    src/*.cpp
    main.cpp)

add_executable(server ${SOURCE_FILES})
target_link_libraries(server BinaryProtocol Boost::system Boost::thread)
target_link_libraries(server libClasses)
target_link_libraries(server pqxx pq)
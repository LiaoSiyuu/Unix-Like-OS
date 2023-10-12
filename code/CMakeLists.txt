cmake_minimum_required(VERSION 3.22)
project(OS_Course_Design)

set(CMAKE_CXX_STANDARD 14)

add_executable(OS_Course_Design
        main.cpp Utility.h
        Inode.cpp
        Inode.h
        FileSystem.cpp
        FileSystem.h
        FileManager.cpp
        FileManager.h
        OpenFileManager.cpp
        OpenFileManager.h
        _Kernel.cpp
        _Kernel.h
        BufferManager.cpp
        BufferManager.h
        Buf.h)

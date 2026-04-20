cmake_minimum_required(VERSION 3.15)
project(budget_tracker CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(budget
    src/main.cpp
    src/Profile.cpp
    src/Ledger.cpp
    src/Storage.cpp
    src/Advisor.cpp
    src/CommandParser.cpp
    src/Dashboard.cpp
    src/BudgetApp.cpp
)
target_include_directories(budget PRIVATE src)

if(MSVC)
    target_compile_options(budget PRIVATE /W4)
else()
    target_compile_options(budget PRIVATE -Wall -Wextra)
endif()

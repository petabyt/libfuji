cmake_minimum_required(VERSION 3.10)
project(fp)

option(FP_INCLUDE_CLI "include cli" OFF)

if(FP_INCLUDE_CLI)
	set(CMAKE_C_FLAGS "-g")
	add_executable(test
		${PROJECT_SOURCE_DIR}/src/cli.c
	)
	target_link_libraries(test
		PRIVATE
		fp
	)
endif()

add_library(fp STATIC 
	src/d185.c
	src/data.c
	src/xml.c
	src/ezxml.c
)
target_compile_definitions(fp PRIVATE -DEZXML_NOMMAP)
target_include_directories(fp
	PUBLIC 
	${PROJECT_SOURCE_DIR}/src
	${LIBXML2_INCLUDE_DIRS}
)

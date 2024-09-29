
set(BREAD_LEXER_SRC ${CMAKE_CURRENT_BINARY_DIR}/BreadLexer.cpp)
set(BREAD_PARSER_SRC ${CMAKE_CURRENT_BINARY_DIR}/BreadParser.cpp)

add_custom_command ( 
    OUTPUT ${BREAD_LEXER_SRC} ${BREAD_PARSER_SRC}
    COMMAND antlr4 ${CMAKE_CURRENT_SOURCE_DIR}/Bread.g4 -Dlanguage=Cpp -visitor -o ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(cmp main.cpp ${BREAD_LEXER_SRC} ${BREAD_PARSER_SRC})
target_link_libraries(cmp -lantlr4-runtime -lLLVM-18)
target_include_directories(cmp PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(cmp PUBLIC /usr/include/antlr4-runtime)
target_include_directories(cmp PUBLIC /usr/include/llvm-18)
target_include_directories(cmp PUBLIC /usr/include/llvm-c-18)

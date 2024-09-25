

add_custom_target ( parser_lexer
    COMMAND antlr4 ${CMAKE_CURRENT_SOURCE_DIR}/Bread.g4 -Dlanguage=Cpp -visitor
)

add_executable(cmp main.cpp BreadLexer.cpp BreadParser.cpp)
add_dependencies(cmp parser_lexer)
target_link_libraries(cmp -lantlr4-runtime)
target_include_directories(cmp PUBLIC /usr/include/antlr4-runtime)

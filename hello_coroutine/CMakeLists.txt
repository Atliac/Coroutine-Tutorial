add_executable(hello_coroutine "main.cpp" )

target_precompile_headers(hello_coroutine PRIVATE <iostream>)
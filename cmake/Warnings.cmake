# Strict warnings, treated as errors, for the project's own targets only.
# Third-party targets (raylib, doctest) keep their own flags.
function(horde_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Werror)
    endif()
endfunction()

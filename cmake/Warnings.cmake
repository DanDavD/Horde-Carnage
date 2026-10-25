# Strict warnings, treated as errors, for the project's own targets only.
# Third-party targets (raylib, doctest) keep their own flags.
function(horde_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
        # Designated initializers that name only the fields that differ from their defaults are how the
        # code sets its numbers (`MovementStats{.maxSpeed = 120.0f}`), and the rest come from default member
        # initializers on purpose. Clang's -Wextra flags every one of them, so that one warning is off.
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wno-missing-field-initializers -Werror)
    endif()
endfunction()

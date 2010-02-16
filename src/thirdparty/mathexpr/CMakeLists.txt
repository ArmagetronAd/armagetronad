include_directories (
    ${armagetronad_INCLUDE_DIRS}
    )


add_library (mathexpr STATIC EXCLUDE_FROM_ALL
    mathexpr.h                  mathexpr.cpp
    )


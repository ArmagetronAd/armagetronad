include_directories (
    ${armagetronad_INCLUDE_DIRS}
    )


add_library (scrap STATIC EXCLUDE_FROM_ALL
    scrap.h                     scrap.cpp
    )

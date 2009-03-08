add_subdirectory (mathexpr)
add_subdirectory (particles)
add_subdirectory (scrap)

add_custom_target (thirdparty
    DEPENDS mathexpr particles scrap
    )

configure_file (aiplayers.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/aiplayers.cfg)
configure_file (rc.config.in ${CMAKE_CURRENT_BINARY_DIR}/rc.config)

install (FILES
    ${CMAKE_CURRENT_BINARY_DIR}/aiplayers.cfg
    default.cfg
    master.srv
    settings.cfg
    settings_authentication.cfg
    settings_dedicated.cfg
    settings_visual.cfg
    subcultures.srv

    DESTINATION ${AA_SYSCONFDIR}
    )

install (DIRECTORY examples
    DESTINATION ${AA_SYSCONFDIR}/examples
    FILES_MATCHING PATTERN "*.cfg")


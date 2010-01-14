configure_file (armagetronad.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/armagetronad.desktop)
configure_file (armagetronad.protocol.in ${CMAKE_CURRENT_BINARY_DIR}/armagetronad.protocol)

install(FILES armagetronad.desktop
    DESTINATION "${DATADIR}/applications")
install(DIRECTORY icons
    DESTINATION "${AA_DATADIR}/desktop"
    FILES_MATCHING PATTERN "*.png"
    )


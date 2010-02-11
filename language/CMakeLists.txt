configure_file(languages.txt.in ${CMAKE_CURRENT_BINARY_DIR}/languages.txt)

install(FILES
    american.txt
    british.txt
    deutsch.txt
    english_base.txt
    english_base_notranslate.txt
    french.txt
    polish.txt
    polish_transliterated.txt
    spanish.txt

    new.txt
    ${CMAKE_CURRENT_BINARY_DIR}/languages.txt

    DESTINATION "${AA_DATADIR}/language"
    )


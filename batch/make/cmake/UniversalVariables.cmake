set (UNIVERSAL_VARS)

macro (UNIVERSAL_VAR VARNAME VAL)
    set ("${VARNAME}" "${VAL}" CACHE STRING "Universal variable ${VARNAME}")
    list(APPEND
        UNIVERSAL_VARS
        "${VARNAME}"
        )
endmacro(UNIVERSAL_VAR VARNAME VAL)

macro (WRITE_UNIVARS)
    file(REMOVE src/tUniversalVariables.h)
    foreach(UNIVAR ${UNIVERSAL_VARS})
        file(APPEND src/tUniversalVariables.h
            )
    endforeach(UNIVAR ${UNIVERSAL_VARS})
endmacro(WRITE_UNIVARS)

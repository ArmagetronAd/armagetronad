set (UNIVERSAL_VARS)

macro (UNIVERSAL_VAR VARNAME VAL)
    set (${VARNAME} ${VAL})
    set (${VARNAME} ${VAL} PARENT_SCOPE)
    list(APPEND
        UNIVERSAL_VARS
        ${VARNAME}
        )
endmacro(UNIVERSAL_VAR VARNAME VAL)

macro (UNIVERSAL_INSTALL_VAR VARNAME VAL)
    set (${VARNAME} ${VAL})
    set (${VARNAME} ${VAL} PARENT_SCOPE)    
    UNIVERSAL_VAR(${VARNAME} ${${VARNAME}})
endmacro(UNIVERSAL_INSTALL_VAR VARNAME VAL)

macro (WRITE_UNIVARS)
    file(REMOVE src/tUniversalVariables.h)
    foreach(UNIVAR ${UNIVERSAL_VARS})
        file(APPEND src/tUniversalVariables.h
"#ifndef ${UNIVAR}
    #define ${UNIVAR} \"${${UNIVAR}}\"
#endif
"
            )
    endforeach(UNIVAR ${UNIVERSAL_VARS})
endmacro(WRITE_UNIVARS)

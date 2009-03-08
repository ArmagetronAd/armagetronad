include_directories (
    ${ARMA_SOURCE_DIR}/src/thirdparty/particles/
    ${ARMA_INCLUDE_DIRS}
    )


add_library (particles
    pDomain.h
    pVec.h
    papi.h
	                            ParticleDLL/action_api.cpp
                                ParticleDLL/other_api.cpp
	ParticleDLL/actions.h       ParticleDLL/actions.cpp
    ParticleDLL/ParticleState.h ParticleDLL/ParticleState.cpp
	ParticleDLL/Particle.h
	ParticleDLL/ParticleGroup.h
    )

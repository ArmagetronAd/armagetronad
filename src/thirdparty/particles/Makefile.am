noinst_LIBRARIES = libparticles.a
libparticles_a_SOURCES = pDomain.h pVec.h papi.h\
			ParticleDLL/action_api.cpp  ParticleDLL/other_api.cpp\
			ParticleDLL/actions.cpp     ParticleDLL/ParticleState.cpp\
			ParticleDLL/actions.h        ParticleDLL/Particle.h\
			ParticleDLL/ParticleGroup.h  ParticleDLL/ParticleState.h
			
libparticles_a_CXXFLAGS = ${AM_CXXFLAGS} -I ${top_srcdir}/src -I ./ -I ParticleDLL


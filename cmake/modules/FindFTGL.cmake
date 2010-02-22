########################################################################
#
# Copyright (c) 2008, Lawrence Livermore National Security, LLC.  
# Produced at the Lawrence Livermore National Laboratory  
# Written by bremer5@llnl.gov,pascucci@sci.utah.edu.  
# LLNL-CODE-406031.  
# All rights reserved. 
#
# Adapted for ArmagetronAd by epsy <epsy46 at free dot fr>
#   
# This file is part of "Simple and Flexible Scene Graph Version 2.0."
# Please also read BSD_ADDITIONAL.txt.
#   
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#   
# @ Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the disclaimer below.
# @ Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the disclaimer (as noted below) in
#   the documentation and/or other materials provided with the
#   distribution.
# @ Neither the name of the LLNS/LLNL nor the names of its contributors
#   may be used to endorse or promote products derived from this software
#   without specific prior written permission.
#   
#  
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL LAWRENCE
# LIVERMORE NATIONAL SECURITY, LLC, THE U.S. DEPARTMENT OF ENERGY OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING
#
########################################################################


#
#
# Try to find the FTGL libraries
# Once done this will define
#
# FTGL_FOUND          - system has ftgl
# FTGL_INCLUDE_DIR    - path to FTGL/FTGL.h
# FTGL_LIBRARIES      - the library that must be included
#
#


FIND_PATH(FTGL_INCLUDE_DIR FTGL/ftgl.h
       ${ADDITIONAL_INCLUDE_PATH}
      /usr/include
      /usr/X11/include
      /usr/X11R6/include
      /usr/local/include
)

# IF (NOT WIN32)

FIND_FILE(FTGL_LIBRARIES libftgl.a  
       ${ADDITIONAL_LIBRARY_PATH}
      /usr/lib
      /usr/X11/lib
      /usr/X11R6/lib
      /sw/lib
      /usr/local/include
)

# # Not sure what's that for...
# ELSE (NOT WIN32)
# 
# FIND_LIBRARY(FTGL_LIBRARIES ftgl_static_MT
#        ${ADDITIONAL_LIBRARY_PATH}
#       /usr/lib
#       /usr/X11/lib
#       /usr/X11R6/lib
#       /sw/lib
#       /usr/local/lib
# )
# 
# ENDIF (NOT WIN32)


IF (FTGL_INCLUDE_DIR AND FTGL_LIBRARIES)

   SET(FTGL_FOUND "YES")
   IF (CMAKE_VERBOSE_MAKEFILE)
      MESSAGE("Using FTGL_INCLUDE_DIR = " ${FTGL_INCLUDE_DIR}) 
      MESSAGE("Using FTGL_LIBRARIES   = " ${FTGL_LIBRARIES}) 
   ENDIF (CMAKE_VERBOSE_MAKEFILE)

ELSE (FTGL_INCLUDE_DIR AND FTGL_LIBRARIES)
   
   IF (CMAKE_VERBOSE_MAKEFILE)
      MESSAGE("************************************")
      MESSAGE("  Necessary libftgl files not found")
      MESSAGE("  FTGL_INCLUDE_DIR = " ${FTGL_INCLUDE_DIR})
      MESSAGE("  FTGL_LIBRARIES   = " ${FTGL_LIBRARIES})
      MESSAGE("************************************")
   ENDIF (CMAKE_VERBOSE_MAKEFILE)

ENDIF (FTGL_INCLUDE_DIR AND FTGL_LIBRARIES)
                         

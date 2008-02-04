/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_tMemManager_H
#define ArmageTron_tMemManager_H

#include "aa_config.h"
#include <new>

#ifdef HAVE_STDLIB
#include <stdlib.h>
#endif

#ifdef HAVE_LIBEFENCE
#include <efence.h>
#ifdef DONTUSEMEMMANAGER
#include <efencepp.h>
#endif
#endif

#ifndef _MSC_VER
#ifndef _cdecl
#define _cdecl
#endif
#endif

#include <stdlib.h>

class tMemManBase{
public:
    static void  Check();
    static void  Profile();
};

#ifdef WIN32
#ifdef DEBUG
//#define DONTUSEMEMMANAGER
#endif
#endif

#ifdef _MSC_VER
//#define THROW_BADALLOC _THROW1(std::bad_alloc)
//#define THROW_NOTHING  _THROW0()
#define THROW_BADALLOC
#define THROW_NOTHING
#else
#define THROW_BADALLOC throw (std::bad_alloc)
#define THROW_NOTHING  throw ()
#endif


#ifndef DONTUSEMEMMANAGER

#ifndef NO_MALLOC_REPLACEMENT

// the following include file was found to disable the macros again
#include <ios>

// macros replacing C memory management
#define malloc(SIZE)                static_cast<void *>(tNEW(char)[SIZE])
#define calloc(ELEMCOUNT, ELEMSIZE) static_cast<void *>(tNEW(char)[(ELEMCOUNT)*(ELEMSIZE)])
#define free(BASEADR)               delete[] (reinterpret_cast< char* >(BASEADR))
#define realloc(BASEADR, NEWSIZE)   realloc not defined

// and other allocating functions
#define strdup(ADR)  tStrDup(ADR)

// implementation
char * tStrDup( char const * s );

// call the real malloc functions
void * real_calloc( size_t nmemb, size_t size ); // call calloc
void * real_malloc( size_t size );               // call malloc
void   real_free( void * ptr );                  // call free
void * real_realloc( void * ptr, size_t size );  // call realloc
char * real_strdup( char const * ptr );          // calls strdup
void * real_mmove( void * ptr, size_t size );    // take memory allocated by real_malloc or a C library function and move to managed memory
char * real_strmove( char * ptr );               // take C string allocated by real_malloc or a C library function and move to managed memory
#endif


void* _cdecl operator new	(size_t size) THROW_BADALLOC;
void  _cdecl operator delete   (void *ptr)  THROW_NOTHING;
void  operator delete   (void *ptr,bool keep) THROW_NOTHING;
void* operator new	(size_t size,const char *classn,const char *file,int line)  THROW_BADALLOC;
void  operator delete   (void *ptr,const char *classn, const char *file,int line)  THROW_NOTHING;
void* operator new[]	(size_t size)  THROW_BADALLOC;
void  operator delete[]   (void *ptr) THROW_NOTHING;
void* operator new[]	(size_t size,const char *classn,const char *file,int line)  THROW_BADALLOC;
void  operator delete[]   (void *ptr,const char *classname,const char *file,int line)  THROW_NOTHING;

void * operator new(
    size_t cb,
    int nBlockUse,
    const char * szFileName,
    int nLine
);

void operator delete(
    void * ptr,
    int nBlockUse,
    const char * szFileName,
    int nLine
);

#define tNEW(x) new(#x,__FILE__,__LINE__) x

#define tMEMMANAGER(classname)  public:void *operator new(size_t s){return tMemMan::Alloc(s); }  void operator delete(void *p){  if (p) tMemMan::Dispose(p); }
#else
#define tNEW(x) new x
#define tMEMMANAGER(classname)  

// just direct to the real malloc functions
#define real_malloc(SIZE)                malloc(SIZE)
#define real_calloc(ELEMCOUNT, ELEMSIZE) calloc(ELEMCOUNT, ELEMSIZE)
#define real_free(BASEADR)               free(BASEADR)
#define real_realloc(BASEADR, NEWSIZE)   realloc(BASEADR, NEWSIZE)
#define real_strdup( PTR )               strdup(PTR)
#define real_mmove( BASEADR, SIZE )      (BASEADR)
#define real_strmove( BASEADR )          (BASEADR)

#endif

#endif

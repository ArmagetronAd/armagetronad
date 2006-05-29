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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#ifndef _ERROR_H_
#define _ERROR_H_

#include "config.h"
#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ ""
#endif
#include <iomanip>
#include <sstream>
#include <iosfwd>
#include <string>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#define tVERIFY( x ) { if ( !( x ) ){ char* mess = "Assertion " #x " failed";  tERR_ERROR_INT( mess ); } }

#ifdef DEBUG

typedef enum {high=0,normal,low,very_low} tLevel;
typedef enum {flow=0,dump}                tChannel;

extern tLevel st_debugLevel[2];

int st_debugValid(tLevel l,tChannel c);


#define tERR_DUMP(level,stream,stuff) if(st_debugValid(level,stream))  std::cout <<  setw(28) << __FUNCTION__  << " : " << stuff << '\n'

#define tERR_FLOW() if(st_debugValid(low,flow)) std::cout  <<  std::setw(30) << __PRETTY_FUNCTION__  << '\n'

#define tERR_FLOW_HIGH() if(st_debugValid(normal,flow)) std::cout  <<  std::setw(30)  << __PRETTY_FUNCTION__  << '\n'

#define tERR_FLOW_LOW() if(st_debugValid(very_low,flow)) std::cout  <<  std::setw(30) << __PRETTY_FUNCTION__  << '\n'

#define tASSERT( x ) { if ( !( x ) ){ char* mess = "Assertion " #x " failed";  tERR_ERROR_INT( mess ); } }

#define tASSERT_EVAL( x ) { if ( !( x ) ){ char* mess = "Assertion " #x " failed";  tERR_ERROR_INT( mess ); } }

#else  /* DEBUG */
#define tERR_DUMP(level,stream,stuff) 
#define tERR_FLOW()
#define tERR_FLOW_HIGH()
#define tERR_FLOW_LOW() 
#define tASSERT( x )
#define tASSERT_EVAL( x ) x
#endif /* DEBUG */

//extern char ERROR_MESSAGE[1000];

void st_Breakpoint();

void st_PresentError( const char* caption, const char *message );
void st_PresentMessage( const char* caption, const char *message );

/*
#ifndef WIN32

#define tERR_ERROR_INT(mess) { std::cerr << "Internal error in " << __PRETTY_FUNCTION__ << " in " << __FILE__<< ':' << __LINE__ << " : \n \t" << mess << '\n'; st_Breakpoint(); ::exit(-1);}
   
#define tERR_ERROR(mess) { std::cerr << "Error in " << __PRETTY_FUNCTION__ << " in " << __FILE__<< ':' << __LINE__ << " : \n \t" << mess << '\n'; st_Breakpoint(); ::exit(-1);}

#else

#include <windows.h>

#define tERR_ERROR_INT(mess) { st_Breakpoint(); std::stringstream s(ERROR_MESSAGE,999,ios::out);  s << "Internal error in " << __PRETTY_FUNCTION__ << " in " << __FILE__<< ':' << __LINE__ << " : \n \t" << mess << "\nPlease send a Bug report!" << '\0'; MessageBox (NULL, ERROR_MESSAGE , "Internal Error", MB_OK); ::exit(-1);}
   
#define tERR_ERROR(mess) {   st_Breakpoint(); std::stringstream s(ERROR_MESSAGE,999,ios::out); s << "Error in " << __PRETTY_FUNCTION__ << " in " << __FILE__<< ':' << __LINE__ << " : \n \t" << mess << '\n'; MessageBox (NULL, ERROR_MESSAGE , "Error", MB_OK); ::exit(-1);}

#endif // WINDOWS
*/

#define tERR_ERROR_INT(mess) { std::ostringstream s;  s << "Internal error in " << __PRETTY_FUNCTION__ << " in " << __FILE__<< ':' << __LINE__ << " : \n \t" << mess << "\nPlease send a Bug report!" << '\0'; st_PresentError( "Internal Error", s.str().c_str() );}

#define tERR_ERROR(mess) {   std::ostringstream s; s << "Error in " << __PRETTY_FUNCTION__ << " in " << __FILE__<< ':' << __LINE__ << " : \n \t" << mess << '\n'; st_PresentError( "Error", s.str().c_str() ); }

#define tERR_MESSAGE(mess) {   std::ostringstream s; s << "Message from " << __PRETTY_FUNCTION__ << " in " << __FILE__<< ':' << __LINE__ << " : \n \t" << mess << '\n'; st_PresentMessage( "Message", s.str().c_str() ); }

#define tERR_WARN(mess) {con << "Warning in " << __PRETTY_FUNCTION__ << " in " << __FILE__<< ':' << __LINE__ << " : \n \t" << mess << '\n';}


/*
#define tERR_ERROR_INT(mess) { st_Breakpoint(); exit(-1); }

#define tERR_ERROR(mess) { st_Breakpoint(); exit(-1); }

#define tERR_WARN(mess) { st_Breakpoint(); }
*/
#endif // _ERROR_H_


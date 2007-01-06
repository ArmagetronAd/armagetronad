%{
#include "tError.h"
%}

%rename(present_error) st_PresentError;
void st_PresentError( const char* caption, const char *message );

%rename(present_message) st_PresentMessage;
void st_PresentMessage( const char* caption, const char *message );

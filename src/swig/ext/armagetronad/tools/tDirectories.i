%{
#include "tDirectories.h"
%}

%feature("director") tPath;
%rename(Path) tPath;
%rename(PathResource) tPathResource;
%rename(PathWebroot) tPathWebroot;
%rename(Directories) tDirectories;

%include "tDirectories.h"

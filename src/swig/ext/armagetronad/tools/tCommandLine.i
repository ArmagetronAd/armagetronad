%{
#include "tCommandLine.h"
%}

// class tCommandLineAnalyzer;
// %import "tLinkedList.i"
// %template(ListItemCommandLineAnalyzer) tListItem<tCommandLineAnalyzer>;

%rename(CommandLineData) tCommandLineData;
%rename(CommandLineParser) tCommandLineParser;
%rename(CommandLineAnalyzer) tCommandLineAnalyzer;
%rename(program_version) programVersion_;
%rename(name) name_;


%include "tCommandLine.h"

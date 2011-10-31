#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
#include "nServerInfo.h"
#include "tCommandLine.h"
#include "tDirectories.h"
#include "tLocale.h"
#include "tVersion.h"
// #include "nSocket.h"
// #include "tConsole.h"
// #include "nNetwork.h"

namespace sq
{
    void ParseConnectionString(const tString & s, tString & connectionName, unsigned & port, unsigned defaultPort)
    {
        int sepIndex = s.find(':');
        if (sepIndex != std::string::npos)
        {
            connectionName = s.SubStr(0, sepIndex);
            port = s.SubStr(sepIndex + 1).ToInt();
        }
        else
        {
            connectionName = s;
            port = defaultPort;
        }
    }
    
    class ServerQueryCommandLineAnalyzer : public tCommandLineAnalyzer
    {
    public:
        ServerQueryCommandLineAnalyzer(tCommandLineAnalyzer *& anchor)
            :tCommandLineAnalyzer(anchor), listOption_(false), masterServer_(""), servers_()
        {
        }
        
        void ReadServersFromStdin()
        {
            int descriptor = fileno(stdin);
            int flags = fcntl(descriptor, F_GETFL);
            fcntl(descriptor, F_SETFL, flags | O_NONBLOCK);
            tString server;
            while (std::getline(std::cin, server))
            {
                servers_.push_back(server);
            }
        }
    private:
        virtual bool DoAnalyze(tCommandLineParser & parser)
        {
            if (parser.GetSwitch("--list", "-l"))
            {
                listOption_ = true;
                return true;
            }
            else if (parser.GetOption(masterServer_, "--master", "-m"))
            {
                return true;
            }
            else if (parser.Current()[0] != '-')
            {
                servers_.push_back(tString(parser.Current()));
                parser.Advance();
                return true;
            }
            return false;
        }

        virtual void DoHelp(std::ostream & s)
        {
            s << "-l, --list                   : Fetches only the server list from the master server, and does not query the individual servers for advanced information.\n";
            s << "-m, --master                 : Use the specified master server. By default, the program will use one of the game-provided default master servers.\n";
        }
        
        virtual bool DoExecute()
        {
            if (!servers_.empty())
            {
                
            }
            else
            {
                LoadMasters();
                // tSuppressConsole console;
                nServerInfo::GetFromMaster();
            }
            
            return true;
        }
        
        void LoadMasters()
        {
            if (masterServer_.size() > 0)
            {
                nMasterLoader masterLoader;
                tString connectionName;
                unsigned port;
                ParseConnectionString(masterServer_, connectionName, port, 4533);
                masterLoader.AddMaster(connectionName, port);
            }
            else
            {
                nServerInfo::GetMasters();
            }
        }

        bool listOption_;
        tString masterServer_;
        std::vector< tString > servers_;
    };
}

int main(int argc, char **argv)
{
    try
    {
        tLocale::Load("languages.txt");
        
        tCommandLineAnalyzer *commandLineAnchor;
        tDirectoriesCommandLineAnalyzer directoryOptions(commandLineAnchor);
        sq::ServerQueryCommandLineAnalyzer options(commandLineAnchor);
        tCommandLineData commandLine(st_programVersion, commandLineAnchor);

        if (!commandLine.Analyse(argc, argv))
            return 0;
            
        options.ReadServersFromStdin();
        commandLine.Execute();

        return 0;
    }
    catch (...)
    {
        return 1;
    }
}

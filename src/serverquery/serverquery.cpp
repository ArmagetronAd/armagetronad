#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
#include "json/json.h"
#include "nNetwork.h"
#include "nServerInfo.h"
#include "nSocket.h"
#include "tCommandLine.h"
#include "tConsole.h"
#include "tDirectories.h"
#include "tLocale.h"
#include "tSystime.h"
#include "tVersion.h"

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
    
    class ServerInfo : public nServerInfo
    {
    public:
        static ServerInfo *GetFirstServer()
        {
            return dynamic_cast< ServerInfo * >( nServerInfo::GetFirstServer() );
        }

        ServerInfo *Next()
        {
            return dynamic_cast< ServerInfo * >( nServerInfo::Next() );
        }
        
        bool IsAdvancedInfoSet() const
        {
            return advancedInfoSet;
        }
        
        void ToJson(Json::Value & object) const
        {
            object["name"] = GetName();
            object["release"] = Release();
            object["users"] = Users();
            object["max_users"] = MaxUsers();
            object["ping"] = Ping();
            object["user_names"] = UserNames();
            object["user_names_one_line"] = UserNamesOneLine();
            object["url"] = Url();
            object["options"] = Options();
        }
    };
    
    nServerInfo *CreateServer()
    {
        nServerInfo *ret = tNEW(ServerInfo);
        return ret;
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
            Json::Value root(Json::arrayValue);
            Json::FastWriter writer;
            
            if (!servers_.empty())
            {
                // Create nServerInfo instances for servers
            }
            else
            {
                LoadMasters();
                nServerInfo::GetFromMaster();
            }
            
            nServerInfo::StartQueryAll();
            while (nServerInfo::DoQueryAll(10))
            {
                CheckUpdates(root, writer);
                tAdvanceFrame(1000);
            }
            CheckUpdates(root, writer);
            // std::cout << writer.write(root);
            return true;
        }
        
        void LoadMasters() const
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
        
        void CheckUpdates(Json::Value & root, Json::FastWriter & writer) const
        {
            ServerInfo *run = ServerInfo::GetFirstServer();
            while (run)
            {
                if (run->IsAdvancedInfoSet())
                {
                    Json::Value serverToJson(Json::objectValue);
                    run->ToJson(serverToJson);
                    std::cout << writer.write(serverToJson);
                    root.append(serverToJson);
                    run->Remove();
                }
                run = run->Next();
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
        tCommandLineAnalyzer *commandLineAnchor;
        tDirectoriesCommandLineAnalyzer directoryOptions(commandLineAnchor);
        sq::ServerQueryCommandLineAnalyzer options(commandLineAnchor);
        tCommandLineData commandLine(st_programVersion, commandLineAnchor);

        if (!commandLine.Analyse(argc, argv))
            return 0;
            
        tLocale::Load("languages.txt");
        nServerInfo::SetCreator(&sq::CreateServer);
        
        options.ReadServersFromStdin();
        tSuppressConsole console;
        commandLine.Execute();
        sn_BasicNetworkSystem.Shutdown();

        return 0;
    }
    catch (...)
    {
        return 1;
    }
}

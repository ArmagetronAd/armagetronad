#include <fcntl.h>
#include <iostream>
#include <sstream>
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
    typedef std::vector< tString > StringVector;
    
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
    
    void SplitString(const tString & s, char delimeter, StringVector & xs)
    {
        std::stringstream ss(s);
        tString item;
        while(std::getline(ss, item, delimeter))
            xs.push_back(item);
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
            object["url"] = Url();
            object["options"] = Options();            
            ToJsonSplit(object, "user_names", UserNames());
            ToJsonSplit(object, "user_global_ids", UserGlobalIDs());
        }
        
        void ToJsonSplit(Json::Value & object, const char *key, const tString & splitString) const
        {
            Json::Value jsonArray(Json::arrayValue);
            StringVector xs;
            SplitString(splitString, '\n', xs);
            for (StringVector::const_iterator it = xs.begin(); it != xs.end(); ++it)
            {
                jsonArray.append(*it);
            }
            object[key] = jsonArray;
        }
    };
    
    nServerInfo *CreateServer()
    {
        return tNEW(ServerInfo);
    }
    
    class ServerQueryCommandLineAnalyzer : public tCommandLineAnalyzer
    {
    public:
        ServerQueryCommandLineAnalyzer(tCommandLineAnalyzer *& anchor)
            :tCommandLineAnalyzer(anchor), listOption_(false), masterServer_(""), aggregateOption_(false), prettyPrintOption_(false), servers_()
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
            else if (parser.GetSwitch("--aggregate", "-a"))
            {
                aggregateOption_ = true;
                return true;
            }
            else if (parser.GetSwitch("--pretty-print", "-p"))
            {
                prettyPrintOption_ = true;
                aggregateOption_ = true;
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
            s << "\n";
            s << "-l, --list                   : Fetches only the server list from the master server, and does not query the individual servers for advanced information.\n";
            s << "-m, --master                 : Use the specified master server. By default, the program will use one of the game-provided default master servers.\n";
            s << "-a, --aggregate              : (TODO document this option)\n";
            s << "-p, --pretty-print           : Create JSON output that is more human-readable. Using this option turns on --aggregate.\n";
        }
        
        virtual bool DoExecute()
        {
            Json::Value root(Json::arrayValue);
            Json::StyledWriter styledWriter;
            Json::FastWriter fastWriter;
            Json::Writer & writer = prettyPrintOption_ ? (Json::Writer &)styledWriter : (Json::Writer &)fastWriter;
            
            if (!servers_.empty())
            {
                for (StringVector::const_iterator it = servers_.begin(); it != servers_.end(); ++it)
                {
                    tString connectionName;
                    unsigned port;
                    ParseConnectionString(*it, connectionName, port, 4534);
                    nServerInfo *server = CreateServer();
                    server->SetConnectionName(connectionName);
                    server->SetPort(port);
                }
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
            
            if (aggregateOption_)
                std::cout << writer.write(root);
            
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
        
        void CheckUpdates(Json::Value & root, Json::Writer & writer) const
        {
            ServerInfo *run = ServerInfo::GetFirstServer();
            while (run)
            {
                if (run->IsAdvancedInfoSet())
                {
                    Json::Value serverToJson(Json::objectValue);
                    run->ToJson(serverToJson);
                    if (aggregateOption_)
                    {
                        root.append(serverToJson);
                    }
                    else
                    {
                        std::cout << writer.write(serverToJson);
                    }
                    run->Remove();
                }
                run = run->Next();
            }
        }

        bool listOption_;
        tString masterServer_;
        bool aggregateOption_;
        bool prettyPrintOption_;
        StringVector servers_;
    };
}

int main(int argc, char **argv)
{
    try
    {
        tCommandLineAnalyzer *commandLineAnchor;
        tDirectoriesCommandLineAnalyzer directoryOptions(commandLineAnchor, false);
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

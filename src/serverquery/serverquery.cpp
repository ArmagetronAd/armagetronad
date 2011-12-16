/*

*************************************************************************

Armagetron Advanced -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2011  by Daniel Lee Harple
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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
#include "tSysTime.h"
#include "tToDo.h"
#include "tVersion.h"

namespace sq
{
    typedef std::vector< tString > StringVector;
    
    void ParseConnectionString(const tString & s, tString & connectionName, unsigned & port, unsigned defaultPort)
    {
        size_t sepIndex = s.find(':');
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
        ServerInfo()
            :nServerInfo(), wasDisplayed_(false)
        {
        }
        
        static ServerInfo *GetFirstServer()
        {
            return dynamic_cast< ServerInfo * >( nServerInfo::GetFirstServer() );
        }

        ServerInfo *Next()
        {
            return dynamic_cast< ServerInfo * >( nServerInfo::Next() );
        }
        
        bool WasDisplayed() const
        {
            return wasDisplayed_;
        }
        
        void SetDisplayed()
        {
            wasDisplayed_ = true;
        }
        
        bool IsAdvancedInfoSet() const
        {
            return advancedInfoSet;
        }
        
        void ToJson(Json::Value & object, bool encodeAllInfo) const
        {
            object["connection_name"] = GetConnectionName();
            object["port"] = GetPort();
            
            if (encodeAllInfo)
            {
                object["name"] = GetName();
                object["release"] = Release();
                object["users"] = Users();
                object["max_users"] = MaxUsers();
                object["ping"] = Ping();
                object["url"] = Url();
                object["options"] = Options();
                object["version_min"] = Version().Min();
                object["version_max"] = Version().Max();
                
                ToJsonSplit(object, "user_names", UserNames());
                ToJsonSplit(object, "user_global_ids", UserGlobalIDs());
                
                const nServerInfo::SettingsDigest & settings = GetSettings();
                if (settings.GetFlag(SettingsDigest::Flags_SettingsDigestSent))
                {
                    Json::Value settingsObject(Json::objectValue);
                    settingsObject["authentication_required"] = settings.GetFlag(SettingsDigest::Flags_AuthenticationRequired);
                    settingsObject["nondefault_map"] = settings.GetFlag(SettingsDigest::Flags_NondefaultMap);
                    settingsObject["team_play"] = settings.GetFlag(SettingsDigest::Flags_TeamPlay);
                    settingsObject["min_play_time_total"] = settings.minPlayTimeTotal_;
                    settingsObject["min_play_time_online"] = settings.minPlayTimeOnline_;
                    settingsObject["min_play_time_team"] = settings.minPlayTimeTeam_;
                    settingsObject["cycle_delay"] = settings.cycleDelay_;
                    settingsObject["acceleration"] = settings.acceleration_;
                    settingsObject["rubber_wall_hump"] = settings.rubberWallHump_;
                    settingsObject["rubber_hit_wall_ratio"] = settings.rubberHitWallRatio_;
                    settingsObject["walls_length"] = settings.wallsLength_;
                    object["settings"] = settingsObject;
                }
            }
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
    private:
        bool wasDisplayed_;
    };
    
    nServerInfo *CreateServer()
    {
        return tNEW(ServerInfo);
    }
    
    class ServerQueryCommandLineAnalyzer : public tCommandLineAnalyzer
    {
    public:
        ServerQueryCommandLineAnalyzer(tCommandLineAnalyzer *& anchor)
            :tCommandLineAnalyzer(anchor), listOption_(false), asynchronousOption_(false), freshOption_(false), masterServer_(""), servers_(), exitStatus_(0)
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
        
        int ExitStatus() const
        {
            return exitStatus_;
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
            else if (parser.GetSwitch("--asynchronous", "-a"))
            {
                asynchronousOption_ = true;
                return true;
            }
            else if (parser.GetSwitch("--fresh", "-f"))
            {
                freshOption_ = true;
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
            s << "\nQuery options\n"
              << "=============\n\n"
              << "If a list of servers is provided on the command-line, then only those servers\n"
              << "will be queried for information. This list can also be read from stdin, as a\n"
              << "newline separated list of values.\n\n"
              << "The program's exit status will be 1 if there is an error fetching the server\n"
              << "list from the master.\n\n"
              << "-l, --list                   : Fetches only the server list from the master,\n"
              << "                               and does not query the individual game servers\n"
              << "                               for advanced information.\n\n"
              << "-m, --master                 : Use the specified master server. By default, the\n"
              << "                               program will select a random master server from\n"
              << "                               the game-provided master.srv configuration file.\n\n"
              << "-a, --asynchronous           : Output the collected server information as it\n"
              << "                               arrives. By default, the program will wait for\n"
              << "                               all queries to finish and output the information\n"
              << "                               as objects in a JSON array. Using this option\n"
              << "                               will write onto one line the JSON object for\n"
              << "                               each server, when its data is received.\n\n"
              << "-f, --fresh                  : Do not preload the server list from the cache on\n"
              << "                               disk.\n\n"
              << "Other options\n"
              << "=============\n";
        }
        
        virtual bool DoExecute()
        {
            Json::Value root(Json::arrayValue);
            Json::StyledWriter styledWriter;
            Json::FastWriter fastWriter;
            Json::Writer & writer = asynchronousOption_ ? (Json::Writer &)fastWriter : (Json::Writer &)styledWriter;
            
            if (servers_.empty())
            {
                tString fileSuffix = LoadMasters();
                std::cerr << "--> Fetching master server list\n";
                nServerInfoBase *master = nServerInfo::GetFromMaster( NULL, fileSuffix.c_str(), !freshOption_);
                if (master)
                {
                    std::cerr << "--> Received " << nServerInfo::ServerCount() << " servers from " << master->GetConnectionName() << ":" << master->GetPort() << "\n";
                }
                else
                {
                    exitStatus_ = 1;
                    std::cerr << "--> Received 0 servers, because none of the master servers could be successfully contacted\n";
                }
            }
            else
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
            
            if (!listOption_)
            {
                std::cerr << "--> Querying servers for advanced information\n";
                nServerInfo::StartQueryAll();
                while (nServerInfo::DoQueryAll(10))
                {
                    if (asynchronousOption_)
                        CheckUpdates(root, writer);
                    // Advance time, and run postponed events (DNS queries, etc..)
                    tAdvanceFrame(1000);
                    st_DoToDo();
                }
            }
            
            CheckUpdates(root, writer);
            
            if (!asynchronousOption_)
                std::cout << writer.write(root);
            
            return true;
        }
        
        tString LoadMasters() const
        {
            std::stringstream fileSuffix;
            fileSuffix << "_serverquery";
            
            if (masterServer_.size() > 0)
            {
                nMasterLoader masterLoader;
                tString connectionName;
                unsigned port;
                ParseConnectionString(masterServer_, connectionName, port, 4533);
                masterLoader.AddMaster(connectionName, port);
                
                fileSuffix << "_" << connectionName << "_" << port;
            }
            else
            {
                nServerInfo::GetMasters();
            }
            
            return fileSuffix.str();
        }
        
        void CheckUpdates(Json::Value & root, Json::Writer & writer) const
        {
            ServerInfo *run = ServerInfo::GetFirstServer();
            while (run)
            {
                if (!run->WasDisplayed() && (run->IsAdvancedInfoSet() || listOption_))
                {
                    run->SetDisplayed();
                    Json::Value serverToJson(Json::objectValue);
                    run->ToJson(serverToJson, !listOption_);
                    if (asynchronousOption_)
                    {
                        std::cout << writer.write(serverToJson);
                    }
                    else
                    {
                        root.append(serverToJson);
                    }                    
                }
                run = run->Next();
            }
        }

        bool listOption_;
        bool asynchronousOption_;
        bool freshOption_;
        tString masterServer_;
        StringVector servers_;
        int exitStatus_;
    };
}

int main(int argc, char **argv)
{
    try
    {
        tCommandLineAnalyzer *commandLineAnchor = NULL;
        tDirectoriesCommandLineAnalyzer directoryOptions(commandLineAnchor, false);
        sq::ServerQueryCommandLineAnalyzer options(commandLineAnchor);
        tCommandLineData commandLine(st_programVersion, commandLineAnchor);
        commandLine.SetExtraProgramUsage(" [server[:port] ...]");

        if (!commandLine.Analyse(argc, argv))
            return 0;
            
        tLocale::Load("languages.txt");
        nServerInfo::SetCreator(&sq::CreateServer);
        srand((unsigned)time(NULL));
        
        options.ReadServersFromStdin();
        tSuppressConsole console;
        commandLine.Execute();
        sn_BasicNetworkSystem.Shutdown();

        return options.ExitStatus();
    }
    catch (...)
    {
        return 1;
    }
}

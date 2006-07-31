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
file by guru3/tank program

*/

#include "gStatistics.h"
#include "tConfiguration.h"

//0 is flat file
//1 will be database
static int statOutputType = 0;
static tSettingItem<int> sot_ci("STAT_OUTPUT", statOutputType);

gStatistics* gStats;

gStatistics::gStatistics()
{
    highscores = new gStatList("highscores", statOutputType);
    won_rounds = new gStatList("won_rounds", statOutputType);
    won_matches = new gStatList("won_matches", statOutputType);
    //	ladder = new gStatList("ladder", statOutputType);
    kills = new gStatList("kills", statOutputType);
    deaths = new gStatList("deaths", statOutputType);
}

gStatistics::~gStatistics()
{
    delete highscores;
    delete won_rounds;
    delete won_matches;
    //	delete ladder;
    delete kills;
    delete deaths;
}

/* original code stripped out to be replaced with the new stuff....

class gHighscoresBase{
    int id;
    static tList<gHighscoresBase> highscoreList;
protected:
    tArray<tString> highName;

    char*   highscore_file;
    tOutput desc;
    int     maxSize;

    // find the player at position i
    ePlayerNetID *online(int p){
        for (int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
            if (se_PlayerNetIDs(i)->IsHuman() && !strcmp(se_PlayerNetIDs(i)->GetUserName(),highName[p]))
                return se_PlayerNetIDs(i);

        return NULL;
    }

    // i is the active player, j the passive one
    virtual void swap_entries(int i,int j){
        Swap(highName[i],highName[j]);

        // swap: i is now the passive player

        // send the poor guy who just dropped a message
        ePlayerNetID *p=online(i);
        if (p){
            tColoredString name;
            name << *p << tColoredString::ColorString(1,.5,.5);

            tOutput message;
            message.SetTemplateParameter(1, static_cast<const char *>(name));
            message.SetTemplateParameter(2, static_cast<const char *>(highName[j]));
            message.SetTemplateParameter(3, i+1);
            message.SetTemplateParameter(4, desc);

            if (i<j)
                message <<  "$league_message_rose" ;
            else
                message <<  "$league_message_dropped" ;

            message << "\n";

            tString s;
            s << message;
            sn_ConsoleOut(s,p->Owner());
            // con << message;
        }

    }

    void load_Name(std::istream &s,int i){
        char c=' ';
        while (s.good() && !s.eof() && isspace(c))
            s.get(c);
        s.putback(c);

        // read and filter name
        tString name;
        name.ReadLine( s );
        ePlayerNetID::FilterName( name, highName[i] );
    }

    void save_Name(std::ostream &s,int i){
        s << highName[i];
    }


public:

    virtual void Save()=0;
    virtual void Load()=0;

    // returns if i should stay above j
    virtual bool inorder(int i,int j)=0;

    void sort(){
        // since single score items travel up the
        // score ladder, this is the most efficient sort algorithm:

        for(int i=1;i<highName.Len();i++)
            for(int j=i;j>=1 && !inorder(j-1,j);j--)
                swap_entries(j,j-1);
    }


    int checkPos(int found){
        // move him up
        int newpos=found;
        while(newpos>0 && !inorder(newpos-1,newpos)){
            swap_entries(newpos,newpos-1);
            newpos--;
        }

        // move him down
        while(newpos<highName.Len()-1 && !inorder(newpos,newpos+1)){
            swap_entries(newpos,newpos+1);
            newpos++;
        }

        //Save();

        return newpos;
    }

    int Find(const char *name,bool force=false){
        int found=highName.Len();
        for(int i=found-1;i>=0 ;i--)
            if(highName[i].Len()<=1 || !strcmp(highName[i],name)){
                found=i;
            }

        if (force && highName[found].Len()<=1)
            highName[found]=name;

        return found;
    }

    gHighscoresBase(char *name,char *sd,int max=0)
            :id(-1),highscore_file(name),desc(sd),maxSize(max){
        highscoreList.Add(this,id);
    }

    virtual ~gHighscoresBase(){
        highscoreList.Remove(this,id);
    }

    virtual void greet_this(ePlayerNetID *p,tOutput &o){
        //    tOutput o;

        int f=Find(p->GetUserName())+1;
        int l=highName.Len();

        o.SetTemplateParameter(1, f);
        o.SetTemplateParameter(2, l);
        o.SetTemplateParameter(3, desc);

        if (l>=f)
            o << "$league_message_greet";
        else
            o << "$league_message_greet_new";

        //    s << o;
    }

    static void Greet(ePlayerNetID *p,tOutput &o){
        o << "$league_message_greet_intro";
        for(int i=highscoreList.Len()-1;i>=0;i--){
            highscoreList(i)->greet_this(p,o);
            if (i>1)
                o << "$league_message_greet_sep" << " ";
            if (i==1)
                o << " " << "$league_message_greet_lastsep" << " ";
        }
        o << ".\n";
    }

    static void SaveAll(){
        if ( !tRecorder::IsRunning() )
            for(int i=highscoreList.Len()-1;i>=0;i--)
                highscoreList(i)->Save();
    }

    static void LoadAll(){
        if ( !tRecorder::IsRunning() )
            for(int i=highscoreList.Len()-1;i>=0;i--)
                highscoreList(i)->Load();
    }
};


tString GreetHighscores()
{
    tOutput o;
    gHighscoresBase::Greet(eCallbackGreeting::Greeted(),o);
    tString s;
    s << o;
    return s;
}

static eCallbackGreeting g(GreetHighscores);

tList<gHighscoresBase> gHighscoresBase::highscoreList;

template<class T>class highscores: public gHighscoresBase{
    protected:
    tArray<T>    high_score;

    virtual void swap_entries(int i,int j){
        Swap(high_score[i],high_score[j]);
        gHighscoresBase::swap_entries(i,j);
    }

    public:
    virtual void Load(){
        std::ifstream s;

        if ( tDirectories::Var().Open ( s, highscore_file ) )
        {
            int i=0;
            while (s.good() && !s.eof())
            {
                s >> high_score[i];
                load_Name(s,i);
                // con << highName[i] << " " << high_score[i] << '\n';
                i++;
            }
        }
    }

    // returns if i should stay above j
    virtual bool inorder(int i,int j)
    {
        return (highName[j].Len()<=1 || high_score[i]>=high_score[j]);
    }


    virtual void Save(){
        std::ofstream s;

        sort();

        if ( tDirectories::Var().Open ( s, highscore_file ) )
        {
            int i=0;
            int max=high_score.Len();
            if (maxSize && max>maxSize)
                max=maxSize;
            while (highName[i].Len()>1 && i<max){
                tString mess;
                mess << high_score[i];
                mess.SetPos(10, false );
                s << mess;
                save_Name(s,i);
                s << '\n';
                i++;
                //std::cout << mess;
                //save_Name(std::cout,i);
                //std::cout << '\n';
            }
        }
    }

    void checkPos(int found,const tString &name,T score){
        tOutput message;
        // find the name in the list
        bool isnew=false;

        message.SetTemplateParameter(1, name);
        message.SetTemplateParameter(2, desc);
        message.SetTemplateParameter(3, score);

        if (highName[found].Len()<=1){
            highName[found]=name;
            message << "$highscore_message_entered";
            high_score[found]=score;
            isnew=true;
        }
        else if (score>high_score[found]){
            message << "$highscore_message_improved";
            high_score[found]=score;
        }
        else
            return;

        // move him up
        int newpos=gHighscoresBase::checkPos(found);

        message.SetTemplateParameter(4, newpos + 1);

        if (newpos!=found || isnew)
            if (newpos==0)
                message << "$highscore_message_move_top";
            else
                message << "$highscore_message_move_pos";
        else
            if (newpos==0)
                message << "$highscore_message_stay_top";
            else
                message << "$highscore_message_stay_pos";

        message << "\n";

        ePlayerNetID *p=online(newpos);
        //con << message;
        if (p)
            sn_ConsoleOut(tString(message),p->Owner());
        //Save();
    }

    void Add( ePlayerNetID* player,T AddScore)
    {
        tASSERT( player );
        tString const & name = player->GetUserName();
        int f=Find(name,true);
        checkPos(f,name,AddScore+high_score[f]);
    }

    void Add( eTeam* team,T AddScore)
    {
        if ( team->NumHumanPlayers() > 0 )
        {
            for ( int i = team->NumPlayers() - 1 ; i>=0; --i )
            {
                ePlayerNetID* player = team->Player( i );
                if ( player->IsHuman() )
                {
                    this->Add( player, AddScore );
                }
            }
        }
    }

    void Check(const ePlayerNetID* player,T score){
        tASSERT( player );
        tString name = player->GetUserName();
        int len = high_score.Len();
        if (len<=0 || score>high_score[len-1]){
            // find the name in the list
            int found=Find(name,true);
            checkPos(found,name,score);
        }
    }

    highscores(char *name,char *sd,int max=0)
            :gHighscoresBase(name,sd,max){
        //		Load();
    }

    virtual ~highscores(){
        //		Save();
    }
};



static REAL ladder_min_bet=1;
static tSettingItem<REAL> ldd_mb("LADDER_MIN_BET",
                                 ladder_min_bet);

static REAL ladder_perc_bet=10;
static tSettingItem<REAL> ldd_pb("LADDER_PERCENT_BET",
                                 ladder_perc_bet);

static REAL ladder_tax=1;
static tSettingItem<REAL> ldd_tex("LADDER_TAX",
                                  ladder_tax);

static REAL ladder_lose_perc_on_load=.2;
static tSettingItem<REAL> ldd_lpl("LADDER_LOSE_PERCENT_ON_LOAD",
                                  ladder_lose_perc_on_load);

static REAL ladder_lose_min_on_load=.2;
static tSettingItem<REAL> ldd_lml("LADDER_LOSE_MIN_ON_LOAD",
                                  ladder_lose_min_on_load);

static REAL ladder_gain_extra=1;
static tSettingItem<REAL> ldd_ga("LADDER_GAIN_EXTRA",
                                 ladder_gain_extra);


class ladder: public highscores<REAL>{
public:
    virtual void Load(){
        highscores<REAL>::Load();

        sort();

        int end=highName.Len();

        for(int i=highName.Len()-1;i>=0;i--){

            // make them lose some points

            REAL loss=ladder_lose_perc_on_load*high_score[i]*.01;
            if (loss<ladder_lose_min_on_load)
                loss=ladder_lose_min_on_load;
            high_score[i]-=loss;

            if (high_score[i]<0)
                end=i;
        }

        // remove the bugggers with less than 0 points
        highName.SetLen(end);
        high_score.SetLen(end);
    }

    void checkPos(int found,const tString &name,REAL score){
        tOutput message;

        message.SetTemplateParameter(1, name);
        message.SetTemplateParameter(2, desc);
        message.SetTemplateParameter(3, score);

        // find the name in the list
        bool isnew=false;
        if (highName[found].Len()<=1){
            highName[found]=name;
            message << "$ladder_message_entered";
            high_score[found]=score;
            isnew=true;
        }
        else{
            REAL diff=score-high_score[found];
            message.SetTemplateParameter(5, static_cast<float>(fabs(diff)));

            if (diff>0)
                message << "$ladder_message_gained";
            else
                message << "$ladder_message_lost";

            high_score[found]=score;
        }

        // move him up
        int newpos=gHighscoresBase::checkPos(found);

        message.SetTemplateParameter(4, newpos + 1);

        if (newpos!=found || isnew)
            if (newpos==0)
                message << "$ladder_message_move_top";
            else
                message << "$ladder_message_move_pos";
        else
            if (newpos==0)
                message << "$ladder_message_stay_top";
            else
                message << "$ladder_message_stay_pos";

        message << "\n";

        ePlayerNetID *p=online(newpos);
        // con << message;
        if (p){
            sn_ConsoleOut(tString(message),p->Owner());
        }

        // Save();
    }

    void Add(const tString &name,REAL AddScore){
        int found=Find(name,true);
        checkPos(found,name,AddScore+high_score[found]);
    }

    // ladder mechanics: what happens if someone wins?
    void winner( eTeam *winningTeam ){
        tASSERT( winningTeam );

        // AI won? bail out.
        if ( winningTeam->NumHumanPlayers() <= 0 )
        {
            return;
        }

        // only do something in multiplayer mode
        int i;
        int count=0;

        tArray<ePlayerNetID*> active;

        for(i=se_PlayerNetIDs.Len()-1;i>=0;i--)
        {
            ePlayerNetID* p=se_PlayerNetIDs(i);

            // only take enemies of the current winners (and the winners themselves) into the list
            if (p->IsHuman() && ( p->CurrentTeam() == winningTeam || eTeam::Enemies( winningTeam, p ) ) )
            {
                count++;
                active[active.Len()] = p;
            }
        }

        // only one active player? quit.
        if ( active.Len() <= 1 )
        {
            return;
        }

        // collect the bets
        tArray<int> nums;
        tArray<REAL> bet;

        REAL pot=0;

        for(i=active.Len()-1;i>=0;i--){

            nums[i]=Find(active(i)->GetUserName(),true);

            if (high_score[nums[i]]<0)
                high_score[nums[i]]=0;

            bet[i]=high_score[nums[i]]*ladder_perc_bet*.01;
            if (bet[i]<ladder_min_bet)
                bet[i]=ladder_min_bet;
            pot+=bet[i];
        }

        pot-=pot*ladder_tax*.01; // you have to pay to the bank :-)

        // now bet[i] tells us how much player nums[i] has betted
        // and pot is the overall bet. Add something to it, prop to
        // the winners ping+ping charity:

        // take the bet from the losers and give it to the winner
        for(i=active.Len()-1; i>=0; i--)
        {
            ePlayerNetID* player = active(i);
            if(player->CurrentTeam() == winningTeam )
            {
                REAL pc=player->ping + player->pingCharity*.001;
                if (pc<0)
                    pc=0;
                if (pc>1) // add sensible bounds
                    pc=1;

                REAL potExtra = pc*ladder_gain_extra;

                if ( player->Object() && player->Object()->Alive() )
                {
                    potExtra *= 2.0f;
                }

                Add(player->GetUserName(), ( pot / winningTeam->NumHumanPlayers() + potExtra ) - bet[i] );
            }
            else
            {
                Add(player->GetUserName(),-bet[i]);
            }
        }
    }

    ladder(char *name,char *sd,int max=0)
            :highscores<REAL>(name,sd,max){
        //		Load();
    }

    virtual ~ladder(){
        //		Save();
    }
};


static highscores<int> highscore("highscores.txt","$highscore_description",100);
static highscores<int> highscore_won_rounds("won_rounds.txt",
        "$won_rounds_description");
static highscores<int> highscore_won_matches("won_matches.txt",
        "$won_matches_description");
static ladder highscore_ladder("ladder.txt",
                               "$ladder_description");

void check_hs(){
    if (sg_singlePlayer)
        if(se_PlayerNetIDs.Len()>0 && se_PlayerNetIDs(0)->IsHuman())
            highscore.Check(se_PlayerNetIDs(0),se_PlayerNetIDs(0)->Score());
}

gHighscoresBase::SaveAll();

highscore_won_rounds.Add(eTeam::teams[winner-1] ,1);
highscore_ladder.winner(eTeam::teams(winner-1));

check_hs();

highscore_won_matches.Add( winningTeam, 1 );

check_hs();

gHighscoresBase::LoadAll();

gHighscoresBase::SaveAll();

*/

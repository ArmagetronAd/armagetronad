%{
#include "gGame.h"
%}

%rename(GameSettings) gGameSettings;
class gGameSettings
{
public:
%rename(score_win) scoreWin;
    int scoreWin;    // score you get when you win a round

%rename(limit_time) limitTime;
    int limitTime;   // match time limit

%rename(limit_rounds) limitRounds;
    int limitRounds; // match round limit

%rename(limit_score) limitScore;
    int limitScore;  // match score limit

%rename(num_ais) numAIs;
    int numAIs;      // number of AI players
    
%rename(min_players) minPlayers;
    int minPlayers;  // minimum number of players

%rename(ai_iq) AI_IQ;
    int AI_IQ;       // preferred IQ of AI players
    
%rename(auto_num) autoNum;
    bool autoNum;    // automatically adjust number of AIs
    
%rename(auto_iq) autoIQ;
    bool autoIQ;     // automatically adjust IQ of AIs

%rename(speed_factor) speedFactor;
    int  speedFactor; // logarithm of cycle speed multiplier

%rename(size_factor) sizeFactor;
    int  sizeFactor;  // logarithm of arena size multiplier

%rename(auto_ai_fraction) autoAIFraction;
    int  autoAIFraction; // helper variable for the autoAI functions

%rename(win_zone_min_round_time) winZoneMinRoundTime;
    int	 winZoneMinRoundTime;	// minimum number of seconds a round must be going before the win zone is activated

%rename(win_zone_min_last_death) winZoneMinLastDeath;
    int	 winZoneMinLastDeath;	// minimum number of seconds the last death happended before the win zone is activated

//    gGameType   gameType;      // what type of game is played?
//    gFinishType finishType;    // what happens when all humans are dead?

    // team settings

%rename(min_teams) minTeams;
%rename(max_teams) maxTeams;
    int			minTeams, maxTeams;
    
%rename(min_players_per_team) minPlayersPerTeam;
%rename(max_players_per_team) maxPlayersPerTeam;
    int			minPlayersPerTeam, maxPlayersPerTeam;
    
%rename(max_team_imbalance) maxTeamImbalance;
    int			maxTeamImbalance;

%rename(balance_team_with_ais) balanceTeamsWithAIs;
%rename(enforce_team_rules_on_quit) enforceTeamRulesOnQuit;
    bool		balanceTeamsWithAIs, enforceTeamRulesOnQuit;

    // game mechanics settings
%rename(walls_stay_up_delay) wallsStayUpDelay;
    REAL		wallsStayUpDelay;	// the time the cycle walls stay up ( negative values: they stay up forever )
    
%rename(walls_length) wallsLength;
    REAL		wallsLength;		// the maximum total length of the walls
    
%rename(explosion_radius) explosionRadius;
    REAL		explosionRadius;	// the radius of the holes blewn in by an explosion

    gGameSettings(int a_scoreWin,
                  int a_limitTime, int a_limitRounds, int a_limitScore,
                  int a_numAIs,    int a_minPlayers,  int a_AI_IQ,
                  bool a_autoNum, bool a_autoIQ,
                  int a_speedFactor, int a_sizeFactor,
                  gGameType a_gameType,  gFinishType a_finishType,
                  int a_minTeams,
                  int a_winZoneMinRoundTime, int a_winZoneMinLastDeath
                 );

    void AutoAI(bool success);
    void Menu();
};

%rename(current_settings) sg_currentSettings;
extern gGameSettings* sg_currentSettings;

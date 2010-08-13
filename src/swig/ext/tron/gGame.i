%{
#include "gGame.h"
extern REAL se_GameTime();
extern bool GridIsReady(int c);
extern void sg_DeclareWinner( eTeam* team, char const * message );
%}


%extend gGame {
    static REAL game_time() {
        return se_GameTime();
    }
    static bool grid_is_ready(int c) {
        return GridIsReady(c);
    }
    static void declare_winner( eTeam* team, char const * message ) {
        sg_DeclareWinner( team, message );
    }
};

%rename(Game) gGame;
class gGame{
    bool GridIsReady(int c); // can we transfer gameObjects that need the grid to exist?

    void StartNewMatch();
};


#include "eTimer.h"
#include "gGame.h"
#include "gParser.h"
#include "nNetwork.h"
#include "zone/zEffector.h"
#include "zone/zTimedZone.h"
#include "zone/zZone.h"

static int sz_timedZoneDeath = 1;
static tSettingItem<int> sz_timedZoneDeathConf( "WIN_ZONE_DEATHS", sz_timedZoneDeath );

static tString sz_wz_custommsg("Zone activated!");
static tSettingItemLine sz_wzcm("WIN_ZONE_CUSTOM_SPAWN_MSG",sz_wz_custommsg);

//! creates a win or death zone (according to configuration) at the specified position
zZone * sz_CreateTimedZone( eGrid * grid, const eCoord & pos )
{
    zZone * zone = zZoneExtManager::Create("", grid);

    gParserState state;

    zone->setupVisuals(state);

    state.set("color", rColor(1, 1, 1, .7));
    state.set("scale", tFunction(1., 0.));

    tPolynomial tpRotation(2);
    tpRotation[0] = 0.0f;
    tpRotation[1] = .3f;
    state.set("rotation", tpRotation);

    gVectorExtra< nNetObjectID > noOwners;
    zEffectGroupPtr ZEG = zEffectGroupPtr(new zEffectGroup(noOwners, noOwners));
    Triad noTriad(_false);
    zValidatorPtr ZV = zValidatorPtr(zValidatorAll::create(noTriad, noTriad));
    zSelectorPtr ZS = zSelectorPtr(zSelectorSelf::create());
    ZS->setCount(-1);

    const char * type;
    if ( sz_timedZoneDeath )
    {
        type = "death";
        sn_ConsoleOut( "$instant_death_activated" );
    }
    else
    {
        type = "win";
        if(!sz_wz_player_win)
        {
            if(sz_wz_custommsg != "") sn_ConsoleOut(sz_wz_custommsg+"\n");
        }
        else if ( sg_currentSettings->gameType != gFREESTYLE )
        {
            sn_ConsoleOut( "$instant_win_activated" );
        }
        else
        {
            sn_ConsoleOut( "$instant_round_end_activated" );
        }
    }

    zEffectorPtr ZE = zEffectorPtr(zEffectorManager::Create(type));
    // FIXME: Trap any of the above being NULL
    ZE->setupVisuals(state);
    ZS->addEffector(ZE);
    ZV->addSelector(ZS);
    ZEG->addValidator(ZV);
    zone->addEffectGroupEnter(ZEG);

    zShapePtr shape = zShapePtr( new zShapeCircle(grid, zone) );

    shape->setReferenceTime( se_GameTime() );

    shape->applyVisuals(state);
    shape->setPosX( tFunction( pos.x, .0 ) );
    shape->setPosY( tFunction( pos.y, .0 ) );

    zone->setShape( shape );

    return zone;
}

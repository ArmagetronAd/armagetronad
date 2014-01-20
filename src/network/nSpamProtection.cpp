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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#include "nSpamProtection.h"
#include "nConfig.h"
#include "tSysTime.h"

static REAL se_SpamProtection = 4.0f;	// degree of spam protection
static REAL se_SpamPenalty	  = 0.0f;	// silence penalty when found guilty of spamming
static int  se_SpamAutoKickCount  = 3;	// minimal number of 
static REAL se_SpamAutoKick	  = 20.0f;	// spam value that causes someone to get instantly kicked.
int			se_SpamMaxLen	  = 80;		// maximal length of chat message

static tSettingItem<REAL> se_SPR("SPAM_PROTECTION",
                                 se_SpamProtection);
static tSettingItem<REAL> se_SPE("SPAM_PENALTY",
                                 se_SpamPenalty);
static tSettingItem<REAL> se_SAK("SPAM_AUTOKICK",
                                 se_SpamAutoKick);
static tSettingItem<int> se_SAKC("SPAM_AUTOKICK_COUNT",
                                 se_SpamAutoKickCount);

// prevent spam_maxlen from being set so low no admin can increase it back up
static bool sn_SpamMaxLenLimit(int const & value)
{
    static int minlen=strlen("/admin SPAM_MAXLEN 1000");
    return value >= minlen;
    // yeah, if the admin also logs out and it's reallylongname@clanwiththelongestname.com, 
    // he's screwed.
}
static nSettingItemWatched<int> se_SML("SPAM_MAXLEN",
                                       se_SpamMaxLen,
                                       nConfItemVersionWatcher::Group_Cheating,
                                       3 );
static tAccessLevelSetter se_SMLAL( se_SML.GetSetting().SetShouldChangeFunc(sn_SpamMaxLenLimit), tAccessLevel_Owner );


nSpamProtectionSettings::nSpamProtectionSettings( REAL timeScale, char const * timeScaleConfig, const tOutput& silence )
        : timeScale_( timeScale ), silence_( silence ), timeScaleSetting_( timeScaleConfig, timeScale_ )
{
}

nSpamProtection::nSpamProtection( const nSpamProtectionSettings& settings )
    : settings_( settings ), spamProtect_( 0.0f ), spamProtectTime_( tSysTimeFloat( )), numWarnings_( 0 )
{
}

nSpamProtection::~nSpamProtection( void )
{
}

REAL	nSpamProtection::BlockTime()					// time left in silenced mode
{
    REAL timeScale = this->settings_.timeScale_ * se_SpamProtection;
    return ( spamProtect_ - 6 ) * timeScale + ( tSysTimeFloat() - spamProtectTime_ );
}

// Reset spam time so everything that happened between last spammy event and now is erased
void nSpamProtection::ResetTime()
{
    double now = tSysTimeFloat();
    
    // but move ahead this much
    double ahead = 1;

    now += ahead;
    spamProtectTime_ = now;
 }
 
 void nSpamProtection::ResetSpam()
 {
     spamProtect_ = 0;
     numWarnings_ = 0;
 }

nSpamProtection::Level	nSpamProtection::CheckSpam( REAL spamlevel, int userToKick, tOutput const & reason )	// check if someone is spamming
{
    if ( se_SpamProtection < 0.01f )
    {
        se_SpamProtection = 0.01f;
    }

    REAL timeScale = this->settings_.timeScale_ * se_SpamProtection;

    spamProtect_ += spamlevel;
    double now = tSysTimeFloat();

    if(now < spamProtectTime_)
        now = spamProtectTime_;

    spamProtect_ -=( tSysTimeFloat() - spamProtectTime_ ) / timeScale;

    spamProtectTime_ = now;
    if ( spamProtect_ < 0 )
        spamProtect_ = 0;

    if ( spamProtect_ > 6 ){
        tOutput message;
        spamProtect_ += se_SpamPenalty;

        message.SetTemplateParameter(1, ( spamProtect_ - 6 ) * timeScale );
        message.Append( settings_.silence_ );

        //		message << ColorString (1,1,0);
        //		message << "$spam_protection";

        sn_ConsoleOut(message,userToKick);

        if ( spamProtect_ > se_SpamAutoKick && numWarnings_ >= se_SpamAutoKickCount )
        {
            tOutput message( "$network_kill_spamkick" );
            message.Append( " " );
            message.Append( reason );
            sn_KickUser( userToKick, message );

            return Level_Hard;
        }

        ++numWarnings_;
        return Level_Mild;
    }

    numWarnings_ = 0;
    return Level_Ok;
}

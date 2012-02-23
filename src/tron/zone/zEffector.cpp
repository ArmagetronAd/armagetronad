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

*/

#include "zEffector.h"
#include "tRandom.h"
#include "gGame.h"
// Only for SpawnPlayer:
#include "gParser.h"


void zEffector::apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    if (count == -1 || count > 0) {
        effect(d_calculatedTargets);
        /*
        tOutput asdf;
        asdf.SetTemplateParameter(1, tString("33"));
        asdf.SetTemplateParameter(2, tString("bibibi"));
        asdf.Append(message);
        sn_ConsoleOut(asdf);
        */
        if (count > 0)
            count --;
    }
}

void
zEffector::setMessage(tString unformated)
{/*
                                              tString res;
                                              for (size_t i=0; i< unformated.Size(); i++)
                                                {
                                                  char c = unformated(i);
                                                  if (c != '\\')
                                            	res += c;
                                                  else if (i < unformated.Size())
                                            	{
                                            	  switch (unformated(i+1))
                                            	    {
                                            	    case 'n':
                                            	      res += '\n';
                                            	      i++;
                                            	      break;
                                            	    case '1':
                                            	      res += '\1';
                                            	      i++;
                                            	      break;
                                            	    default:
                                            	      res += '\\';
                                            	      break;
                                            	    }
                                            	}
                                                }

                                              message = res;
                                             */
    message << unformated;
    /*
    message.Append( unformated );
    message << unformated.c_str();
    message.Append( unformated.c_str());
    */
}

void zEffector::applyContext(gParserState const &) {
}

void
zEffector::readXML(tXmlParser::node const & node)
{
    if (node.HasProp("count"))
        setCount(node.GetPropInt("count"));

    if (node.HasProp("description"))
        setMessage(node.GetProp("description"));
}

void
zEffector::setupVisuals(gParserState & state)
{
}


zEffectorManager::FactoryList &
zEffectorManager::_effectors()
{
    static FactoryList ifl;
    return ifl;
}

zEffector*
zEffectorManager::Create(std::string const & typex, tXmlParser::node const * node)
{
    std::string type = typex;
    transform (type.begin(), type.end(), type.begin(), tolower);

    FactoryList::const_iterator iterEffectorFactory;
    if ((iterEffectorFactory = _effectors().find(type)) == _effectors().end())
    {
#ifdef DEBUG
        tERR_WARN("Unknown effect \"" + type + '"');
#endif
        return zEffector::create();
    }
        
    
    VoidFactoryBase*Fy = iterEffectorFactory->second.get();

    if (NullFactory*ptr = dynamic_cast<NullFactory*>(Fy))
    {
        zEffector*rv = ptr->Factory();
        if (node)
            rv->readXML(*node);
        return rv;
    }
    if (XMLFactory*ptr = dynamic_cast<XMLFactory*>(Fy))
    {
        assert(node);
        return ptr->Factory(type, *node);
    }
    return NULL;
}

zEffector*
zEffectorManager::Create(std::string const & typex, tXmlParser::node const & node_p)
{
    return
    Create(typex, &node_p);
}

zEffector*
zEffectorManager::Create(std::string const & typex)
{
    return
    Create(typex, NULL);
}

void
zEffectorManager::Register(std::string const & type, std::string const & desc, NullFactory_t f)
{
    _effectors().insert(std::make_pair(type, new NullFactory(f)));
}
void
zEffectorManager::Register(std::string const & type, std::string const & desc, XMLFactory_t f)
{
    _effectors().insert(std::make_pair(type, new XMLFactory(f)));
}


static zEffectorRegistration regWin("win", "", zEffectorWin::create);

void zEffectorWin::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    // BOP
    static const char* message="$player_win_instant";
    // EOP

    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
            iter != d_calculatedTargets.end();
            ++iter)
    {
        sg_DeclareWinner((*iter)->CurrentTeam(), message );
    }
}

void
zEffectorWin::setupVisuals(gParserState & state)
{
    state.set("color", rColor(0, 1, 0, .7));
}

static int sz_score_deathzone=-1;
static tSettingItem<int> sz_dz("SCORE_DEATHZONE",sz_score_deathzone);

static zEffectorRegistration regDeath("death", "", zEffectorDeath::create);

void zEffectorDeath::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
            iter != d_calculatedTargets.end();
            ++iter)
    {
        (*iter)->AddScore(sz_score_deathzone, tOutput(), "$player_lose_suicide");
        (*iter)->Object()->Kill();
    }
}

void
zEffectorDeath::setupVisuals(gParserState & state)
{
    state.set("color", rColor(1, 0, 0, .7));
}

//
//
//

static zEffectorRegistration regPoint("point", "", zEffectorPoint::create);

void
zEffectorPoint::readXML(tXmlParser::node const & node)
{
    setPoint(node.GetPropInt("score"));
}

void zEffectorPoint::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
            iter != d_calculatedTargets.end();
            ++iter)
    {
        //	(*iter)->AddScore(d_score, tOutput(), "$player_lose_suicide");
        (*iter)->AddScore(d_score, tOutput(), message);
    }
}

//
//
//

static zEffectorRegistration regRubberRecharge("rubberrecharge", "", zEffectorCycleRubber::create);

void zEffectorCycleRubber::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
            iter != d_calculatedTargets.end();
            ++iter)
    {
        static_cast<gCycle *>((*iter)->Object())->SetRubber(0.0);
    }
}


//
//
//

static zEffectorRegistration regBrakeRecharge("brakerecharge", "", zEffectorCycleBrake::create);

void zEffectorCycleBrake::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
            iter != d_calculatedTargets.end();
            ++iter)
    {
        static_cast<gCycle *>((*iter)->Object())->SetBrakingReservoir(1.0);
    }
}

//
//
//

static zEffectorRegistration regAcceleration("acceleration", "", zEffectorCycleAcceleration::create);

void
zEffectorCycleAcceleration::readXML(tXmlParser::node const & node)
{
    string str = node.GetProp("value");
    setValue(tFunction().parse(str));
}

void zEffectorCycleAcceleration::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
            iter != d_calculatedTargets.end();
            ++iter)
    {
        static_cast<gCycle *>((*iter)->Object())->AddZoneAcceleration(acceleration.GetOffset());
    }
}

//
//
//

static zEffectorRegistration regSpawnPlayer("spawnplayer", "", zEffectorSpawnPlayer::create);

void zEffectorSpawnPlayer::applyContext(gParserState const & state) {
    setGrid( state.get<eGrid*>("grid") );
    setArena( state.get<gArena*>("arena") );
}

void zEffectorSpawnPlayer::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
            iter != d_calculatedTargets.end();
            ++iter)
    {
        sg_RespawnPlayer(grid, arena, (*iter));
    }
}

//
//
//

static zEffectorRegistration regSetting("setting", "", zEffectorSetting::create);

void
zEffectorSetting::readXML(tXmlParser::node const & node)
{
    if (node.HasProp("settingName"))
        setSettingName(node.GetProp("settingName"));
    if (node.HasProp("settingValue"))
        setSettingValue(node.GetProp("settingValue"));
}

void zEffectorSetting::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    std::stringstream ss;
    ss << settingName  << " " << settingValue;
    tConfItemBase::LoadAll(ss);

}

//
//
//

#ifdef ENABLE_SCRIPTING
static zEffectorRegistration regScripting("script", "", zEffectorScripting::create);

void
zEffectorScripting::readXML(tXmlParser::node const & node)
{
    if (node.HasProp("scriptName"))
    {
        callback = sScripting::GetInstance()->GetProcRef(node.GetProp("scriptName"));
    }
}

void zEffectorScripting::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    if (!callback) return;
    sArgs::ptr tmp;
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
            iter != d_calculatedTargets.end();
            ++iter)
    {
        *tmp << (*iter)->GetUserName();
    }
    *callback << *tmp;
    callback->Call();
    tmp->Clear();
}
#endif



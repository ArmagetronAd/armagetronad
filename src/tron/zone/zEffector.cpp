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

zEffector*
tEffectorManager::Create(const char*type, tXmlParser::node*node)
{
    FactoryList::const_iterator iterEffectorFactory;
    if ((iterEffectorFactory = _effectors.find(tString(type))) == _effectors.end())
        return NULL;
    
    VoidFactoryBase*Fy = iterEffectorFactory->second;

    if (NullFactory*ptr = dynamic_cast<NullFactory*>(Fy)) {
        return ptr->Factory();
    }
    else
    if (XMLFactory*ptr = dynamic_cast<XMLFactory*>(Fy)) {
        return ptr->Factory(type, node);
    }
    return NULL;
}

void
tEffectorManager::Register(const char*type, const char*desc, NullFactory_t f)
{
    _effectors[tString(type)] = new NullFactory(f);
}
void
tEffectorManager::Register(const char*type, const char*desc, XMLFactory_t f)
{
    _effectors[tString(type)] = new XMLFactory(f);
}

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

void zEffectorDeath::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
            iter != d_calculatedTargets.end();
            ++iter)
    {
        static_cast<gCycle *>((*iter)->Object())->Kill();
    }
}

//
//
//

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

void zEffectorSetting::effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
    std::stringstream ss;
    ss << settingName  << " " << settingValue;
    tConfItemBase::LoadAll(ss);

}



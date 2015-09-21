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

#include "cockpit/cCockpit.h"
#include "tValue.h"
#include "cockpit/cGauges.h"
#include "cockpit/cLabel.h"
#include "cockpit/cMap.h"
#include "cockpit/cRectangle.h"

#ifndef DEDICATED

#include "ePlayer.h"
#include "gCycle.h"
#include "eTimer.h"
#include "rRender.h"
#include "rFont.h"
#include "rScreen.h"
#include "eSensor.h"
#include <iostream>

#include <time.h>

static void parsecockpit () {
    cCockpit::GetCockpit()->ProcessCockpit();
}

static void readjust_cockpit () {
    cCockpit::GetCockpit()->Readjust();
}

static rCallbackAfterScreenModeChange reloadft(&readjust_cockpit);

static tString cockpit_file("Anonymous/standard-0.0.1.aacockpit.xml");
static tConfItem<tString> cf("COCKPIT_FILE",cockpit_file,&parsecockpit);

cCockpit::~cCockpit() {
    ClearWidgets();
}
cCockpit::cCockpit() :
        m_Cam(all),
        m_Player(0),
        m_FocusPlayer(0),
        m_ViewportPlayer(0),
        m_FocusCycle(0),
        m_Widgets_perplayer(),
m_Widgets_rootwindow() {
}

void cCockpit::ClearWidgets(void) {
    //while(!m_Widgets_perplayer.empty()) {
    //    delete m_Widgets_perplayer.front();
    //    m_Widgets_perplayer.pop_front();
    //}
    //while(!m_Widgets_rootwindow.empty()) {
    //    delete m_Widgets_rootwindow.front();
    //    m_Widgets_rootwindow.pop_front();
    //}
    m_Widgets_rootwindow.clear();
    m_Widgets_perplayer.clear();
    m_EventHandlers.clear();
}

void cCockpit::SetPlayer(ePlayer *player) {
    m_Player = player;
    m_ViewportPlayer = m_FocusPlayer = m_Player->netPlayer;
    if (player->cam) {
        for(int i =0 ; i< se_PlayerNetIDs.Len(); i++){
            ePlayerNetID *testPlayer = se_PlayerNetIDs[i];
            if(const eGameObject *testCycle = testPlayer->Object()) {
                if(player->cam->Center() == testCycle) {
                    m_FocusPlayer = testPlayer;
                }
            }
        }
    }
    if(m_FocusPlayer != NULL) {
        m_FocusCycle = dynamic_cast<gCycle *>(m_FocusPlayer->Object());
    } else {
        m_FocusCycle = 0;
    }
}

//callbacks
std::auto_ptr<tValue::Base> cCockpit::cb_CurrentRubber(void) {
    return std::auto_ptr<tValue::Base>(new tValue::Float(m_FocusCycle->GetRubber()));
}
std::auto_ptr<tValue::Base> cCockpit::cb_CurrentPing(void) {
    return std::auto_ptr<tValue::Base>(new tValue::Float(static_cast<int>(m_ViewportPlayer->ping*1000)));
}
std::auto_ptr<tValue::Base> cCockpit::cb_CurrentSpeed(void) {
    return std::auto_ptr<tValue::Base>(new tValue::Float(m_FocusCycle->Speed()));
}
std::auto_ptr<tValue::Base> cCockpit::cb_MaxSpeed(void) {
    return std::auto_ptr<tValue::Base>(new tValue::Int( static_cast<int>(ceil( m_FocusCycle->MaximalSpeed() / 10.) *10)));
}
std::auto_ptr<tValue::Base> cCockpit::cb_CurrentBrakingReservoir(void) {
    return std::auto_ptr<tValue::Base>(new tValue::Float(m_FocusCycle->GetBrakingReservoir()));
}
std::auto_ptr<tValue::Base> cCockpit::cb_AliveEnemies(void){
    int aliveenemies=0;
    unsigned short int max = se_PlayerNetIDs.Len();
    if(m_ViewportPlayer)
    {
        for(unsigned short int i=0;i<max;i++){
            ePlayerNetID *p=se_PlayerNetIDs(i);
            if(p->Object() && p->Object()->Alive() && p->CurrentTeam() != m_ViewportPlayer->CurrentTeam())
                aliveenemies++;
        }
    }
    return std::auto_ptr<tValue::Base>(new tValue::Int(aliveenemies));
}
std::auto_ptr<tValue::Base> cCockpit::cb_AliveTeammates(void){
    int alivemates=0;
    unsigned short int max = se_PlayerNetIDs.Len();
    if(m_ViewportPlayer)
    {
        for(unsigned short int i=0;i<max;i++){
            ePlayerNetID *p=se_PlayerNetIDs(i);
            if(p->Object() && p->Object()->Alive() && p->CurrentTeam() == m_ViewportPlayer->CurrentTeam())
                alivemates++;
        }
    }
    return std::auto_ptr<tValue::Base>(new tValue::Int(alivemates));
}

std::auto_ptr<tValue::Base> cCockpit::cb_Framerate(void){

    static int fps       = 60;
    static REAL lastTime = 0;

    const REAL newtime = tSysTimeFloat();
    const REAL ts      = newtime - lastTime;

    int newfps   = static_cast<int>(se_AverageFPS());
    if (fabs((newfps-fps)*ts)>4)
    {
        fps      = newfps;
        lastTime = newtime;
    }
    return std::auto_ptr<tValue::Base>(new tValue::Int(fps));
}

std::auto_ptr<tValue::Base> cCockpit::cb_RunningTime(void){
    return std::auto_ptr<tValue::Base>(new tValue::Float(tSysTimeFloat()));
}

std::auto_ptr<tValue::Base> cCockpit::cb_CurrentTimeHours(void){
    struct tm* thisTime;
    time_t rawtime;

    time ( &rawtime );
    thisTime = localtime ( &rawtime );

    return std::auto_ptr<tValue::Base>(new tValue::Int(thisTime->tm_hour));
}

std::auto_ptr<tValue::Base> cCockpit::cb_CurrentTimeHours12h(void){
    struct tm* thisTime;
    time_t rawtime;

    time ( &rawtime );
    thisTime = localtime ( &rawtime );

    return std::auto_ptr<tValue::Base>(new tValue::Int((thisTime->tm_hour+11)%12+1));
}

std::auto_ptr<tValue::Base> cCockpit::cb_CurrentTimeMinutes(void){
    struct tm* thisTime;
    time_t rawtime;

    time ( &rawtime );
    thisTime = localtime ( &rawtime );

    return std::auto_ptr<tValue::Base>(new tValue::Int(thisTime->tm_min));
}

std::auto_ptr<tValue::Base> cCockpit::cb_CurrentTimeSeconds(void){
    struct tm* thisTime;
    time_t rawtime;

    time ( &rawtime );
    thisTime = localtime ( &rawtime );

    return std::auto_ptr<tValue::Base>(new tValue::Int(thisTime->tm_sec));
}

std::auto_ptr<tValue::Base> cCockpit::cb_CurrentScore(void){
    return std::auto_ptr<tValue::Base>(new tValue::Int(m_ViewportPlayer->TotalScore()));
}
std::auto_ptr<tValue::Base> cCockpit::cb_TopScore(void){
    int topscore = 0;
    for(int i =0 ; i< se_PlayerNetIDs.Len(); i++){
        int thisscore = se_PlayerNetIDs[i]->TotalScore();
        thisscore>topscore?topscore=thisscore:1;
    }
    return std::auto_ptr<tValue::Base>(new tValue::Int(topscore));
}

std::auto_ptr<tValue::Base> cCockpit::cb_FastestSpeed(void){
    REAL max=0;
    //tString name;

    for(int i =0 ; i< se_PlayerNetIDs.Len(); i++){
        ePlayerNetID *p = se_PlayerNetIDs[i];

        if(gCycle *h = (gCycle*)(p->Object())){
            if (h->Speed()>max && h->Alive()){
                max =  (float) h->Speed();  // changed to float for more accuracy in reporting top speed
                //name = p->GetName();
            }
        }
    }
    return std::auto_ptr<tValue::Base>(new tValue::Float(max));
}

std::auto_ptr<tValue::Base> cCockpit::cb_TimeToImpactFront(void){
    eSensor test(m_FocusCycle, m_FocusCycle->Position(), m_FocusCycle->Direction());
    test.detect(5.*m_FocusCycle->Speed());
    return std::auto_ptr<tValue::Base>(new tValue::Float(test.hit/m_FocusCycle->Speed()));
}
std::auto_ptr<tValue::Base> cCockpit::cb_TimeToImpactRight(void){
    eSensor test(m_FocusCycle, m_FocusCycle->Position(), m_FocusCycle->Direction().Turn(0,-1));
    test.detect(5.*m_FocusCycle->Speed());
    return std::auto_ptr<tValue::Base>(new tValue::Float(test.hit/m_FocusCycle->Speed()));
}
std::auto_ptr<tValue::Base> cCockpit::cb_TimeToImpactLeft(void){
    eSensor test(m_FocusCycle, m_FocusCycle->Position(), m_FocusCycle->Direction().Turn(0,1));
    test.detect(5.*m_FocusCycle->Speed());
    return std::auto_ptr<tValue::Base>(new tValue::Float(test.hit/m_FocusCycle->Speed()));
}

std::auto_ptr<tValue::Base> cCockpit::cb_FastestName(void){
    REAL max=0;
    tString name;

    for(int i =0 ; i< se_PlayerNetIDs.Len(); i++){
        ePlayerNetID *p = se_PlayerNetIDs[i];

        if(gCycle *h = (gCycle*)(p->Object())){
            if (h->Speed()>max && h->Alive()){
                max =  (float) h->Speed();  // changed to float for more accuracy in reporting top speed
                name = p->GetName();
            }
        }
    }
    return std::auto_ptr<tValue::Base>(new tValue::String(name));
}

cCockpit* cCockpit::_instance = 0;

void cCockpit::ProcessCockpit(void) {
    ClearWidgets();

    if (!LoadWithParsing(cockpit_file)) return;
    node cur = GetFileContents();
    if(!cur) {
        tERR_WARN("No Cockpit node found!");
    }
    if (m_Type != "aacockpit") {
        tERR_WARN("Type 'aacockpit' expected, found '" << cur.GetProp("type") << "' instead");
        return;
    }
    if (cur.IsOfType("Cockpit")) {
        ProcessWidgets(cur);
	if(sr_screenWidth != 0) Readjust();
        return;
    } else {
        tERR_WARN("Found a node of type '" + cur.GetName() + "' where type 'Cockpit' was expected");
        return;
    }
}

void cCockpit::ProcessWidgets(node cur) {
    std::map<tString, node> templates;
    for(cur = cur.GetFirstChild(); cur; ++cur) {
        if (cur.IsOfType("text") || cur.IsOfType("comment")) continue;
	if (cur.IsOfType("WidgetTemplate")) {
	    templates[cur.GetProp("id")] = cur;
	    continue;
	}
        cWidget::Base_ptr widget_ptr = ProcessWidgetType(cur);
        if(&*widget_ptr == 0) {
            tERR_WARN("Unknown Widget type '" + cur.GetName() + "'");
            continue;
        }
        cWidget::Base &widget = *widget_ptr;

	//Process all templates first
	tString use(cur.GetProp("usetemplate"));

	use += " "; //add the extra seperator, makes things easier
	int pos = 0;
	int next;
	while((next = use.find(' ', pos)) != -1) {
	    tString thisuse = use.SubStr(pos, next - pos);
	    if(templates.count(thisuse)) {
		ProcessWidget(templates[thisuse], widget);
	    } else if (!thisuse.empty()) {
		tERR_WARN(tString("Nothing known about template id '") + thisuse + "'.");
	    }
	    pos = next+1;
	}

	ProcessWidget(cur, widget);
        if(cur.GetProp("viewport") == "all") {
            m_Widgets_perplayer.push_back(widget_ptr.release());
        } else {
            m_Widgets_rootwindow.push_back(widget_ptr.release());
        }
    }
}

void cCockpit::ProcessWidget(node cur, cWidget::Base &widget) {
        ProcessWidgetCamera(cur, widget);
        int num;
        cur.GetProp("toggle", num);
        m_EventHandlers.insert(std::pair<int, cWidget::Base *>(num, &widget));
        widget.SetDefaultState(cur.GetPropBool("toggleDefault"));
        widget.SetSticky(cur.GetPropBool("toggleSticky"));
        ProcessWidgetCore(cur, widget);
}

cWidget::Base_ptr cCockpit::ProcessWidgetType(node cur) {
    if(cur.IsOfType("NeedleGauge"))
        return cWidget::Base_ptr(new cWidget::NeedleGauge());
    if(cur.IsOfType("BarGauge"))
        return cWidget::Base_ptr(new cWidget::BarGauge());
    if(cur.IsOfType("VerticalBarGauge"))
        return cWidget::Base_ptr(new cWidget::VerticalBarGauge());
    if(cur.IsOfType("Label"))
        return cWidget::Base_ptr(new cWidget::Label());
    if(cur.IsOfType("Map"))
        return cWidget::Base_ptr(new cWidget::Map());
    if(cur.IsOfType("Rectangle"))
        return cWidget::Base_ptr(new cWidget::Rectangle());
    return cWidget::Base_ptr();
}

void cCockpit::ProcessWidgetCamera(node cur, cWidget::Base &widget) {
    tString cam(cur.GetProp("camera"));
    if(cam.size() == 0) {
        tERR_WARN("Empty camera string");
        widget.SetCam(all);
    }
    std::map<tString, unsigned> cams;
    cams[tString("custom")] = custom;
    cams[tString("follow")] = follow;
    cams[tString("free")] = free;
    cams[tString("in")] = in;
    cams[tString("server_custom")] = server_custom;
    cams[tString("smart")] = smart;
    cams[tString("all")] = all;
    cams[tString("*")] = all;

    unsigned ret=0;
    bool invert=false;
    if(cam(0) == '^') {
        invert=true;
        cam.erase(0,1);
    }
    cam += " "; //add the extra seperator, makes things easier

    int pos = 0;
    int next;
    while((next = cam.find(' ', pos)) != -1) {
        tString thiscam = cam.SubStr(pos, next - pos);
        if(cams.count(thiscam)) {
            ret |= cams[thiscam];
        } else {
            tERR_WARN(tString("Nothing known about camera type '") + thiscam + "'.");
        }
        pos = next+1;
    }
    if(invert) {
        ret = ~ret;
    }
    widget.SetCam(ret);
}

void cCockpit::ProcessWidgetCore(node cur, cWidget::Base &widget) {
    for (cur = cur.GetFirstChild(); cur; ++cur) {
        tString name = cur.GetName();
        if(name == "comment" || name == "text") continue;
        widget.Process(cur);
    }
}


cCockpit* cCockpit::GetCockpit() {
    static cCockpit theHud;

    return &theHud;
}

void cCockpit::RenderPlayer() {
    if( m_Player == 0 ) return;

    if(m_FocusPlayer != 0 && m_ViewportPlayer != 0) {
        sr_ResetRenderState(false);
        Color(1,1,1);
        if(m_Player->cam) {

            if (m_FocusCycle && ( !m_Player->netPlayer || !m_Player->netPlayer->IsChatting()) && se_GameTime()>-2){
                //h->Speed()>maxmeterspeed?maxmeterspeed+=10:1;

                for(unsigned int i=0; i<m_Widgets_perplayer.size(); i++)
                {
                    if (m_Widgets_perplayer[i]->GetCam() & m_Cam && m_Widgets_perplayer[i]->Active())
                        m_Widgets_perplayer[i]->Render();
                }
                //  bool displayfastest = true;// put into global, set via menusytem... subby to do.make sr_DISPLAYFASTESTout

            }
        }
    }

}
void cCockpit::RenderRootwindow() {
    sr_ResetRenderState(true);
    glViewport (GLsizei(0),
                GLsizei(0),
                GLsizei(sr_screenWidth),
                GLsizei(sr_screenWidth));

    //BeginLineLoop();
    //Color(1.,1.,1.,1.);
    //Vertex(-.1,-.1);
    //Vertex( .1,-.1);
    //Vertex( .1, .1);
    //Vertex(-.1, .1);
    //RenderEnd();
    for(unsigned int i=0; i<m_Widgets_rootwindow.size(); i++) {
        if(m_Widgets_rootwindow[i]->Active()) {
            m_Widgets_rootwindow[i]->Render();
        }
    }
}


static void display_cockpit_lucifer() {
    //std::cerr << "display_cockpit_lucifer()\n";
    cCockpit* theCockpit = cCockpit::GetCockpit();

    static bool loaded = false;
    if(!loaded) {
        theCockpit->ProcessCockpit();
        loaded = true;
    }

    sr_ResetRenderState(true);

    if (!(se_mainGameTimer &&
            se_mainGameTimer->speed > .9 &&
            se_mainGameTimer->speed < 1.1 &&
            se_mainGameTimer->IsSynced() )) return;

    rViewportConfiguration* viewportConfiguration = rViewportConfiguration::CurrentViewportConfiguration();

    for ( int viewport = viewportConfiguration->num_viewports-1; viewport >= 0; --viewport )
    {
        // get the ID of the player in the viewport
        int playerID = sr_viewportBelongsToPlayer[ viewport ];

        // get the player
        ePlayer* player = ePlayer::PlayerConfig( playerID );

        // select the corrected viewport
        viewportConfiguration->Port( viewport )->CorrectAspectBottom().Select();

        // delegate
        theCockpit->SetPlayer( player );
        theCockpit->RenderPlayer();
    }

    theCockpit->RenderRootwindow();
}

static rPerFrameTask dfps(&display_cockpit_lucifer);

static uActionGlobal cockpitKey1("COCKPIT_KEY_1");
static uActionGlobal cockpitKey2("COCKPIT_KEY_2");
static uActionGlobal cockpitKey3("COCKPIT_KEY_3");
static uActionGlobal cockpitKey4("COCKPIT_KEY_4");
static uActionGlobal cockpitKey5("COCKPIT_KEY_5");
static uActionGlobalFunc ck1(&cockpitKey1, &cCockpit::ProcessKey1, true);
static uActionGlobalFunc ck2(&cockpitKey2, &cCockpit::ProcessKey2, true);
static uActionGlobalFunc ck3(&cockpitKey3, &cCockpit::ProcessKey3, true);
static uActionGlobalFunc ck4(&cockpitKey4, &cCockpit::ProcessKey4, true);
static uActionGlobalFunc ck5(&cockpitKey5, &cCockpit::ProcessKey5, true);

bool cCockpit::ProcessKey1(float i) { return cCockpit::GetCockpit()->HandleEvent(1, i>0); }
bool cCockpit::ProcessKey2(float i) { return cCockpit::GetCockpit()->HandleEvent(2, i>0); }
bool cCockpit::ProcessKey3(float i) { return cCockpit::GetCockpit()->HandleEvent(3, i>0); }
bool cCockpit::ProcessKey4(float i) { return cCockpit::GetCockpit()->HandleEvent(4, i>0); }
bool cCockpit::ProcessKey5(float i) { return cCockpit::GetCockpit()->HandleEvent(5, i>0); }

bool cCockpit::HandleEvent(int id, bool state) {
    if(m_EventHandlers.count(id)){
        for(std::multimap<int, cWidget::Base *>::iterator iter = m_EventHandlers.find(id); iter != m_EventHandlers.end() && iter->first == id; ++iter) {
            iter->second->Toggle(state);
        }
        return true;
    }
    return false;
}

void cCockpit::Readjust(void) {
    if (sr_screenWidth == 0) return;
    std::cerr << "sr_screenWidth: " << sr_screenWidth << std::endl;
    std::cerr << "sr_screenHeight: " << sr_screenHeight << std::endl;
    float factor = 4./3. / (static_cast<float>(sr_screenWidth)/static_cast<float>(sr_screenHeight));
    //float factor = 2.;
    for(tAutoDeque<cWidget::Base>::iterator iter = m_Widgets_perplayer.begin(); iter != m_Widgets_perplayer.end(); ++iter) {
        if(cWidget::WithCoordinates *coordWidget = dynamic_cast<cWidget::WithCoordinates *>(*iter)) {
            coordWidget->SetFactor(factor);
        }
    }
    for(tAutoDeque<cWidget::Base>::iterator iter = m_Widgets_rootwindow.begin(); iter != m_Widgets_rootwindow.end(); ++iter) {
        if(cWidget::WithCoordinates *coordWidget = dynamic_cast<cWidget::WithCoordinates *>(*iter)) {
            coordWidget->SetFactor(factor);
        }
    }
}

#endif

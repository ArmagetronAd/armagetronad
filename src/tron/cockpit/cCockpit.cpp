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

#include "cockpit/cCockpit.h"
#include "tValue.h"
#include "values/vParser.h"
#include "cockpit/cGauges.h"
#include "cockpit/cLabel.h"
#include "cockpit/cMap.h"
#include "cockpit/cRectangle.h"
#include "nConfig.h"

#ifndef DEDICATED

#include "ePlayer.h"
#include "gCycle.h"
#include "eTimer.h"
#include "eTeam.h"
#include "eCamera.h"
#include "rRender.h"
#include "rFont.h"
#include "rScreen.h"
#include "eSensor.h"
#include <iostream>
#include "eSoundMixer.h"

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

typedef std::pair<tString, tValue::Callback<cCockpit>::cb_ptr> cbpair;
static const cbpair cbarray[] = {
                                    cbpair(tString("player_rubber")       , &cCockpit::cb_CurrentRubber),
                                    cbpair(tString("player_acceleration") , &cCockpit::cb_CurrentAcceleration),
                                    cbpair(tString("current_ping")        , &cCockpit::cb_CurrentPing),
                                    cbpair(tString("player_speed")        , &cCockpit::cb_CurrentSpeed),
                                    cbpair(tString("max_speed")           , &cCockpit::cb_MaxSpeed),
                                    cbpair(tString("player_brakes")       , &cCockpit::cb_CurrentBrakingReservoir),
                                    cbpair(tString("enemies_alive")       , &cCockpit::cb_AliveEnemies),
                                    cbpair(tString("friends_alive")       , &cCockpit::cb_AliveTeammates),
                                    cbpair(tString("current_framerate")   , &cCockpit::cb_Framerate),
                                    cbpair(tString("time_since_start")    , &cCockpit::cb_RunningTime),
                                    cbpair(tString("current_minutes")     , &cCockpit::cb_CurrentTimeMinutes),
                                    cbpair(tString("current_hours")       , &cCockpit::cb_CurrentTimeHours),
                                    cbpair(tString("current_hours12h")    , &cCockpit::cb_CurrentTimeHours12h),
                                    cbpair(tString("current_seconds")     , &cCockpit::cb_CurrentTimeSeconds),
                                    cbpair(tString("current_score")       , &cCockpit::cb_CurrentScore),
                                    cbpair(tString("top_score")           , &cCockpit::cb_TopScore),
                                    cbpair(tString("current_score_team")  , &cCockpit::cb_CurrentScoreTeam),
                                    cbpair(tString("top_score_team")      , &cCockpit::cb_TopScoreTeam),
                                    cbpair(tString("fastest_speed")       , &cCockpit::cb_FastestSpeed),
                                    cbpair(tString("fastest_name")        , &cCockpit::cb_FastestName),
                                    cbpair(tString("fastest_speed_round") , &cCockpit::cb_FastestSpeedRound),
                                    cbpair(tString("fastest_name_round")  , &cCockpit::cb_FastestNameRound),
                                    cbpair(tString("time_to_impact_front"), &cCockpit::cb_TimeToImpactFront),
                                    cbpair(tString("time_to_impact_right"), &cCockpit::cb_TimeToImpactRight),
                                    cbpair(tString("time_to_impact_left") , &cCockpit::cb_TimeToImpactLeft),
                                    cbpair(tString("current_song")        , &cCockpit::cb_CurrentSong),
                                    cbpair(tString("current_name")        , &cCockpit::cb_CurrentName),
                                    cbpair(tString("current_colored_name"), &cCockpit::cb_CurrentColoredName),
                                    cbpair(tString("current_pos_x")       , &cCockpit::cb_CurrentPosX),
                                    cbpair(tString("current_pos_y")       , &cCockpit::cb_CurrentPosY)
                                };
std::map<tString, tValue::Callback<cCockpit>::cb_ptr> const stc_callbacks(cbarray, cbarray+sizeof(cbarray)/sizeof(cbpair));

std::set<tString> stc_forbiddenCallbacks;
#endif
static tString stc_forbiddenCallbacksString;
#ifndef DEDICATED


static void reparseforbiddencallbacks(void) {
    stc_forbiddenCallbacks.clear();

    tString callbacks = stc_forbiddenCallbacksString + ":"; //add the extra separator, makes things easier

    size_t pos = 0;
    size_t next;
    while((next = callbacks.find(':', pos)) != tString::npos) {
        tString callback = callbacks.SubStr(pos, next - pos);
        if(stc_callbacks.count(callback)) {
            stc_forbiddenCallbacks.insert(callback);
        }
        pos = next+1;
    }
    parsecockpit();
}

static nSettingItem<tString> fcs("FORBID_COCKPIT_DATA", stc_forbiddenCallbacksString,&reparseforbiddencallbacks);
#else
static nSettingItem<tString> fcs("FORBID_COCKPIT_DATA", stc_forbiddenCallbacksString);
#endif
#ifndef DEDICATED


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
    if(m_FocusPlayer != 0) {
        m_FocusCycle = dynamic_cast<gCycle *>(m_FocusPlayer->Object());
    } else {
        m_FocusCycle = 0;
    }
}

//callbacks
tValue::BasePtr cCockpit::cb_CurrentRubber(void) {
    return tValue::BasePtr(new tValue::Float(m_FocusCycle->GetRubber()));
}
tValue::BasePtr cCockpit::cb_CurrentAcceleration(void) {
    return tValue::BasePtr(new tValue::Float(m_FocusCycle->GetAcceleration()));
}
tValue::BasePtr cCockpit::cb_CurrentPing(void) {
    return tValue::BasePtr(new tValue::Float(static_cast<int>(m_ViewportPlayer->ping*1000)));
}
tValue::BasePtr cCockpit::cb_CurrentSpeed(void) {
    return tValue::BasePtr(new tValue::Float(m_FocusCycle->Speed()));
}
tValue::BasePtr cCockpit::cb_MaxSpeed(void) {
    return tValue::BasePtr(new tValue::Int( static_cast<int>(ceil( m_FocusCycle->MaximalSpeed() / 10.) *10)));
}
tValue::BasePtr cCockpit::cb_CurrentBrakingReservoir(void) {
    return tValue::BasePtr(new tValue::Float(m_FocusCycle->GetBrakingReservoir()));
}
tValue::BasePtr cCockpit::cb_AliveEnemies(void){
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
    return tValue::BasePtr(new tValue::Int(aliveenemies));
}
tValue::BasePtr cCockpit::cb_AliveTeammates(void){
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
    return tValue::BasePtr(new tValue::Int(alivemates));
}

tValue::BasePtr cCockpit::cb_Framerate(void){

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
    return tValue::BasePtr(new tValue::Int(fps));
}

tValue::BasePtr cCockpit::cb_RunningTime(void){
    return tValue::BasePtr(new tValue::Float(tSysTimeFloat()));
}

tValue::BasePtr cCockpit::cb_CurrentTimeHours(void){
    struct tm* thisTime;
    time_t rawtime;

    time ( &rawtime );
    thisTime = localtime ( &rawtime );

    return tValue::BasePtr(new tValue::Int(thisTime->tm_hour));
}

tValue::BasePtr cCockpit::cb_CurrentTimeHours12h(void){
    struct tm* thisTime;
    time_t rawtime;

    time ( &rawtime );
    thisTime = localtime ( &rawtime );

    return tValue::BasePtr(new tValue::Int((thisTime->tm_hour+11)%12+1));
}

tValue::BasePtr cCockpit::cb_CurrentTimeMinutes(void){
    struct tm* thisTime;
    time_t rawtime;

    time ( &rawtime );
    thisTime = localtime ( &rawtime );

    return tValue::BasePtr(new tValue::Int(thisTime->tm_min));
}

tValue::BasePtr cCockpit::cb_CurrentTimeSeconds(void){
    struct tm* thisTime;
    time_t rawtime;

    time ( &rawtime );
    thisTime = localtime ( &rawtime );

    return tValue::BasePtr(new tValue::Int(thisTime->tm_sec));
}

REAL stc_fastestSpeedRound = .0;
REAL stc_fastestSpeed;
tString stc_fastestNameRound;
tString stc_fastestName;
int stc_topScore;

tValue::BasePtr cCockpit::cb_CurrentScore(void){
    return tValue::BasePtr(new tValue::Int(m_ViewportPlayer->TotalScore()));
}
tValue::BasePtr cCockpit::cb_TopScore(void){
    return tValue::BasePtr(new tValue::Int(stc_topScore));
}

tValue::BasePtr cCockpit::cb_CurrentScoreTeam(void){
    if(m_ViewportPlayer->CurrentTeam() == 0) {
        return tValue::BasePtr(new tValue::Base());
    }
    return tValue::BasePtr(new tValue::Int(m_ViewportPlayer->CurrentTeam()->Score()));
}
tValue::BasePtr cCockpit::cb_TopScoreTeam(void){
    int max = 0;
    for(int i=0;i<eTeam::teams.Len();++i){
        eTeam *t = eTeam::teams(i);
        if(t->Score() > max) max = t->Score();
    }
    return tValue::BasePtr(new tValue::Int(max));
}

static void stc_updateFastest() {
    stc_fastestSpeed = 0.;
    stc_topScore = 0;
    for(int i =0 ; i< se_PlayerNetIDs.Len(); i++){
        ePlayerNetID *p = se_PlayerNetIDs[i];

        if(gCycle *h = (gCycle*)(p->Object())){
            if (h->Speed()>stc_fastestSpeedRound && h->Alive()){
                stc_fastestSpeedRound =  (float) h->Speed();  // changed to float for more accuracy in reporting top speed
                stc_fastestNameRound = p->GetName();
            }
            if (h->Speed()>stc_fastestSpeed && h->Alive()){
                stc_fastestSpeed =  (float) h->Speed();  // changed to float for more accuracy in reporting top speed
                stc_fastestName = p->GetName();
            }
        }
        int thisscore = se_PlayerNetIDs[i]->TotalScore();
        if(thisscore>stc_topScore)
            stc_topScore=thisscore;
    }
}

static rPerFrameTask stcuf(&stc_updateFastest);

tValue::BasePtr cCockpit::cb_FastestSpeedRound(void){
    return tValue::BasePtr(new tValue::Float(stc_fastestSpeedRound));
}

tValue::BasePtr cCockpit::cb_FastestNameRound(void){
    return tValue::BasePtr(new tValue::String(stc_fastestNameRound));
}

tValue::BasePtr cCockpit::cb_FastestSpeed(void){
    return tValue::BasePtr(new tValue::Float(stc_fastestSpeed));
}

tValue::BasePtr cCockpit::cb_FastestName(void){
    return tValue::BasePtr(new tValue::String(stc_fastestName));
}

tValue::BasePtr cCockpit::cb_TimeToImpactFront(void){
    eSensor test(m_FocusCycle, m_FocusCycle->Position(), m_FocusCycle->Direction());
    test.detect(5.*m_FocusCycle->Speed());
    return tValue::BasePtr(new tValue::Float(test.hit/m_FocusCycle->Speed()));
}
tValue::BasePtr cCockpit::cb_TimeToImpactRight(void){
    eSensor test(m_FocusCycle, m_FocusCycle->Position(), m_FocusCycle->Direction().Turn(0,-1));
    test.detect(5.*m_FocusCycle->Speed());
    return tValue::BasePtr(new tValue::Float(test.hit/m_FocusCycle->Speed()));
}
tValue::BasePtr cCockpit::cb_TimeToImpactLeft(void){
    eSensor test(m_FocusCycle, m_FocusCycle->Position(), m_FocusCycle->Direction().Turn(0,1));
    test.detect(5.*m_FocusCycle->Speed());
    return tValue::BasePtr(new tValue::Float(test.hit/m_FocusCycle->Speed()));
}

tValue::BasePtr cCockpit::cb_CurrentSong(void){
    return tValue::BasePtr(new tValue::String(eSoundMixer::GetMixer()->GetCurrentSong()));
}

tValue::BasePtr cCockpit::cb_CurrentName(void) {
    return tValue::BasePtr(new tValue::String(tString(m_FocusPlayer->GetName())));
}

tValue::BasePtr cCockpit::cb_CurrentColoredName(void) {
    tColoredString ret;
    ret << *m_FocusPlayer;
    return tValue::BasePtr(new tValue::String(ret));
}

tValue::BasePtr cCockpit::cb_CurrentPosX(void) {
    return tValue::BasePtr(new tValue::Float(m_FocusCycle->Position().x));
}

tValue::BasePtr cCockpit::cb_CurrentPosY(void) {
    return tValue::BasePtr(new tValue::Float(m_FocusCycle->Position().x));
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
    cams[tString("mer")] = mer;
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
    if(m_FocusPlayer != 0 && m_ViewportPlayer != 0) {
        sr_ResetRenderState(false);
        Color(1,1,1);
        if(m_Player->cam) {

            if (m_FocusCycle && ( !m_Player->netPlayer || !m_Player->netPlayer->IsChatting()) && se_GameTime()>-2){
                //h->Speed()>maxmeterspeed?maxmeterspeed+=10:1;

                for(unsigned int i=0; i<m_Widgets_perplayer.size(); i++)
                {
                    int cam = m_Widgets_perplayer[i]->GetCam();
                    switch(m_Player->cam->GetCamMode()) {
                    case CAMERA_IN:
                    case CAMERA_SMART_IN:
                        if(!(cam & in)) continue;
                        break;
                    case CAMERA_CUSTOM:
                        if(!(cam & custom)) continue;
                        break;
                    case CAMERA_FREE:
                        if(!(cam & free)) continue;
                        break;
                    case CAMERA_FOLLOW:
                        if(!(cam & follow)) continue;
                        break;
                    case CAMERA_SMART:
                        if(!(cam & smart)) continue;
                        break;
                    case CAMERA_SERVER_CUSTOM:
                        if(!(cam & server_custom)) continue;
                        break;
                    case CAMERA_MER:
                        if(!(cam & mer)) continue;
                        break;
                    case CAMERA_COUNT:
                        continue; //not handled, no sense?!
                    }
                    if (m_Widgets_perplayer[i]->Active())
                        m_Widgets_perplayer[i]->Render();
                }
                //  bool displayfastest = true;// put into global, set via menusytem... subby to do.make sr_DISPLAYFASTESTout

            }
        }
    }

}
void cCockpit::RenderRootwindow() {
    if( m_Player == 0 ) return;
    if(m_ViewportPlayer == 0) return;

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

#if 0	// Testing ground :)
    vValue::Expr::Core::Base *test = vValue::Parser::parse(tString("10"));
    std::cerr << "test: " << test->GetValue() << std::endl;
#endif
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

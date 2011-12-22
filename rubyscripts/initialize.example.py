print("Start scripting initialization.")

import sys, os
print os.getcwd()
sys.path.append('./src/swig/ext/')
import armagetronad

def player_entered(args):
    print args

def round_winner(args):
    p=args[1]
    print args
    print("PLAYER_MESSAGE "+p+' "Congratulation from script '+p+' !"')
    armagetronad.ConfItem.load_line("PLAYER_MESSAGE "+p+' "Congratulation from script '+p+' !"')
    ci = armagetronad.ConfItem.find("CYCLE_SPEED")
    print "cycle_speed " + ci.get()
    ci.set("20")

def new_round(args):
    ci = armagetronad.ConfItem.find("CONSOLE_MESSAGE")
    ci.set(str(armagetronad.Team.num_teams()))
    for i in range(armagetronad.Team.num_teams()):
        t=armagetronad.Team.team(i)
        ci.set("Team name: "+t.name())
        for i in range(t.num_players()):
            p=t.player(i)
            ci.set("Player name: "+p.name())

def death_frag(args):
    print args
    print(args[1])
    p=armagetronad.Player.find_player(args[1])
    o=armagetronad.Player.find_player(args[0])
    if p.is_human():
        print "> congratulation !"
        p.set_name("killer")
        points = 10
        out = armagetronad.Output(args[1]+" was awarded "+str(points)+" points because I want to!\n")
        p.add_score_output(points,out,out)
        c = p.cycle()
        pos = c.position()
        dirt = c.direction()
        speed = c.speed()
        print "pos ("+str(pos.x)+","+str(pos.y)+")"
        print "dir ("+str(dirt.x)+","+str(dirt.y)+")"
        print "speed "+str(speed)
        dest = pos + ( dirt * speed ) 
        c.drop_wall(0)
        time = c.last_time()
        c.move_safely(dest, time, time+0.01)
        c.drop_wall(1)
        c.request_sync_all()
        c = o.cycle()
        pos = o.position()
        dirt = o.direction()
        speed = o.speed()
        dest = pos + ( dirt * speed ) 
        armagetronad.Cycle.respawn_cycle(dest, dirt, o, 0)

armagetronad.LadderLogWriter.find("ROUND_WINNER").set_callback(round_winner)
armagetronad.LadderLogWriter.find("PLAYER_ENTERED").set_callback(player_entered)
armagetronad.LadderLogWriter.find("NEW_ROUND").set_callback(new_round)
armagetronad.LadderLogWriter.find("DEATH_FRAG").set_callback(death_frag)

def script_config(args):
    print ">", args

test = armagetronad.ConfItemScript("TEST_SCRIPT", script_config)
test_al = armagetronad.AccessLevelSetter(test, armagetronad.tAccessLevel_Moderator)

ci=armagetronad.ConfItem.find("CONSOLE_MESSAGE")
if ci.required_level() == armagetronad.tAccessLevel_Moderator:
    print "correct"

armagetronad.ConfItem.load_all("CONSOLE_MESSAGE test line 1\nCONSOLE_MESSAGE test line 2\nCONSOLE_MESSAGE test line 3")
armagetronad.ConfItem.load_line("CONSOLE_MESSAGE test line 4\nCONSOLE_MESSAGE test line 5\nCONSOLE_MESSAGE test line 6")

writer = armagetronad.LadderLogWriter.find("PLAYER_ENTERED")
if not(writer.is_enabled()):
    writer.enabled(1)
    print "turned on"
else:
    writer.enabled(0)
    print "turned off"

if not(writer.is_enabled()):
    writer.enabled(1)
    print "turned on"
else:
    writer.enabled(0)
    print "turned off"

print("End scripting initialization.")


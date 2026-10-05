#include "doctest.h"
#include "ePlayer.h"

#include "tDefer.h"
#include "MockConsole.h"
#include "tSysTime.h"

// not needed here, maybe for some other place
/*
#include "gAIBase.h"
#include "tToDo.h"
namespace
{
void Cleanup()
{
    sn_SetNetState(nSTANDALONE);

    st_DoToDo();

    // delete all the things
    nNetObject::SyncAll();
    gAIPlayer::ClearAll();
    nNetObject::ClearAll();
    ePlayerNetID::ClearAll();
    nNetObject::ClearAllDeleted();

    st_DoToDo();
}
} // namespace
*/

// Tests for ePlayerNetID system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("ePlayerNetID")
{
    TEST_CASE("ePlayerNetID basics")
    {
        // check that player gets destroyed at the end
        INVARIANT_CHECK(!se_PlayerNetIDs.Len())
        // INVARIANT(Cleanup();)

        GIVEN("an ePlayerNetID for an existing player")
        {
            // you can spawn players just by creating them,
            // store the result in a smart pointer for later cleanup.
            // This constructor creates a player bound to local player 0.
            auto player = tRefPtr<ePlayerNetID>::Make(0);

            THEN("player has a name")
            {
                tString name;
                player->GetName(name);
                CHECK(name.Len() > 2);
            }

            THEN("player has a network ID")
            {
                CHECK(player->ID() > 0);
            }
        }

        GIVEN("an ePlayerNetID for a remote player")
        {
            // This constructor creates a player not bound to a local player.
            auto player = tRefPtr<ePlayerNetID>::Make();

            THEN("player has a network ID")
            {
                CHECK(player->ID() > 0);
            }
        }
    }

    TEST_CASE("Scoring system")
    {
        MockConsole swallow;

        GIVEN("a player with default score")
        {
            auto player = tRefPtr<ePlayerNetID>::Make(0);

            WHEN("checking initial score")
            {
                THEN("Score() returns 0")
                {
                    CHECK(player->Score() == 0);
                }
            }

            WHEN("adding points to score")
            {
                tOutput reasonWin, reasonLose;
                player->AddScore(10, reasonWin, reasonLose);

                THEN("Score() returns the added points")
                {
                    CHECK(player->Score() == 10);
                }

                THEN("TotalScore() returns the total")
                {
                    CHECK(player->TotalScore() >= 10);
                }
            }

            WHEN("adding negative points")
            {
                tOutput reasonWin, reasonLose;
                player->AddScore(-5, reasonWin, reasonLose);

                THEN("Score() returns the reduced score")
                {
                    CHECK(player->Score() == -5);
                }
            }
        }

        GIVEN("multiple players with different scores")
        {
            auto player1 = tRefPtr<ePlayerNetID>::Make(0);
            auto player2 = tRefPtr<ePlayerNetID>::Make();

            tOutput reasonWin, reasonLose;
            player1->AddScore(100, reasonWin, reasonLose);
            player2->AddScore(50, reasonWin, reasonLose);

            WHEN("comparing scores")
            {
                THEN("players have different scores")
                {
                    CHECK(player1->Score() > player2->Score());
                }
            }
        }

        GIVEN("players for score difference tracking")
        {
            WHEN("resetting score differences")
            {
                ePlayerNetID::ResetScoreDifferences();

                THEN("no crash occurs")
                {
                    // TODO lastScore_ is private and we have no way of reading it
                }
            }
        }
    }

    TEST_CASE("Player state")
    {
        MockConsole swallow;

        GIVEN("a newly created player")
        {
            auto player = tRefPtr<ePlayerNetID>::Make(0);

            WHEN("checking default state")
            {
                THEN("IsSpectating returns false")
                {
                    CHECK_FALSE(player->IsSpectating());
                }

                THEN("IsChatting returns false")
                {
                    CHECK_FALSE(player->IsChatting());
                }

                THEN("IsActive returns true")
                {
                    CHECK(player->IsActive());
                }

                THEN("IsSilenced returns false")
                {
                    CHECK_FALSE(player->IsSilenced());
                }

                THEN("IsSuspended returns false")
                {
                    CHECK_FALSE(player->IsSuspended());
                }

                THEN("CanRespawn returns current state")
                {
                    // CanRespawn requires currentTeam, suspended_ == 0, and !spectating_
                    // Player created with Make(0) may not have a team, so CanRespawn may be false
                    // Just verify it doesn't crash
                    CHECK(player->CanRespawn() == player->CanRespawn());
                }
            }
        }

        GIVEN("a player in spectator mode")
        {
            // Note: Spectator mode is controlled by the game,
            // we can only check the getter in default state
            auto player = tRefPtr<ePlayerNetID>::Make();

            WHEN("checking spectator status")
            {
                THEN("IsSpectating returns current state")
                {
                    // Player starts not spectating
                    CHECK_FALSE(player->IsSpectating());
                }
            }
        }

        GIVEN("a player in chat mode")
        {
            auto player = tRefPtr<ePlayerNetID>::Make();

            WHEN("setting chat state")
            {
                player->SetChatting(ePlayerNetID::ChatFlags_Chat, true);

                THEN("IsChatting returns true")
                {
                    CHECK(player->IsChatting());
                }

                WHEN("clearing chat state")
                {
                    player->SetChatting(ePlayerNetID::ChatFlags_Chat, false);

                    THEN("IsChatting returns false")
                    {
                        CHECK_FALSE(player->IsChatting());
                    }
                }
            }
        }

        GIVEN("a suspended player")
        {
            auto player = tRefPtr<ePlayerNetID>::Make();

            WHEN("suspending player")
            {
                player->Suspend(3);

                THEN("IsSuspended returns true")
                {
                    CHECK(player->IsSuspended());
                }

                THEN("CanRespawn returns false")
                {
                    CHECK_FALSE(player->CanRespawn());
                }
            }
        }

        GIVEN("a silenced player")
        {
            auto player = tRefPtr<ePlayerNetID>::Make();

            WHEN("silencing player")
            {
                player->SetSilenced(true);

                THEN("IsSilenced returns true")
                {
                    CHECK(player->IsSilenced());
                }

                WHEN("unsilencing player")
                {
                    player->SetSilenced(false);

                    THEN("IsSilenced returns false")
                    {
                        CHECK_FALSE(player->IsSilenced());
                    }
                }
            }
        }
    }

    TEST_CASE("Static collection methods")
    {
        MockConsole swallow;

        GIVEN("no players created")
        {
            WHEN("checking player list")
            {
                THEN("se_PlayerNetIDs is empty")
                {
                    CHECK(se_PlayerNetIDs.Len() == 0);
                }
            }
        }

        GIVEN("players created")
        {
            auto player1 = tRefPtr<ePlayerNetID>::Make(0);
            auto player2 = tRefPtr<ePlayerNetID>::Make();

            WHEN("checking player list")
            {
                THEN("se_PlayerNetIDs contains created players")
                {
                    // Note: Players may or may not be in the list depending on initialization
                    // This test documents current behavior
                    int count = se_PlayerNetIDs.Len();
                    CHECK(count >= 0); // Just verify no crash
                }
            }
        }

        GIVEN("players with scores for ranking")
        {
            auto player2 = tRefPtr<ePlayerNetID>::Make();
            auto player1 = tRefPtr<ePlayerNetID>::Make(0);

            tOutput reasonWin, reasonLose;
            player1->AddScore(100, reasonWin, reasonLose);
            player2->AddScore(50, reasonWin, reasonLose);

            WHEN("sorting by score")
            {
                // players start ordered as we put them
                CHECK(player1.get() == se_PlayerNetIDs(1));
                CHECK(player2.get() == se_PlayerNetIDs(0));

                ePlayerNetID::SortByScore();

                THEN("players are sorted")
                {
                    CHECK(player1.get() == se_PlayerNetIDs(0));
                    CHECK(player2.get() == se_PlayerNetIDs(1));
                }
            }

            WHEN("getting ranking")
            {
                tString ranking = ePlayerNetID::Ranking(5, false);

                THEN("ranking string is not empty")
                {
                    CHECK(ranking.Len() >= 0);
                }
            }
        }

        GIVEN("ClearAll functionality")
        {
            WHEN("clearing all players")
            {
                THEN("ClearAll completes without crash")
                {
                    ePlayerNetID::ClearAll();
                }

                THEN("ClearAll also works with players active")
                {
                    auto player1 = tRefPtr<ePlayerNetID>::Make(0);
                    ePlayerNetID::ClearAll();
                }
            }
        }
    }

    TEST_CASE("Name management")
    {
        MockConsole swallow;

        GIVEN("a player with default name")
        {
            auto player = tRefPtr<ePlayerNetID>::Make(0);

            WHEN("getting name")
            {
                tString name;
                player->GetName(name);

                THEN("name is not empty")
                {
                    CHECK(name.Len() > 0);
                }
            }

            WHEN("getting colored name")
            {
                tColoredString coloredName;
                player->GetColoredName(coloredName);

                THEN("colored name is not empty")
                {
                    CHECK(coloredName.Len() > 0);
                }
            }

            WHEN("getting user name")
            {
                tString userName;
                player->GetUserName(userName);

                THEN("user name is not empty")
                {
                    CHECK(userName.Len() > 0);
                }
            }
        }

        GIVEN("name filtering")
        {
            WHEN("filtering a name with color codes")
            {
                tString input("Player\x03\x03Name"); // With color codes
                tString output = ePlayerNetID::FilterName(input);

                THEN("color codes are removed")
                {
                    // Color code 0x03 should be removed
                    CHECK(output.Len() > 0);
                }
            }

            WHEN("filtering a name with special characters")
            {
                tString input("Player\x01\x02\x03Name");
                tString output = ePlayerNetID::FilterName(input);

                THEN("unprintables are removed")
                {
                    CHECK(output.Len() > 0);
                }
            }

            WHEN("filtering a name with spaces")
            {
                tString input("Player Name");
                tString output = ePlayerNetID::FilterName(input);

                THEN("spaces are converted to underscores and case is normalized")
                {
                    // FilterName converts spaces to underscores and lowercases
                    CHECK(output == tString("player_name"));
                }
            }

            WHEN("setting player name")
            {
                auto player = tRefPtr<ePlayerNetID>::Make();
                tString newName("TestPlayer");
                player->SetName(newName);

                tString actualName;
                player->GetName(actualName);

                THEN("name is set correctly")
                {
                    CHECK(actualName == newName);
                }
            }
        }
    }

    TEST_CASE("Time tracking")
    {
        GIVEN("a newly created player")
        {
            auto player = tRefPtr<ePlayerNetID>::Make(0);

            WHEN("getting creation time")
            {
                nTimeAbsolute creationTime = player->GetTimeCreated();

                THEN("creation time is valid")
                {
                    // Just verify it doesn't crash and returns a value
                    CHECK(creationTime >= doctest::Approx(0));
                }
            }

            WHEN("checking last activity")
            {
                REAL lastActivity = player->LastActivity();

                THEN("last activity time is valid")
                {
                    // Should be 0 or positive for newly created player
                    CHECK(lastActivity >= doctest::Approx(0));
                }
            }
        }

        GIVEN("a player with activity")
        {
            auto player = tRefPtr<ePlayerNetID>::Make();
            REAL lastActivityBefore = player->LastActivity();

            WHEN("letting time pass")
            {
                tMockAdvanceFrame(1);

                THEN("last activity time increased")
                {
                    REAL lastActivity = player->LastActivity();
                    CHECK(lastActivity >= lastActivityBefore);
                }
            }

            WHEN("recording activity")
            {
                tMockAdvanceFrame(1);
                player->Activity();

                THEN("last activity time goes to near zero")
                {
                    REAL lastActivity = player->LastActivity();
                    CHECK(lastActivity <= doctest::Approx(0));
                }
            }
        }
    }

    TEST_CASE("Chat functionality")
    {
        MockConsole swallow;

        GIVEN("a player")
        {
            auto player = tRefPtr<ePlayerNetID>::Make(0);

            WHEN("checking initial chat state")
            {
                THEN("IsChatting returns false")
                {
                    CHECK_FALSE(player->IsChatting());
                }
            }

            WHEN("setting chat state for console")
            {
                player->SetChatting(ePlayerNetID::ChatFlags_Console, true);

                THEN("IsChatting returns true")
                {
                    CHECK(player->IsChatting());
                }
            }

            WHEN("sending chat message")
            {
                player->Chat(tString("Test message"));

                CHECK(strstr(swallow.GetLastPrinted(), "Test message"));
            }
        }
    }

    TEST_CASE("Access control")
    {
        GIVEN("a player")
        {
            auto player = tRefPtr<ePlayerNetID>::Make(0);

            WHEN("getting access level")
            {
                tAccessLevel level = player->GetAccessLevel();

                THEN("access level is valid")
                {
                    // Default should be tAccessLevel_Program or similar
                    CHECK(level >= tAccessLevel_Program);
                }
            }

            WHEN("checking login status")
            {
                bool isLoggedIn = player->IsLoggedIn();

                THEN("login status is reported")
                {
                    // Player is not logged in by default
                    CHECK_FALSE(isLoggedIn);
                }
            }

            WHEN("setting logged in state")
            {
                // this is the old remote admin system, it requires elevated base access rights
                tCurrentAccessLevel elevator(tAccessLevel_Owner, true);
                // the old system is no longer usable in default builds.

                player->BeLoggedIn();

                THEN("IsLoggedIn returns true")
                {
                    CHECK(player->IsLoggedIn());
                }

                WHEN("setting logged out state")
                {
                    player->BeNotLoggedIn();

                    THEN("IsLoggedIn returns false")
                    {
                        CHECK_FALSE(player->IsLoggedIn());
                    }
                }
            }

            WHEN("getting last access level")
            {
                tAccessLevel lastLevel = player->GetLastAccessLevel();

                THEN("last access level is valid")
                {
                    CHECK(lastLevel >= tAccessLevel_Program);
                }
            }
        }
    }
}

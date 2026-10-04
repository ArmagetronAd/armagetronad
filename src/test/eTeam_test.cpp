#include "doctest.h"

#include "tSysTime.h"
#include "tDefer.h"

#include "eTeam.h"
#include "ePlayer.h"

#include "MockConsole.h"

// Tests for eTeam system
// Purpose: Document the status quo behavior and detect regressions

TEST_SUITE("eTeam")
{
    TEST_CASE("eTeam base tests")
    {
        MockConsole mockConsole; // swallow log output

        GIVEN("eTeam")
        {
            auto team = tRefPtr<eTeam>::Make();

            THEN("it can take up one ePlayerNetID")
            {
                auto player = tRefPtr<ePlayerNetID>::Make();
                player->SetName("Z");
                team->AddPlayer(player);

                AND_THEN("it takes the player's name")
                {
                    CHECK(team->Name().StartsWith("Z"));
                }
            }
        }
    }

    // Management Functions Tests (fn-9-1)
    TEST_CASE("eTeam Management Functions")
    {
        MockConsole mockConsole;

        GIVEN("a team and players")
        {
            auto team = tRefPtr<eTeam>::Make();
            auto player1 = tRefPtr<ePlayerNetID>::Make();
            auto player2 = tRefPtr<ePlayerNetID>::Make();
            auto player3 = tRefPtr<ePlayerNetID>::Make();

            WHEN("adding players to the team")
            {
                REQUIRE(team->NumPlayers() == 0);

                team->AddPlayer(player1);
                THEN("team has 1 player")
                {
                    CHECK(team->NumPlayers() == 1);

                    // two ways to check team membership (we prefer the first):
                    CHECK(player1->CurrentTeam() == team);
                    CHECK(team->Player(0) == player1);
                }

                team->AddPlayer(player2);
                THEN("team has 2 players")
                {
                    CHECK(team->NumPlayers() == 2);
                    CHECK(player1->CurrentTeam() == team);
                    CHECK(player2->CurrentTeam() == team);
                    CHECK(player3->CurrentTeam() == nullptr);
                }

                team->AddPlayer(player3);
                THEN("team has 3 players")
                {
                    CHECK(team->NumPlayers() == 3);
                }
            }

            WHEN("removing players from the team")
            {
                team->AddPlayer(player1);
                team->AddPlayer(player2);
                team->AddPlayer(player3);
                REQUIRE(team->NumPlayers() == 3);

                team->RemovePlayer(player2);
                THEN("team has 2 players after removal")
                {
                    CHECK(team->NumPlayers() == 2);
                    CHECK(player1->CurrentTeam() == team);
                    CHECK(player2->CurrentTeam() == nullptr);
                    CHECK(player3->CurrentTeam() == team);
                }

                team->RemovePlayer(player1);
                team->RemovePlayer(player3);
                THEN("team is empty after removing all players")
                {
                    CHECK(team->NumPlayers() == 0);
                }
            }

            WHEN("adding player with AddPlayerDirty")
            {
                int originalNumPlayers = team->NumPlayers();
                team->AddPlayerDirty(player1);
                THEN("player is added without calling UpdateProperties")
                {
                    CHECK(team->NumPlayers() == originalNumPlayers + 1);
                    CHECK(team->Player(team->NumPlayers() - 1) == player1);
                }
            }

            WHEN("checking if new team is allowed")
            {
                // Just verify it doesn't crash and returns a boolean
                bool allowed = eTeam::NewTeamAllowed();
                CHECK(allowed);
            }

            WHEN("checking if player may join")
            {
                bool mayJoin1 = team->PlayerMayJoin(player1);
                bool mayJoin2 = team->PlayerMayJoin(player2);
                CHECK(mayJoin1);
                CHECK(mayJoin2);
            }
        }
    }

    // Scoring System Tests (fn-9-2)
    TEST_CASE("eTeam Scoring System")
    {
        MockConsole mockConsole;

        GIVEN("a team")
        {
            auto team = tRefPtr<eTeam>::Make();

            WHEN("checking initial score")
            {
                CHECK(team->Score() == 0);
            }

            WHEN("adding points to score")
            {
                team->AddScore(10);
                CHECK(team->Score() == 10);

                team->AddScore(5);
                CHECK(team->Score() == 15);
            }

            WHEN("setting score to specific value")
            {
                team->SetScore(100);
                CHECK(team->Score() == 100);
            }

            WHEN("resetting score")
            {
                team->AddScore(50);
                REQUIRE(team->Score() == 50);
                team->ResetScore();
                CHECK(team->Score() == 0);
            }

            WHEN("adding score with reasons")
            {
                tOutput reasonWin, reasonLose;
                team->AddScore(25, reasonWin, reasonLose);
                CHECK(team->Score() == 25);
            }
        }
    }

    // Player Queries Tests (fn-9-4)
    TEST_CASE("eTeam Player Queries")
    {
        MockConsole mockConsole;

        GIVEN("a team with multiple players")
        {
            auto team = tRefPtr<eTeam>::Make();
            auto player1 = tRefPtr<ePlayerNetID>::Make();
            auto player2 = tRefPtr<ePlayerNetID>::Make();
            auto player3 = tRefPtr<ePlayerNetID>::Make();

            team->AddPlayer(player1);
            tMockAdvanceFrame(0.1);
            team->AddPlayer(player2);
            tMockAdvanceFrame(0.1);
            team->AddPlayer(player3);

            WHEN("querying player count")
            {
                CHECK(team->NumPlayers() == 3);
            }

            WHEN("accessing players by index")
            {
                // using the normally not preferred team check method, testing that players are added in order
                CHECK(team->Player(0) == player1);
                CHECK(team->Player(1) == player2);
                CHECK(team->Player(2) == player3);
            }

            WHEN("checking if team has human/AI players")
            {
                int humans = team->NumHumanPlayers();
                int ais = team->NumAIPlayers();
                CHECK(humans >= 0);
                CHECK(ais >= 0);
                CHECK(humans + ais <= team->NumPlayers());
            }

            WHEN("checking oldest and youngest players")
            {
                auto oldest = team->OldestPlayer();
                auto youngest = team->YoungestPlayer();
                CHECK(oldest != nullptr);
                CHECK(youngest != nullptr);
                CHECK(youngest != oldest);
            }

            WHEN("checking oldest and youngest human players")
            {
                auto oldest = team->OldestHumanPlayer();
                auto youngest = team->YoungestHumanPlayer();
                CHECK(oldest != nullptr);
                CHECK(youngest != nullptr);
            }

            WHEN("checking if team is alive")
            {
                bool isAlive = team->Alive();
                CHECK(!isAlive); // we have not spawned any game objects, so nobody is alive yet
            }
        }
    }

    // State Management Tests (fn-9-5)
    TEST_CASE("eTeam State Management")
    {
        MockConsole mockConsole;

        GIVEN("a team")
        {
            auto team = tRefPtr<eTeam>::Make();

            WHEN("checking initial state")
            {
                // TeamID may be -1 for newly created teams
                int teamID = team->TeamID();
                CHECK(teamID >= -1); // Allow -1 for uninitialized
                CHECK(team->Name().Len() > 0);
                CHECK(team->R() <= 255);
                CHECK(team->G() <= 255);
                CHECK(team->B() <= 255);
            }

            WHEN("setting lock state")
            {
                bool original = team->IsLocked();
                team->SetLocked(!original);
                CHECK(team->IsLocked() == !original);

                team->SetLocked(original);
                CHECK(team->IsLocked() == original);
            }

            WHEN("inviting and uninviting players")
            {
                auto player = tRefPtr<ePlayerNetID>::Make();

                team->Invite(player);
                CHECK(team->IsInvited(player) == true);

                team->UnInvite(player);
                CHECK(team->IsInvited(player) == false);
            }

            WHEN("checking rounds played")
            {
                int original = team->RoundsPlayed();
                team->PlayRound();
                CHECK(team->RoundsPlayed() == original + 1);
            }
        }
    }

    // Properties and Appearance Tests (fn-9-5 continued)
    TEST_CASE("eTeam Properties and Appearance")
    {
        MockConsole mockConsole;

        GIVEN("a team")
        {
            auto team = tRefPtr<eTeam>::Make();

            WHEN("getting colored name")
            {
                tColoredString colored = team->GetColoredName();
                CHECK(colored.Len() >= 3);
            }

            WHEN("printing name")
            {
                tString nameStr;
                team->PrintName(nameStr);
                CHECK(nameStr.Len() >= 3);
            }
        }
    }

    // Balancing Logic Tests (fn-9-3)
    TEST_CASE("eTeam Balancing Logic")
    {
        MockConsole mockConsole;

        eTeam::EnforceConstraints();

        int maxTeams = eTeam::maxTeams;
        int maxPlayers = eTeam::maxPlayers;
        int maxImbalance = eTeam::maxImbalance;
        CHECK(maxTeams >= 0);
        CHECK(maxPlayers >= 0);
        CHECK(maxImbalance >= 0);

        auto team = tRefPtr<eTeam>::Make();
        bool balance = team->BalanceThisTeam();
        CHECK(balance);

        bool isHuman = team->IsHuman();
        CHECK(isHuman);
    }

    // Static Methods Tests (fn-9-6)
    TEST_CASE("eTeam Static Methods")
    {
        MockConsole mockConsole;

        GIVEN("two teams with onep player")
        {
            auto team1 = tRefPtr<eTeam>::Make();
            auto team2 = tRefPtr<eTeam>::Make();
            auto player1 = tRefPtr<ePlayerNetID>::Make();
            auto player2 = tRefPtr<ePlayerNetID>::Make();
            team1->AddPlayer(player1);
            team2->AddPlayer(player2);
            THEN("they are considered enemies")
            {
                CHECK(ePlayerNetID::Enemies(player1, player2));
                CHECK(!eTeam::Enemies(team1, player1));
                CHECK(!eTeam::Enemies(team2, player2));
                CHECK(eTeam::Enemies(team1, player2));
                CHECK(eTeam::Enemies(team2, player1));
                CHECK(eTeam::Enemies(team1, team2));
            }
        }

        GIVEN("two teams with diferent scores")
        {
            INVARIANT_CHECK(eTeam::teams.Len() == 0);

            auto team1 = tRefPtr<eTeam>::Make();
            auto team2 = tRefPtr<eTeam>::Make();

            // teams need players to be added to the list
            auto p1 = tRefPtr<ePlayerNetID>::Make();
            auto p2 = tRefPtr<ePlayerNetID>::Make();
            team1->AddPlayer(p1);
            team2->AddPlayer(p2);

            team2->AddScore(1);

            THEN("they can be sorted")
            {
                CHECK(eTeam::teams(0) == team1);
                CHECK(eTeam::teams(1) == team2);

                eTeam::SortByScore();

                CHECK(eTeam::teams(0) == team2);
                CHECK(eTeam::teams(1) == team1);
            }

            THEN("they can be swapped")
            {
                int numTeams = eTeam::teams.Len();
                CHECK(numTeams == 2);

                CHECK(eTeam::teams(0) == team1);
                CHECK(eTeam::teams(1) == team2);

                eTeam::SwapTeamsNo(0, 1);

                CHECK(eTeam::teams(0) == team2);
                CHECK(eTeam::teams(1) == team1);
            }

            THEN("there is some ranking")
            {
                tString ranking = eTeam::Ranking();
                CHECK(ranking.Len() >= 10);
            }
        }
    }

    // Update Methods Tests (fn-9-6)
    TEST_CASE("eTeam Update Methods")
    {
        MockConsole mockConsole;

        auto team = tRefPtr<eTeam>::Make();

        team->UpdateProperties();
        team->UpdateAppearance();
        team->Update();

        // no assert, just do not crash
    }
}

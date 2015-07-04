/*
All Hands On Deck
    Copyright (C) 2015 Vladimir "allejo" Jimenez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "bzfsAPI.h"
#include "plugin_utils.h"

static void killAllPlayers ()
{
    bz_APIIntList *playerList = bz_newIntList();
    bz_getPlayerIndexList(playerList);

    for (unsigned int i = 0; i < playerList->size(); i++)
    {
        int playerID = playerList->get(i);

        if (bz_getPlayerByIndex(playerID)->spawned && bz_killPlayer(playerID, true))
        {
            bz_setPlayerLosses(playerID, bz_getPlayerLosses(playerID) - 1);
        }
    }

    bz_deleteIntList(playerList);
}

static void sendToPlayers (bz_eTeamType team, std::string message)
{
    bz_APIIntList *playerList = bz_newIntList();
    bz_getPlayerIndexList(playerList);

    for (unsigned int i = 0; i < playerList->size(); i++)
    {
        int playerID = playerList->get(i);

        if (bz_getPlayerByIndex(playerID)->team == team)
        {
            bz_sendTextMessagef(playerID, playerID, message.c_str());
        }
    }

    bz_deleteIntList(playerList);
}

static std::string teamColorLiteral (bz_eTeamType teamColor)
{
    switch (teamColor)
    {
        case eBlueTeam:
            return "Blue";

        case eGreenTeam:
            return "Green";

        case ePurpleTeam:
            return "Purple";

        case eRedTeam:
            return "Red";

        default:
            return "Rogue";
    }
}

static std::string teamToTeamFlag (bz_eTeamType team)
{
  switch (team)
  {
    case eBlueTeam:
      return "B*";

    case eGreenTeam:
      return "G*";

    case ePurpleTeam:
      return "P*";

    case eRedTeam:
      return "R*";

    default:
      return "";
  }
}

class AhodZone : public bz_CustomZoneObject
{
public:
    AhodZone() : bz_CustomZoneObject() {}
};

class AllHandsOnDeck : public bz_Plugin, bz_CustomMapObjectHandler
{
public:
    virtual const char* Name () {return "All Hands On Deck";}
    virtual void Init (const char* config);
    virtual void Event (bz_EventData *eventData);
    virtual void Cleanup (void);
    virtual bool MapObject (bz_ApiString object, bz_CustomMapObjectInfo *data);

    virtual bool isInsideAhodZone (int playerID);
    virtual bool isEntireTeamOnBase (bz_eTeamType team);
    virtual bool doesTeamHaveEnemyFlag (bz_eTeamType team, bz_eTeamType enemy);

    AhodZone ahodZone;

    bz_eTeamType teamOne, teamTwo;
};

BZ_PLUGIN(AllHandsOnDeck)

void AllHandsOnDeck::Init (const char* /* commandLine */)
{
    bz_registerCustomMapObject("AHOD", this);

    teamOne = eNoTeam;
    teamTwo = eNoTeam;

    for (bz_eTeamType t = eRedTeam; t <= ePurpleTeam; t = (bz_eTeamType) (t + 1))
    {
        if (bz_getTeamPlayerLimit(t) > 0)
        {
            if      (teamOne == eNoTeam) teamOne = t;
            else if (teamTwo == eNoTeam) teamTwo = t;
        }
    }

    Register(bz_eAllowCTFCaptureEvent);
    Register(bz_ePlayerJoinEvent);
    Register(bz_ePlayerPausedEvent);
    Register(bz_eTickEvent);
}

void AllHandsOnDeck::Cleanup (void)
{
    Flush();

    bz_removeCustomMapObject("AHOD");
}

bool AllHandsOnDeck::MapObject (bz_ApiString object, bz_CustomMapObjectInfo *data)
{
    if (object != "AHOD" || !data)
    {
        return false;
    }

    ahodZone.handleDefaultOptions(data);

    return true;
}

void AllHandsOnDeck::Event (bz_EventData *eventData)
{
    switch (eventData->eventType)
    {
        case bz_eAllowCTFCaptureEvent: // This event is called each time a flag is about to be captured
        {
            bz_AllowCTFCaptureEventData_V1* allowCtfData = (bz_AllowCTFCaptureEventData_V1*)eventData;

            allowCtfData->allow = false;
        }
        break;

        case bz_ePlayerJoinEvent: // This event is called each time a player joins the game
        {
            bz_PlayerJoinPartEventData_V1* joinData = (bz_PlayerJoinPartEventData_V1*)eventData;
            int playerID = joinData->playerID;

            bz_sendTextMessage(playerID, playerID, "********************************************************************");
            bz_sendTextMessage(playerID, playerID, " ");
            bz_sendTextMessage(playerID, playerID, "  --- How To Play ---");
            bz_sendTextMessage(playerID, playerID, "     Take the enemy flag to the neutral (green) base along with your");
            bz_sendTextMessage(playerID, playerID, "     entire team in order to cap. If any player is missing, you will");
            bz_sendTextMessage(playerID, playerID, "     not be able to cap. Teamwork matters!");
            bz_sendTextMessage(playerID, playerID, " ");
            bz_sendTextMessage(playerID, playerID, "********************************************************************");
        }
        break;

        case bz_ePlayerPausedEvent: // This event is called each time a playing tank is paused
        {
            bz_PlayerPausedEventData_V1* pauseData = (bz_PlayerPausedEventData_V1*)eventData;

            if (isInsideAhodZone(pauseData->playerID) && pauseData->pause)
            {
                bz_killPlayer(pauseData->playerID, false);
                bz_sendTextMessage(BZ_SERVER, pauseData->playerID, "Pausing in the AHOD zone is not permitted.");
            }
        }
        break;

        case bz_eTickEvent: // This event is called once for each BZFS main loop
        {
            // AHOD is only enabled if there are at least 2 players per team
            if (bz_getTeamCount(teamOne) < 2 || bz_getTeamCount(teamTwo) < 2)
            {
                return;
            }

            bool teamOneAhod = isEntireTeamOnBase(teamOne);
            bool teamTwoAhod = isEntireTeamOnBase(teamTwo);

            if (teamOneAhod || teamTwoAhod)
            {
                bool teamOneHasEnemyFlag = doesTeamHaveEnemyFlag(teamOne, teamTwo);
                bool teamTwoHasEnemyFlag = doesTeamHaveEnemyFlag(teamTwo, teamOne);

                if (teamOneHasEnemyFlag || teamTwoHasEnemyFlag)
                {
                    if ((teamOneAhod && teamOneHasEnemyFlag) || (teamTwoAhod && teamTwoHasEnemyFlag))
                    {
                        bz_eTeamType victor = (teamOneHasEnemyFlag) ? teamOne : teamTwo;
                        bz_eTeamType loser  = (teamOneHasEnemyFlag) ? teamTwo : teamOne;

                        killAllPlayers();
                        bz_incrementTeamWins(victor, 1);
                        bz_incrementTeamLosses(loser, 1);
                        bz_resetFlags(false);

                        sendToPlayers(loser, std::string("Team flag captured by the " + teamColorLiteral(victor) + " team!"));
                        sendToPlayers(victor, std::string("Great teamwork! Don't let them capture your flag!"));
                    }
                }
            }
        }
        break;

        default: break;
    }
}

bool AllHandsOnDeck::doesTeamHaveEnemyFlag(bz_eTeamType team, bz_eTeamType enemy)
{
    bz_APIIntList *playerList = bz_newIntList();
    bz_getPlayerIndexList(playerList);

    bool doesHaveEnemyFlag = false;

    for (unsigned int i = 0; i < playerList->size(); i++ )
    {
        int playerID = playerList->get(i);
        const char* cFlag = bz_getPlayerFlag(playerID);
        std::string teamFlag = (cFlag == NULL) ? "" : cFlag;

        if (bz_getPlayerTeam(playerID) == team && teamFlag == teamToTeamFlag(enemy))
        {
            doesHaveEnemyFlag = true;
            break;
        }
    }

    bz_deleteIntList(playerList);

    return doesHaveEnemyFlag;
}

bool AllHandsOnDeck::isInsideAhodZone (int playerID)
{
    bz_BasePlayerRecord* pr = bz_getPlayerByIndex(playerID);
    bool isInside = false;

    if (pr && ahodZone.pointInZone(pr->lastKnownState.pos) && pr->spawned && !pr->lastKnownState.falling)
    {
        isInside = true;
    }

    bz_freePlayerRecord(pr);

    return isInside;
}

bool AllHandsOnDeck::isEntireTeamOnBase (bz_eTeamType team)
{
    if (bz_getTeamCount(teamTwo) < 2)
    {
        return false;
    }

    bool entireTeamOnBase = true;

    bz_APIIntList *playerList = bz_newIntList();
    bz_getPlayerIndexList(playerList);

    for (unsigned int i = 0; i < playerList->size(); i++)
    {
        int playerID = playerList->get(i);

        if (bz_getPlayerTeam(playerID) == team && !isInsideAhodZone(playerID))
        {
            entireTeamOnBase = false;
            break;
        }
    }

    bz_deleteIntList(playerList);

    return entireTeamOnBase;
}
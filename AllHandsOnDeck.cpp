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

#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "bzfsAPI.h"

// Define plugin name
const std::string PLUGIN_NAME = "All Hands On Deck!";

// Define plugin version numbering
const int MAJOR = 1;
const int MINOR = 0;
const int REV = 0;
const int BUILD = 1;

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

static void fileToVector (const char* filePath, std::vector<std::string> &storage)
{
    std::ifstream file(filePath);
    std::string str;

    while (std::getline(file, str))
    {
        if (str.empty())
        {
            str = " ";
        }

        storage.push_back(str);
    }
}

class AhodZone : public bz_CustomZoneObject
{
public:
    AhodZone() : bz_CustomZoneObject(),
        defined(false) {}

    bool defined;
};

class AllHandsOnDeck : public bz_Plugin, bz_CustomMapObjectHandler
{
public:
    virtual const char* Name ();
    virtual void Init (const char* config);
    virtual void Event (bz_EventData *eventData);
    virtual void Cleanup (void);
    virtual bool MapObject (bz_ApiString object, bz_CustomMapObjectInfo *data);

    virtual bool isInsideAhodZone (int playerID);
    virtual bool isEntireTeamOnBase (bz_eTeamType team);
    virtual bool doesTeamHaveEnemyFlag (bz_eTeamType team, bz_eTeamType enemy);

    AhodZone ahodZone;

    bz_eTeamType teamOne, teamTwo;

    bool enabled;
    std::vector<std::string> introMessage;
};

BZ_PLUGIN(AllHandsOnDeck)

const char* AllHandsOnDeck::Name (void)
{
    static std::string pluginBuild = "";

    if (!pluginBuild.size())
    {
        std::ostringstream pluginBuildStream;

        pluginBuildStream << PLUGIN_NAME << " " << MAJOR << "." << MINOR << "." << REV << " (" << BUILD << ")";
        pluginBuild = pluginBuildStream.str();
    }

    return pluginBuild.c_str();
}

void AllHandsOnDeck::Init (const char* commandLine)
{
    bz_registerCustomMapObject("AHOD", this);

    enabled = false;
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

    if (commandLine)
    {
        fileToVector(commandLine, introMessage);
    }

    Register(bz_eAllowCTFCaptureEvent);
    Register(bz_ePlayerJoinEvent);
    Register(bz_ePlayerPausedEvent);
    Register(bz_eTickEvent);
    Register(bz_eWorldFinalized);
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
    ahodZone.defined = true;

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

            if (introMessage.empty())
            {
                bz_sendTextMessage(playerID, playerID, "********************************************************************");
                bz_sendTextMessage(playerID, playerID, " ");
                bz_sendTextMessage(playerID, playerID, "  --- How To Play ---");
                bz_sendTextMessage(playerID, playerID, "     Take the enemy flag to the neutral base along with your entire");
                bz_sendTextMessage(playerID, playerID, "     team in order to cap. If any player is missing, you will not be");
                bz_sendTextMessage(playerID, playerID, "     able to cap. Teamwork matters!");
                bz_sendTextMessage(playerID, playerID, " ");
                bz_sendTextMessage(playerID, playerID, "********************************************************************");
            }
            else
            {
                for (auto line : introMessage)
                {
                    bz_sendTextMessage(playerID, playerID, line.c_str());
                }
            }
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
                if (enabled)
                {
                    bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "All Hands on Deck! has been disabled. Minimum of 2v2 is required.");
                    enabled = false;
                }

                return;
            }

            if (!enabled)
            {
                bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "All Hands on Deck! has been enabled.");
                enabled = true;
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

        case bz_eWorldFinalized:
        {
            if (!ahodZone.defined)
            {
               bz_debugMessage(0, "DEBUG :: AllHandsOnDeck :: There was no AHOD zone defined for this AHOD style map.");
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
    std::unique_ptr<bz_BasePlayerRecord> pr(bz_getPlayerByIndex(playerID));

    if (pr && ahodZone.pointInZone(pr->lastKnownState.pos) && pr->spawned && !pr->lastKnownState.falling)
    {
        return true;
    }

    return false;
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
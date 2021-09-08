/*
Copyright (C) 2015-2017 Vladimir "allejo" Jimenez

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <algorithm>
#include <cmath>
#include <map>

#include "bzfsAPI.h"
#include "plugin_files.h"
#include "plugin_utils.h"

// Level used for debug messages
const int DEBUG_VERBOSITY = 4;

// Define plugin name
const char* PLUGIN_NAME = "All Hands On Deck!";

// Define plugin version numbering
const int MAJOR = 1;
const int MINOR = 1;
const int REV = 2;
const int BUILD = 52;

enum class AhodGameMode
{
    Undefined = -1,
    SingleDeck = 0, // Game mode where all teams need to go a neutral Deck in order to capture; requires a single "DECK" map object
    MultipleDecks   // Game mode where teams must have the entire team + enemy flag present on their respective Deck to capture; requires multiple "DECK" map objects
};

class DeckObject : public bz_CustomZoneObject
{
public:
    DeckObject() : bz_CustomZoneObject(),
        defined(false),
        team(eNoTeam)
    {}

    bool defined;
    bz_eTeamType team;
};

class AllHandsOnDeck : public bz_Plugin, bz_CustomMapObjectHandler
{
public:
    virtual const char* Name ();
    virtual void Init(const char* config);
    virtual void Event(bz_EventData *eventData);
    virtual void Cleanup(void);
    virtual bool MapObject(bz_ApiString object, bz_CustomMapObjectInfo *data);

private:
    bool isPlayerOnDeck(int playerID);
    bool enoughHandsOnDeck(bz_eTeamType team, int *flagCarrier, bz_eTeamType *teamCapping);
    DeckObject& getTargetDeck(int playerID);
    double getAhodPercentage();
    void sendWelcomeMessage(int playerID);

    // Plug-in configuration
    void handleCommandLine(const char* commandLine);
    void configureGameMode(const std::string &gameModeLiteral);
    void configureWelcomeMessage(const std::string &filepath);

    // Game status information
    bool enabled;
    AhodGameMode gameMode;

    // SingleDeck Mode data
    DeckObject singleDeck;

    // MultipleDecks Mode data
    std::map<bz_eTeamType, bool> teamHasCapped;
    std::map<bz_eTeamType, DeckObject> teamDecks;

    // Miscellaneous data
    bool introDisabled;
    std::vector<bz_eTeamType> availableTeams;
    std::vector<std::string> introMessage;
};

BZ_PLUGIN(AllHandsOnDeck)

const char* AllHandsOnDeck::Name(void)
{
    static const char* pluginBuild;

    if (!pluginBuild)
    {
        pluginBuild = bz_format("%s %d.%d.%d (%d)", PLUGIN_NAME, MAJOR, MINOR, REV, BUILD);
    }

    return pluginBuild;
}

void AllHandsOnDeck::Init(const char* commandLine)
{
    bz_registerCustomMapObject("AHOD", this);
    bz_registerCustomMapObject("DECK", this);

    enabled = false;
    introDisabled = false;
    gameMode = AhodGameMode::Undefined;

    for (bz_eTeamType t = eRedTeam; t <= ePurpleTeam; t = (bz_eTeamType) (t + 1))
    {
        if (bz_getTeamPlayerLimit(t) > 0)
        {
            teamHasCapped[t] = false;
            availableTeams.push_back(t);
        }
    }

    handleCommandLine(commandLine);

    Register(bz_eAllowCTFCaptureEvent);
    Register(bz_ePlayerJoinEvent);
    Register(bz_ePlayerPausedEvent);
    Register(bz_eTickEvent);
    Register(bz_eWorldFinalized);

    // Default to 100% of the team
    if (!bz_BZDBItemExists("_ahodPercentage"))
    {
        bz_setBZDBDouble("_ahodPercentage", 1);
    }

    bz_setDefaultBZDBDouble("_ahodPercentage", 1);
}

void AllHandsOnDeck::Cleanup(void)
{
    Flush();

    bz_removeCustomMapObject("AHOD");
    bz_removeCustomMapObject("DECK");
}

bool AllHandsOnDeck::MapObject(bz_ApiString object, bz_CustomMapObjectInfo *data)
{
    if ((object != "AHOD" && object != "DECK") || !data)
    {
        return false;
    }

    if (object == "AHOD")
    {
        bz_debugMessagef(0, "WARNING :: %s :: The 'AHOD' object has been renamed to 'DECK'.", PLUGIN_NAME);
        bz_debugMessagef(0, "WARNING :: %s :: Future versions of this plug-in will drop support for 'AHOD' objects.", PLUGIN_NAME);
    }

    if (gameMode == AhodGameMode::SingleDeck)
    {
        if (singleDeck.defined)
        {
            bz_debugMessagef(0, "WARNING :: %s :: A single deck has already been defined, ignoring any other decks...", PLUGIN_NAME);
            return true;
        }

        singleDeck.handleDefaultOptions(data);
        singleDeck.defined = true;
    }
    else if (gameMode == AhodGameMode::MultipleDecks)
    {
        DeckObject teamDeck;
        teamDeck.handleDefaultOptions(data);
        teamDeck.defined = true;

        for (unsigned int i = 0; i < data->data.size(); i++)
        {
            std::string line = data->data.get(i);

            bz_APIStringList nubs;
            nubs.tokenize(line.c_str(), " ", 0, true);

            if (nubs.size() > 0)
            {
                std::string key = bz_toupper(nubs.get(0).c_str());

                if (key == "COLOR")
                {
                    teamDeck.team = (bz_eTeamType)atoi(nubs.get(1).c_str());
                }
            }
        }

        if (teamDecks.count(teamDeck.team) == 0)
        {
            teamDecks[teamDeck.team] = teamDeck;
            bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: %s :: Registered deck for %s team", PLUGIN_NAME, bzu_GetTeamName(teamDeck.team));
        }
        else
        {
            bz_debugMessagef(0, "ERROR :: %s :: Multiple bases for %s team detected. Ignoring...", PLUGIN_NAME, bzu_GetTeamName(teamDeck.team));
        }
    }

    return true;
}

void AllHandsOnDeck::Event(bz_EventData *eventData)
{
    switch (eventData->eventType)
    {
        case bz_eAllowCTFCaptureEvent: // This event is called each time a flag is about to be captured
        {
            bz_AllowCTFCaptureEventData_V1* allowCtfData = (bz_AllowCTFCaptureEventData_V1*)eventData;
            bz_eTeamType teamCapping = allowCtfData->teamCapping;

            allowCtfData->allow = teamHasCapped[teamCapping];

            teamHasCapped[teamCapping] = false;
        }
        break;

        case bz_ePlayerJoinEvent: // This event is called each time a player joins the game
        {
            bz_PlayerJoinPartEventData_V1* joinData = (bz_PlayerJoinPartEventData_V1*)eventData;
            int playerID = joinData->playerID;

            sendWelcomeMessage(playerID);

            if (!enabled)
            {
                bz_sendTextMessage(BZ_SERVER, playerID, "All Hands on Deck! has been disabled. Minimum of 2v2 is required.");
            }
        }
        break;

        case bz_ePlayerPausedEvent: // This event is called each time a playing tank is paused
        {
            bz_PlayerPausedEventData_V1* pauseData = (bz_PlayerPausedEventData_V1*)eventData;

            if (isPlayerOnDeck(pauseData->playerID) && pauseData->pause)
            {
                bz_killPlayer(pauseData->playerID, false);
                bz_sendTextMessage(BZ_SERVER, pauseData->playerID, "Pausing on the deck is not permitted.");
            }
        }
        break;

        case bz_eTickEvent: // This event is called once for each BZFS main loop
        {
            // AHOD is only enabled if there are at least 2 players per team
            for (auto team : availableTeams)
            {
                if (bz_getTeamCount(team) < 2)
                {
                    if (enabled)
                    {
                        bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "All Hands on Deck! has been disabled. Minimum of 2 players per team is required.");
                        enabled = false;
                    }

                    return;
                }
            }

            if (!enabled)
            {
                bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "All Hands on Deck! has been enabled.");
                enabled = true;
            }

            for (auto team : availableTeams)
            {
                int flagCarrier;
                bz_eTeamType teamBeingCapped;

                if (enoughHandsOnDeck(team, &flagCarrier, &teamBeingCapped))
                {
                    teamHasCapped[team] = true;
                    bz_triggerFlagCapture(flagCarrier, team, teamBeingCapped);
                }
            }
        }
        break;

        case bz_eWorldFinalized:
        {
            if (gameMode == AhodGameMode::SingleDeck)
            {
                if (!singleDeck.defined)
                {
                    bz_debugMessagef(0, "ERROR :: %s :: There was no AHOD zone defined for this AHOD style map.", PLUGIN_NAME);
                }
            }
            else if (gameMode == AhodGameMode::MultipleDecks)
            {
                if (teamDecks.size() < 2)
                {
                    bz_debugMessagef(0, "WARNING :: %s :: There were not enough bases found on this map for a MultipleDecks game mode.", PLUGIN_NAME);
                }
            }
        }
        break;

        default:
            break;
    }
}

void AllHandsOnDeck::handleCommandLine(const char* commandLine)
{
    if (!commandLine)
    {
        return;
    }

    bz_APIStringList cmdLineOptions;
    cmdLineOptions.tokenize(commandLine, ",");

    if (cmdLineOptions.size() != 1 && cmdLineOptions.size() != 2)
    {
        bz_debugMessagef(0, "ERROR :: %s :: Syntax usage:", PLUGIN_NAME);
        bz_debugMessagef(0, "ERROR :: %s ::   -loadplugin AllHandsOnDeck,<SingleDeck|MultipleDecks>", PLUGIN_NAME);
        bz_debugMessagef(0, "ERROR :: %s ::   -loadplugin AllHandsOnDeck,<SingleDeck|MultipleDecks>,<default|disabled|filepath>", PLUGIN_NAME);

        return;
    }

    configureGameMode(cmdLineOptions[0]);

    if (cmdLineOptions.size() == 2)
    {
        configureWelcomeMessage(cmdLineOptions[1]);
    }
}

void AllHandsOnDeck::configureGameMode(const std::string &gameModeLiteral)
{
    if (gameModeLiteral == "SingleDeck")
    {
        gameMode = AhodGameMode::SingleDeck;

        bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: %s :: Game mode set to 'SingleDeck'", PLUGIN_NAME);
    }
    else if (gameModeLiteral == "MultipleDecks")
    {
        gameMode = AhodGameMode::MultipleDecks;

        bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: %s :: Game mode set to 'MultipleDecks'", PLUGIN_NAME);
    }

    if (gameMode == AhodGameMode::Undefined)
    {
        bz_debugMessagef(0, "ERROR :: %s :: Ahod game mode is undefined! This plug-in will not work correctly.", PLUGIN_NAME);
    }
}

void AllHandsOnDeck::configureWelcomeMessage(const std::string &filepath)
{
    if (filepath == "disabled")
    {
        introDisabled = true;

        bz_debugMessagef(DEBUG_VERBOSITY, "DEBUG :: %s :: Welcome message has been disabled.", PLUGIN_NAME);

        return;
    }
    else if (!filepath.empty())
    {
        introMessage = getFileTextLines(filepath);
    }
}

void AllHandsOnDeck::sendWelcomeMessage(int playerID)
{
    if (introDisabled)
    {
        return;
    }

    // Don't have a player send themselves this message if they don't have the "talk" permission. Otherwise, this will
    // spam with "You can't talk!" messages
    if (!bz_hasPerm(playerID, "talk"))
    {
        return;
    }

    if (!introMessage.empty())
    {
        for (auto line : introMessage)
        {
            bz_sendTextMessage(playerID, playerID, line.c_str());
        }

        return;
    }

    std::string location;

    if (gameMode == AhodGameMode::SingleDeck)
    {
        location = "the neutral base";
    }
    else if (gameMode == AhodGameMode::MultipleDecks)
    {
        location = "your own base";
    }

    bz_sendTextMessagef(playerID, playerID, "********************************************************************");
    bz_sendTextMessagef(playerID, playerID, " ");
    bz_sendTextMessagef(playerID, playerID, "  --- How To Play ---");
    bz_sendTextMessagef(playerID, playerID, "     Take the enemy flag to %s along with %.0lf%% of your", location.c_str(), (getAhodPercentage() * 100));
    bz_sendTextMessagef(playerID, playerID, "     team in order to cap. Teamwork matters!");
    bz_sendTextMessagef(playerID, playerID, " ");
    bz_sendTextMessagef(playerID, playerID, "********************************************************************");
}

double AllHandsOnDeck::getAhodPercentage()
{
    return std::min(std::abs(bz_getBZDBDouble("_ahodPercentage")), 1.0);
}

bool AllHandsOnDeck::isPlayerOnDeck(int playerID)
{
    bz_BasePlayerRecord *pr = bz_getPlayerByIndex(playerID);

    if (!pr)
    {
        return false;
    }

    DeckObject& targetDeck = getTargetDeck(playerID);
    
    bz_debugMessagef(DEBUG_VERBOSITY, "VERBOSE :: %s :: Checking player %s inside deck...", PLUGIN_NAME, pr->callsign.c_str());

    if (!pr->spawned)
    {
        bz_debugMessagef(DEBUG_VERBOSITY, "VERBOSE :: %s :: player %s is dead", PLUGIN_NAME, pr->callsign.c_str());
        return false;
    }

    if (DEBUG_VERBOSITY <= bz_getDebugLevel())
    {
        float playerPos[3] = { pr->lastKnownState.pos[0], pr->lastKnownState.pos[1], pr->lastKnownState.pos[2] };

        bz_debugMessagef(DEBUG_VERBOSITY, "VERBOSE :: %s :: player %s is at location %.2f, %.2f, %.2f", PLUGIN_NAME, pr->callsign.c_str(), playerPos[0], playerPos[1], playerPos[2]);
    }

    bool playerInsideDeckZone = (targetDeck.pointInZone(pr->lastKnownState.pos));
    bool playerIsNotJumping = !pr->lastKnownState.falling;
    
    if (!playerInsideDeckZone)
    {
        bz_debugMessagef(DEBUG_VERBOSITY, "VERBOSE :: %s :: player %s is not inside deck zone", PLUGIN_NAME, pr->callsign.c_str());
    }
    
    if (!playerIsNotJumping)
    {
        bz_debugMessagef(DEBUG_VERBOSITY, "VERBOSE :: %s :: player %s is jumping inside the deck", PLUGIN_NAME, pr->callsign.c_str());
    }
    
    bz_freePlayerRecord(pr);
    
    return (playerInsideDeckZone && playerIsNotJumping);
}

DeckObject& AllHandsOnDeck::getTargetDeck(int playerID)
{
    if (gameMode == AhodGameMode::MultipleDecks)
    {
        return teamDecks[bz_getPlayerTeam(playerID)];
    }

    return singleDeck;
}

// Check to see if there are enough players of a specified team on a deck
bool AllHandsOnDeck::enoughHandsOnDeck(bz_eTeamType team, int *flagCarrier, bz_eTeamType *teamCapping)
{
    int teamCount = 0, teamTotal = bz_getTeamCount(team);

    if (teamTotal < 2)
    {
        return false;
    }

    *flagCarrier = -1;
    *teamCapping = eNoTeam;

    bz_APIIntList *playerList = bz_newIntList();
    bz_getPlayerIndexList(playerList);

    for (unsigned int i = 0; i < playerList->size(); i++)
    {
        int playerID = playerList->get(i);

        // Ignore them if they don't belong to the team we're checking
        if (bz_getPlayerTeam(playerID) != team || !isPlayerOnDeck(playerID))
        {
            continue;
        }

        teamCount++;

        // Don't override the current flag carrier if the team has multiple enemy flags
        if (*flagCarrier == -1)
        {
            bz_eTeamType flag = bzu_getTeamFromFlag(bz_getPlayerFlag(playerID));

            // They're holding a team flag and it's not their own team flag
            if (flag != eNoTeam && flag != team)
            {
                bz_debugMessagef(DEBUG_VERBOSITY, "VERBOSE :: %s :: player %s recorded with %s team flag", PLUGIN_NAME, bz_getPlayerCallsign(playerID), bzu_GetTeamName(flag));

                *flagCarrier = playerID;
                *teamCapping = flag;
            }
        }
    }

    bz_deleteIntList(playerList);
    
    double currentPercentage = (teamCount / (double)teamTotal);
    double requiredPercentage = getAhodPercentage();

    return (currentPercentage >= requiredPercentage) && (*flagCarrier != -1);
}

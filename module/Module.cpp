/*
 * AresBWAPIBot : A bot for SC:BW using BWAPI library;
 * Copyright (C) 2015 Vincent PALANCHER; Florian LEMEASLE
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "Module.h"
#include "BroodwarLogger.h"
#include "core/log/Log.h"
#include <BWAPI/Game.h>
#include <BWAPI/Player.h>
#include <BWAPI/Unitset.h>
#include <iostream>

namespace ares
{
namespace module
{

Module::Module()
{
    core::log::Facade::initializeAuxiliaryLogger(core::log::LoggerPtr(new BroodwarLogger));
}

void Module::onStart()
{
    // We do not care about replays
    if (BWAPI::Broodwar->isReplay()) {
        return;
    }

    /* Set the command optimization level so that common commands can
       be grouped and reduce the bot's APM (Actions Per Minute). */
    BWAPI::Broodwar->setCommandOptimizationLevel(2);

#ifdef ARES_DEBUG_BUILD
            BWAPI::Broodwar->sendText("Ares version %d.%d.%d-debug", ARES_MAJOR_VERSION,
                                      ARES_MINOR_VERSION, ARES_PATCH_VERSION);
    BWAPI::Broodwar->registerEvent([](BWAPI::Game*){
        BWAPI::Broodwar->drawTextScreen(BWAPI::Positions::Origin, "%cAres v%d.%d.%d",
                                        BWAPI::Text::Red,
                                        ARES_MAJOR_VERSION, ARES_MINOR_VERSION, ARES_PATCH_VERSION);
    });
#endif

    ARES_DEBUG() << "AresBWAPIBot version " << ARES_MAJOR_VERSION << "." << ARES_MINOR_VERSION
                 << "." << ARES_PATCH_VERSION;
    ARES_DEBUG() << "New game started on map \"" << BWAPI::Broodwar->mapName() << "\"";
    ARES_DEBUG() << "Matchup is Ares (" << BWAPI::Broodwar->self()->getRace()
                 << ") vs Enemy (" << BWAPI::Broodwar->enemy()->getRace() << ")";
}

void Module::onEnd(bool isWinner)
{
    ARES_DEBUG() << "Game is finished, Ares " << (isWinner ? "won" : "lost") << " the game";
}

void Module::onSaveGame(std::string gameName)
{
    ARES_DEBUG() << "Game was saved as " << gameName;
}

namespace
{
BWAPI::Unitset filterSelfUnits(BWAPI::UnitFilter filter)
{
    BWAPI::Unitset filteredUnitSet;
    auto units = BWAPI::Broodwar->self()->getUnits();
    for (auto&& unit : units)
    {
        if (unit && filter(unit))
        {
            filteredUnitSet.insert(unit);
        }
    }
    return filteredUnitSet;
}

bool unitIsActive(BWAPI::Unit unit)
{
    return (BWAPI::Filter::Exists && !BWAPI::Filter::IsLockedDown &&
             !BWAPI::Filter::IsMaelstrommed && !BWAPI::Filter::IsStasised &&
             !BWAPI::Filter::IsLoaded && BWAPI::Filter::IsPowered &&
             !BWAPI::Filter::IsStuck && BWAPI::Filter::IsCompleted &&
             !BWAPI::Filter::IsConstructing)(unit);
}

bool unitIsIdleWorker(BWAPI::Unit unit)
{

    return (BWAPI::PtrUnitFilter(&unitIsActive) && BWAPI::Filter::IsWorker && BWAPI::Filter::IsIdle)(unit);
}

bool unitIsResourceDepot(BWAPI::Unit unit)
{
    return (BWAPI::PtrUnitFilter(&unitIsActive) && BWAPI::Filter::IsResourceDepot)(unit);
}

}

void Module::onFrame()
{
    // Return if the game is a replay or is paused
    // Also prevent spamming by only running our onFrame once every number of latency frames.
    // Latency frames are the number of frames before commands are processed.
    if (BWAPI::Broodwar->isReplay() || BWAPI::Broodwar->isPaused() || !BWAPI::Broodwar->self()
            || BWAPI::Broodwar->getFrameCount() % BWAPI::Broodwar->getLatencyFrames() != 0)
    {
        return;
    }

    sendIdleWorkersToMinerals();
    buildWorkers();
}

void Module::onSendText(std::string text)
{
    BWAPI::Broodwar->sendText("%s", text.c_str());
}

void Module::onReceiveText(BWAPI::Player player, std::string text)
{
    const std::string playerName = (player == BWAPI::Broodwar->self() ?
                                        "Ares" : player->getName());
    const std::string message = playerName + " said \"" + text + "\"";
    ARES_INFO() << message;
    BWAPI::Broodwar << message << std::endl;
}

void Module::onPlayerLeft(BWAPI::Player player)
{
    const std::string message = player->getName() + " has left the game";
    ARES_INFO() << message;
    BWAPI::Broodwar << message << std::endl;
}

void Module::onNukeDetect(BWAPI::Position target)
{
    std::ostringstream oss;
    oss << "Nuclear launch detected";
    if (target) {
        oss << " at " << target;
    }
    ARES_DEBUG() << oss.str();
}

void Module::onUnitCreate(BWAPI::Unit unit)
{
    // We do not care about replays
    if (BWAPI::Broodwar->isReplay()) {
        return;
    }
    BWAPI::Player player = unit->getPlayer();
    const std::string playerName = (player == BWAPI::Broodwar->self() ?
                                        "Ares" : player->getName());
    ARES_DEBUG() << playerName << " created unit of type " << unit->getType();
}

void Module::onUnitMorph(BWAPI::Unit unit)
{
    // We do not care about replays
    if (BWAPI::Broodwar->isReplay()) {
        return;
    }
    BWAPI::Player player = unit->getPlayer();
    const std::string playerName = (player == BWAPI::Broodwar->self() ?
                                        "Ares" : player->getName());
    ARES_DEBUG() << playerName << " morphed unit of type " << unit->getType();
}

void Module::sendIdleWorkersToMinerals()
{
    // send idle workers to mineral fields
    for (auto&& unit : BWAPI::Broodwar->self()->getUnits())
    {
        if (unitIsIdleWorker(unit))
        {
            unit->gather(unit->getClosestUnit(BWAPI::Filter::IsMineralField));
        }
    }
}

void Module::buildWorkers()
{
    for (auto&& resourceDepot : filterSelfUnits(&unitIsResourceDepot))
    {
        // Order the depot to construct more workers! But only when it is idle.
        if (resourceDepot->isIdle() && !resourceDepot->train(resourceDepot->getType().getRace().getWorker()))
        {
            // If that fails, draw the error at the location so that you can visibly see what went wrong!
            // However, drawing the error once will only appear for a single frame
            // so create an event that keeps it on the screen for some frames
            BWAPI::Position pos = resourceDepot->getPosition();
            BWAPI::Error lastErr = BWAPI::Broodwar->getLastError();
            BWAPI::Broodwar->registerEvent(
                        [pos,lastErr](BWAPI::Game*){
                            BWAPI::Broodwar->drawTextMap(pos, "%c%s", BWAPI::Text::White, lastErr.c_str());
                        }, // action
                        nullptr, // condition
                        BWAPI::Broodwar->getLatencyFrames());  // frames to run

            // Retrieve the supply provider type in the case that we have run out of supplies
            BWAPI::UnitType supplyProviderType = resourceDepot->getType().getRace().getSupplyProvider();
            static int lastChecked = 0;

            // If we are supply blocked and haven't tried constructing more recently
            if (lastErr == BWAPI::Errors::Insufficient_Supply &&
                    lastChecked + 400 < BWAPI::Broodwar->getFrameCount() &&
                    BWAPI::Broodwar->self()->incompleteUnitCount(supplyProviderType) == 0)
            {
                lastChecked = BWAPI::Broodwar->getFrameCount();

                // Retrieve a unit that is capable of constructing the supply needed
                BWAPI::Unit supplyBuilder = resourceDepot->getClosestUnit(
                            BWAPI::Filter::GetType == supplyProviderType.whatBuilds().first &&
                            (BWAPI::Filter::IsIdle || BWAPI::Filter::IsGatheringMinerals)
                            && BWAPI::Filter::IsOwned);
                // If a unit was found
                if (supplyBuilder)
                {
                    if (supplyProviderType.isBuilding())
                    {
                        BWAPI::TilePosition targetBuildLocation = BWAPI::Broodwar->getBuildLocation(supplyProviderType,
                                                                                                    supplyBuilder->getTilePosition());
                        if (targetBuildLocation)
                        {
                            // Register an event that draws the target build location
                            BWAPI::Broodwar->registerEvent([targetBuildLocation, supplyProviderType](BWAPI::Game*) {
                                BWAPI::Broodwar->drawBoxMap( BWAPI::Position(targetBuildLocation),
                                                             BWAPI::Position(targetBuildLocation + supplyProviderType.tileSize()),
                                                             BWAPI::Colors::Blue);
                            },
                            nullptr,  // condition
                            supplyProviderType.buildTime() + 100 );  // frames to run

                            // Order the builder to construct the supply structure
                            supplyBuilder->build(supplyProviderType, targetBuildLocation);
                        }
                    }
                    else
                    {
                        // Train the supply provider (Overlord) if the provider is not a structure
                        supplyBuilder->train(supplyProviderType);
                    }
                }
            }
        }
    }
}

}
}

/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "InstanceScript.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "blackrock_spire.h"
#include "GameObjectAI.h"
#include <memory>

BlackrockSpireCreatureId const DragonspireMobs[] = { NPC_BLACKHAND_DREADWEAVER, NPC_BLACKHAND_SUMMONER, NPC_BLACKHAND_VETERAN };
DragonspireHallRuneGameObjectId const DragonspireHallRuneGameObjectIds[] = { HALL_RUNE_1, HALL_RUNE_2, HALL_RUNE_3, HALL_RUNE_4, HALL_RUNE_5, HALL_RUNE_6, HALL_RUNE_7 };

enum EventIds
{
	EVENT_UROK_DOOMHOWL_SPAWNS_1           = 3,
	EVENT_UROK_DOOMHOWL_SPAWNS_2           = 4,
	EVENT_UROK_DOOMHOWL_SPAWNS_3           = 5,
	EVENT_UROK_DOOMHOWL_SPAWNS_4           = 6,
	EVENT_UROK_DOOMHOWL_SPAWNS_5           = 7,
	EVENT_UROK_DOOMHOWL_SPAWN_IN           = 8
};

class instance_blackrock_spire : public InstanceMapScript
{
public:
	instance_blackrock_spire() : InstanceMapScript(BRSScriptName, 229) { }

	struct instance_blackrock_spireMapScript : public InstanceScript
	{
		instance_blackrock_spireMapScript(InstanceMap* map) : InstanceScript(map)
		{
			SetHeaders(DataHeader);
			SetBossNumber(EncounterCount);

			//set the eventFlags to 0
			eventFlags = BlackrockSpireEventFlags::NONE;
		}

		void OnCreatureCreate(Creature* creature) override
		{
			//TODO: Maybe a macro or something to clean this up
			switch (creature->GetEntry())
			{
			case NPC_HIGHLORD_OMOKK:
			case NPC_SHADOW_HUNTER_VOSHGAJIN:
			case NPC_WARMASTER_VOONE:
			case NPC_MOTHER_SMOLDERWEB:
			case NPC_UROK_DOOMHOWL:
			case NPC_QUARTERMASTER_ZIGRIS:
			case NPC_GIZRUL_THE_SLAVENER:
			case NPC_HALYCON:
			case NPC_OVERLORD_WYRMTHALAK:
			case NPC_GYTH:
			case NPC_THE_BEAST:
			case NPC_GENERAL_DRAKKISATH:
			case NPC_SCARSHIELD_INFILTRATOR:
				guidMap[CreatureIdToEncounterId((BlackrockSpireCreatureId)creature->GetEntry())] = creature->GetGUID();
				break;
			case NPC_PYROGAURD_EMBERSEER:
				guidMap[DATA_PYROGAURD_EMBERSEER] = creature->GetGUID();
				if (GetBossState(DATA_PYROGAURD_EMBERSEER) == DONE)
					creature->DisappearAndDie();
				break;
			case NPC_WARCHIEF_REND_BLACKHAND:
				guidMap[DATA_WARCHIEF_REND_BLACKHAND] = creature->GetGUID();
				if (GetBossState(DATA_GYTH) == DONE)
					creature->DisappearAndDie();
				break;
			case NPC_LORD_VICTOR_NEFARIUS:
				//His guid was never used in previous implementation
				if (GetBossState(DATA_GYTH) == DONE)
					creature->DisappearAndDie();
				break;
			}
		 }

		void OnUnitDeath(Unit* unit) override
		{
			//Only check runes if the event isn't finished and has started
			if (isDragonspireRuneHallEncounterStartedButUnfinished(eventFlags))
			{
				if(Creature* creatureThatDied = unit->ToCreature())
					if(std::find(DragonspireMobs, DragonspireMobs + 3, creatureThatDied->GetCreatureData()->id) != (DragonspireMobs + 3)) //if the creature was a DragonSpire Rune Hall Creature we're interested
						for (GameObjectAI* runeAI : dragonspireRuneAI)
						{
							//Dispatch the OnUnitDeath event to the rune.
							runeAI->SetData64(BlackrockSpireInstanceEvent::DRAGONSPIRE_RUNE_HALL_CREATURE_DEATH, unit->GetGUID()); //use 64bit version to pass GUID
						}
			}
		}

		void OnGameObjectCreate(GameObject* go) override
		{
			//Objects here share keys with their entry unlike NPC/Encounters
			switch (go->GetEntry())
			{
				case GO_WHELP_SPAWNER:
					go->CastSpell(NULL, SPELL_SUMMON_ROOKERY_WHELP);
					break;
				case GO_EMBERSEER_IN:
					guidMap[go->GetEntry()] = go->GetGUID();
					if (GetBossState(DATA_DRAGONSPIRE_ROOM) == DONE)
						HandleGameObject(ObjectGuid::Empty, true, go);
					break;
				case GO_DOORS:
					guidMap[go->GetEntry()] = go->GetGUID();
					if (GetBossState(DATA_DRAGONSPIRE_ROOM) == DONE)
						HandleGameObject(ObjectGuid::Empty, true, go);
					break;
				case GO_EMBERSEER_OUT:
					guidMap[go->GetEntry()] = go->GetGUID();
					if (GetBossState(DATA_PYROGAURD_EMBERSEER) == DONE)
						HandleGameObject(ObjectGuid::Empty, true, go);
					break;
				case DragonspireHallRuneGameObjectId::HALL_RUNE_1:
				case DragonspireHallRuneGameObjectId::HALL_RUNE_2:
				case DragonspireHallRuneGameObjectId::HALL_RUNE_3:
				case DragonspireHallRuneGameObjectId::HALL_RUNE_4:
				case DragonspireHallRuneGameObjectId::HALL_RUNE_5:
				case DragonspireHallRuneGameObjectId::HALL_RUNE_6:
				case DragonspireHallRuneGameObjectId::HALL_RUNE_7:
					guidMap[go->GetEntry()] = go->GetGUID();
					//if it's a hall rune let's cache the rune AI
					if (GameObjectAI* ai = go->AI())
						dragonspireRuneAI.push_back(ai);
						break;
				case GO_EMBERSEER_RUNE_1:
				case GO_EMBERSEER_RUNE_2:
				case GO_EMBERSEER_RUNE_3:
				case GO_EMBERSEER_RUNE_4:
				case GO_EMBERSEER_RUNE_5:
				case GO_EMBERSEER_RUNE_6:
				case GO_EMBERSEER_RUNE_7:
					guidMap[go->GetEntry()] = go->GetGUID();
					HandleGameObject(ObjectGuid::Empty, true, go);
					go->SetGoState(GOState::GO_STATE_READY); //should be disabled until Embeerseer encounter starts
					break;
				case GO_PORTCULLIS_ACTIVE:
					guidMap[go->GetEntry()] = go->GetGUID();
					if (GetBossState(DATA_GYTH) == DONE)
						HandleGameObject(ObjectGuid::Empty, true, go);
					break;
				case GO_PORTCULLIS_TOBOSSROOMS:
					guidMap[go->GetEntry()] = go->GetGUID();
					if (GetBossState(DATA_GYTH) == DONE)
						HandleGameObject(ObjectGuid::Empty, true, go);
					break;
				default:
					break;
			}
		}

		bool SetBossState(uint32 type, EncounterState state) override
		{
			if (!InstanceScript::SetBossState(type, state))
			{
				
				if (isDragonspireRuneHallEncounterStartedButUnfinished(eventFlags))
				{
					bool isEventFinished = true;

					//Check if the rune event is started or unended; if it is we should check if it ended.
					for (DragonspireHallRuneGameObjectId id : DragonspireHallRuneGameObjectIds)
						isEventFinished = isEventFinished && (GetBossState(RuneGameObjectEntryToEncounterId(id)) == DONE);

					if (isEventFinished)
					{
						//CRITICAL to set flags first
						eventFlags = (BlackrockSpireEventFlags)(eventFlags | BlackrockSpireEventFlags::DRAGONSPIRE_RUNE_HALL_COMPLETED);

						SetBossState(BlackrockSpireEncounterId::DATA_DRAGONSPIRE_ROOM, DONE);

						//schedule finished event
						Events.ScheduleEvent(BlackrockSpireInstanceEvent::DRAGONSPIRE_RUNE_HALL_FINISHED, Seconds(1));
					}	
				}

				return false;
			}
				

			 return true;
		}

		void ProcessEvent(WorldObject* /*obj*/, uint32 eventId) override
		{
			switch (eventId)
			{
				case EVENT_PYROGUARD_EMBERSEER:
					if (GetBossState(DATA_PYROGAURD_EMBERSEER) == NOT_STARTED)
					{
						if (Creature* Emberseer = instance->GetCreature(guidMap[DATA_PYROGAURD_EMBERSEER]))
							Emberseer->AI()->SetData(1, 1);
					}
					break;
				case EVENT_UROK_DOOMHOWL:
					if (GetBossState(NPC_UROK_DOOMHOWL) == NOT_STARTED)
					{

					}
					break;
				default:
					break;
			}
		}

		void SetData(uint32 type, uint32 data) override
		{
			switch (type)
			{
				case AREATRIGGER:
					//Called when players enters the dragonspire hall trigger
					if (data == AREATRIGGER_DRAGONSPIRE_HALL)
					{
						//If the room is cleared we don't need to do anything
						if (GetBossState(DATA_DRAGONSPIRE_ROOM) != DONE && (eventFlags & BlackrockSpireEventFlags::DRAGONSPIRE_RUNE_HALL_STARTED) == 0) //second condition prevents uneeded events from being schedules just because someone continues to trigger
						{
							
							eventFlags = (BlackrockSpireEventFlags)(eventFlags | BlackrockSpireEventFlags::DRAGONSPIRE_RUNE_HALL_STARTED);

							//First indicate that the event has started
							//Event handler will handle the actual event.
							Events.ScheduleEvent(BlackrockSpireInstanceEvent::DRAGONSPIRE_RUNE_HALL_START, Seconds(1)); //schedule the start of the event
						}			
					}
				default:
					break;
			}
		}

		ObjectGuid GetGuidData(uint32 type) const override
		{
			return guidMap.find(type) != guidMap.end() ? guidMap.at(type) : ObjectGuid::Empty;
		}

		void Update(uint32 diff) override
		{
			Events.Update(diff);

			while (BlackrockSpireInstanceEvent eventId = (BlackrockSpireInstanceEvent)Events.ExecuteEvent())
			{
				switch (eventId)
				{
				case BlackrockSpireInstanceEvent::DRAGONSPIRE_RUNE_HALL_START:
					//Alert the runes that the rune hall event has started
					for (GameObjectAI* runeAI : dragonspireRuneAI)
					{
						//Dispatch the OnUnitDeath event to the rune.
						runeAI->EventInform(BlackrockSpireInstanceEvent::DRAGONSPIRE_RUNE_HALL_START); //use 64bit version to pass GUID
					}
					break;
				case BlackrockSpireInstanceEvent::DRAGONSPIRE_RUNE_HALL_FINISHED:
					//Open the doors
					HandleGameObject(guidMap[GameObjectsIds::GO_EMBERSEER_IN], true);
					HandleGameObject(guidMap[GameObjectsIds::GO_DOORS], true);
				default:
					break;
				}
			}
		}

		protected:
			EventMap Events;
			BlackrockSpireEventFlags eventFlags;
			std::vector<GameObjectAI*> dragonspireRuneAI; //we don't manage the AI instance so no need to use shared or unique ptr
			std::map<uint32, ObjectGuid> guidMap;
			ObjectGuid go_emberseerrunes[7];

		private:

			//quick way to try to grab the door encounter AI
			GameObjectAI* TryLoadEmberseerDoorEncounterAI()
			{
				if (GameObject* emberDoorGO = instance->GetGameObject(guidMap[GO_EMBERSEER_IN]))
					return emberDoorGO->AI();
				
				return nullptr;
			}

	};

	InstanceScript* GetInstanceScript(InstanceMap* map) const override
	{
		return new instance_blackrock_spireMapScript(map);
	}
};

/*#####
# at_dragonspire_hall
#####*/

class at_dragonspire_hall : public AreaTriggerScript
{
public:
	at_dragonspire_hall() : AreaTriggerScript("at_dragonspire_hall") { }

	bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/) override
	{
		if (player && player->IsAlive())
		{
			if (InstanceScript* instance = player->GetInstanceScript())
			{
				instance->SetData(AREATRIGGER, AREATRIGGER_DRAGONSPIRE_HALL);
				return true;
			}
		}

		return false;
	}
};

/*#####
# at_blackrock_stadium
#####*/

class at_blackrock_stadium : public AreaTriggerScript
{
public:
	at_blackrock_stadium() : AreaTriggerScript("at_blackrock_stadium") { }

	bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/) override
	{
		if (player && player->IsAlive())
		{
			InstanceScript* instance = player->GetInstanceScript();
			if (!instance)
				return false;

			if (Creature* rend = player->FindNearestCreature(NPC_WARCHIEF_REND_BLACKHAND, 50.0f))
			{
				rend->AI()->SetData(AREATRIGGER, AREATRIGGER_BLACKROCK_STADIUM);
				return true;
			}
		}

		return false;
	}
};

class at_nearby_scarshield_infiltrator : public AreaTriggerScript
{
public:
	at_nearby_scarshield_infiltrator() : AreaTriggerScript("at_nearby_scarshield_infiltrator") { }

	bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/) override
	{
		if (player->IsAlive())
			if (InstanceScript* instance = player->GetInstanceScript())
				if (Creature* infiltrator = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_SCARSHIELD_INFILTRATOR)))
				{
					if (player->getLevel() >= 57)
						infiltrator->AI()->SetData(1, 1);
					else if (infiltrator->GetEntry() == NPC_SCARSHIELD_INFILTRATOR)
						infiltrator->AI()->Talk(0, player);

					return true;
				}

		return false;
	}
};

class go_dragonspire_hall_room_rune : public GameObjectScript
{
public:
	//We shouldn't activate the rune on spawn.
	//Players may not even enter UBRS so it would be a waste.
	go_dragonspire_hall_room_rune() : GameObjectScript("go_dragonspire_hall_room_rune") { }

	struct go_dragonspire_hall_room_runeAI : GameObjectAI
	{
	public:
		go_dragonspire_hall_room_runeAI(GameObject* gameObject) : GameObjectAI(gameObject)
		{ 

		}

		void InitializeAI() override
		{ 
			GameObjectAI::InitializeAI();
		}

		void Reset() override
		{
			GameObjectAI::Reset();

			bool shouldSetDeactivated = go->GetInstanceScript()->GetBossState(RuneGameObjectEntryToEncounterId((DragonspireHallRuneGameObjectId)go->GetEntry())) == DONE;

			go->GetInstanceScript()->HandleGameObject(go->GetGUID(), shouldSetDeactivated);
		}

		void EventInform(uint32 eventId) override
		{ 
			switch ((BlackrockSpireInstanceEvent)eventId)
			{
				case BlackrockSpireInstanceEvent::DRAGONSPIRE_RUNE_HALL_START: //the hall event has started so we need to initialize the rune AI
				{
					trackedCreatureGuids.reserve(6); //save some time preallocating

					std::list<Creature*> creatureList;

					//For each creature that is suppose to protect the rune we look for it in a 15y radius.
					for (BlackrockSpireCreatureId creatureId : DragonspireMobs)
						GetCreatureListWithEntryInGrid(creatureList, go, creatureId, 15.0f);

					for (Creature* creature : creatureList)
					{
						//Track GUIDs and not UNITS
						trackedCreatureGuids.push_back(creature->GetGUID());
					}
				}
				break;

				default:
					break;
			}
		}

		void SetData64(uint32 eventId, uint64 value) override //we need the 64bit version to transfer ObjectGUIDs
		{
			ObjectGuid objectGuid = (ObjectGuid)value;

			switch ((BlackrockSpireInstanceEvent)eventId)
			{
			case BlackrockSpireInstanceEvent::DRAGONSPIRE_RUNE_HALL_CREATURE_DEATH:

				//try to remove the GUID from the tracked guids
				trackedCreatureGuids.erase(std::remove(trackedCreatureGuids.begin(), trackedCreatureGuids.end(), objectGuid), trackedCreatureGuids.end());

				if (trackedCreatureGuids.size() == 0) //if it's empty the rune is no longer protected
				{
					HandleUnprotectedRune();
				}
			default:
				break;
			}
		}

	private:
		std::vector<ObjectGuid> trackedCreatureGuids; //since the amount of creatures are small we don't need a map

		void HandleUnprotectedRune()
		{
			trackedCreatureGuids.clear();

			//set the instance boss state to indicate that the rune has been deactivated and deactivate it
			go->GetInstanceScript()->HandleGameObject(go->GetGUID(), false);

			go->GetInstanceScript()->SetBossState(RuneGameObjectEntryToEncounterId((DragonspireHallRuneGameObjectId)go->GetEntry()), DONE); //we don't know the encounter ID. The instance will though
		}
	};

	GameObjectAI* GetAI(GameObject* go) const override
	{ 
		return GetInstanceAI<go_dragonspire_hall_room_runeAI>(go);
	}
};

void AddSC_instance_blackrock_spire()
{
	new instance_blackrock_spire();
	new go_dragonspire_hall_room_rune();
	new at_dragonspire_hall();
	new at_blackrock_stadium();
	new at_nearby_scarshield_infiltrator();
}

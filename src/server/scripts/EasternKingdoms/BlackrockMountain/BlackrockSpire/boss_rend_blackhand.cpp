/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "blackrock_spire.h"

enum Spells
{
    SPELL_WHIRLWIND                 = 13736, // sniffed
    SPELL_CLEAVE                    = 15284,
    SPELL_MORTAL_STRIKE             = 16856,
    SPELL_FRENZY                    = 8269,
    SPELL_KNOCKDOWN                 = 13360  // On spawn during Gyth fight
};

enum Says
{
    // Rend Blackhand
    SAY_BLACKHAND_1                 = 0,
    SAY_BLACKHAND_2                 = 1,
    EMOTE_BLACKHAND_DISMOUNT        = 2,
    // Victor Nefarius
    SAY_NEFARIUS_0                  = 0,
    SAY_NEFARIUS_1                  = 1,
    SAY_NEFARIUS_2                  = 2,
    SAY_NEFARIUS_3                  = 3,
    SAY_NEFARIUS_4                  = 4,
    SAY_NEFARIUS_5                  = 5,
    SAY_NEFARIUS_6                  = 6,
    SAY_NEFARIUS_7                  = 7,
    SAY_NEFARIUS_8                  = 8,
    SAY_NEFARIUS_9                  = 9,
};

enum Adds
{
    NPC_CHROMATIC_WHELP             = 10442,
    NPC_CHROMATIC_DRAGONSPAWN       = 10447,
    NPC_BLACKHAND_DRAGON_HANDLER    = 10742
};

enum Misc
{
    NEFARIUS_PATH_1                 = 1379670,
    NEFARIUS_PATH_2                 = 1379671,
    NEFARIUS_PATH_3                 = 1379672,
    REND_PATH_1                     = 1379680,
    REND_PATH_2                     = 1379681,
};

Position const GythLoc =      { 211.762f,  -397.5885f, 111.1817f,  4.747295f   };
Position const Teleport1Loc = { 194.2993f, -474.0814f, 121.4505f, -0.01225555f };
Position const Teleport2Loc = { 216.485f,  -434.93f,   110.888f,  -0.01225555f };

enum Events
{
    EVENT_START_1                   = 1,
    EVENT_START_2                   = 2,
    EVENT_START_3                   = 3,
    EVENT_START_4                   = 4,
    EVENT_TURN_TO_REND              = 5,
    EVENT_TURN_TO_PLAYER            = 6,
    EVENT_TURN_TO_FACING_1          = 7,
    EVENT_TURN_TO_FACING_2          = 8,
    EVENT_TURN_TO_FACING_3          = 9,
    EVENT_WAVE_1                    = 10,
    EVENT_WAVE_2                    = 11,
    EVENT_WAVE_3                    = 12,
    EVENT_WAVE_4                    = 13,
    EVENT_WAVE_5                    = 14,
    EVENT_WAVE_6                    = 15,
    EVENT_WAVE_7                    = 33,
    EVENT_WAVES_TEXT_1              = 16,
    EVENT_WAVES_TEXT_2              = 17,
    EVENT_WAVES_TEXT_3              = 18,
    EVENT_WAVES_TEXT_4              = 19,
    EVENT_WAVES_TEXT_5              = 20,
    EVENT_WAVES_COMPLETE_TEXT_1     = 21,
    EVENT_WAVES_COMPLETE_TEXT_2     = 22,
    EVENT_WAVES_COMPLETE_TEXT_3     = 23,
    EVENT_WAVES_EMOTE_1             = 24,
    EVENT_WAVES_EMOTE_2             = 25,
    EVENT_PATH_REND                 = 26,
    EVENT_PATH_NEFARIUS             = 27,
    EVENT_TELEPORT_1                = 28,
    EVENT_TELEPORT_2                = 29,
    EVENT_WHIRLWIND                 = 30,
    EVENT_CLEAVE                    = 31,
    EVENT_MORTAL_STRIKE             = 32,
};

float waveTimeMultiplier = 0.01f;

class boss_rend_blackhand : public CreatureScript
{
public:
    boss_rend_blackhand() : CreatureScript("boss_rend_blackhand") { }

    struct boss_rend_blackhandAI : public BossAI
    {
        boss_rend_blackhandAI(Creature* creature) : BossAI(creature, DATA_WARCHIEF_REND_BLACKHAND)
        {
            victorGUID.Clear();
            portcullisGUID.Clear();
        }

        void Reset() override
        {
            if (instance->GetBossState(DATA_GYTH) != EncounterState::DONE) //it's a tricky situation when Gyth is dead but Rend is alive.
            {
                isFinalWave = false;

                //Reset victor and rend's position
                if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                {
                    //reset his position too otherwise he'll be standing on the ledge
                    victor->SetPosition(victor->GetHomePosition());
                    victor->AI()->Reset();
                }

                //Open Event Door
                if (ObjectGuid door = instance->GetGuidData(GO_PORTCULLIS_ACTIVE))
                    instance->HandleGameObject(door, true);


                summons.DespawnAll();
                summons.clear();
            }
            else
            {
                GatherFightObjectGuids();

                //Reset victor and rend's position
                if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                {
                    //reset his position too otherwise he'll be standing on the ledge
                    victor->SetPosition(victor->GetHomePosition());
                }
            }

            //We need to despawn waves because a wave could spawn right before a despawn
            //This would leave the wave walking down threw the event even though the event ended   
            instance->SetBossState(DATA_WARCHIEF_REND_BLACKHAND, EncounterState::FAIL);

            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            BossAI::EnterCombat(who);

            events.ScheduleEvent(EVENT_WHIRLWIND,     urand(13000, 15000));
            events.ScheduleEvent(EVENT_CLEAVE,        urand(15000, 17000));
            events.ScheduleEvent(EVENT_MORTAL_STRIKE, urand(17000, 19000));
        }

        void IsSummonedBy(Unit* /*summoner*/) override
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            DoZoneInCombat();
            instance->SetBossState(DATA_WARCHIEF_REND_BLACKHAND, EncounterState::IN_PROGRESS);
        }

        //Called when a creature in a summon group is despawned. This will include Gyth
        void SummonedCreatureDespawn(Creature* summon) override
        {
            //Despawning if the creature is dead is fine

            //Check if they're alive during despawn. This would mean they weren't engaged and the event should be failed
            if (summon->IsAlive())
            {
                //Reset Gyth event
                instance->SetBossState(DATA_GYTH, EncounterState::FAIL);

                //If we're dead we need to respawn
                if (!me->IsAlive())
                {
                    me->Respawn();
                    me->SetPosition(me->GetHomePosition());
                }
                else
                    Reset();
            }
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            summons.Despawn(summon);

            //If this is the final wave and no summons are left alive then it's time to finish the wave event
            if (isFinalWave && summons.empty())
            {
                events.ScheduleEvent(EVENT_WAVES_COMPLETE_TEXT_1, 20000 * waveTimeMultiplier);
                isFinalWave = false;
            }
        }

        void JustSummoned(Creature* summon) override
        {

            summons.Summon(summon);

            //Don't move if it's Gyth. He's controlled differently
            if (summon->GetEntry() == NPC_GYTH)
                return;

            //Make the creature walk to the center of the room.
            Position summonPosition = summon->GetPosition();
            summon->SetWalk(true); //make them walk; UBRS reference video indicated that the creatures walk after being spawned
            summon->GetMotionMaster()->MovePoint(0, summonPosition.GetPositionX() - 80, summonPosition.GetPositionY(), summonPosition.GetPositionZ(), false);
        }


        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            if (Creature* victor = me->FindNearestCreature(NPC_LORD_VICTOR_NEFARIUS, 150.0f, true))
                victor->AI()->SetData(1, 2);
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == AREATRIGGER && data == AREATRIGGER_BLACKROCK_STADIUM) //don't restart if Gyth has died.
            {
                //Don't restart the event if Gyth is done or inprogress or if Rend has been defeated
                if (instance->GetBossState(DATA_GYTH) != EncounterState::DONE && instance->GetBossState(DATA_GYTH) != EncounterState::IN_PROGRESS
                    && instance->GetBossState(DATA_WARCHIEF_REND_BLACKHAND) != EncounterState::DONE)
                {
                    instance->SetBossState(DATA_GYTH, EncounterState::IN_PROGRESS);

                    GatherFightObjectGuids();
                    events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                    events.ScheduleEvent(EVENT_START_1, 1000);
                }
            }
        }

        void GatherFightObjectGuids()
        {
            if (Creature* victor = me->FindNearestCreature(NPC_LORD_VICTOR_NEFARIUS, 50.0f, true))
                victorGUID = victor->GetGUID();

            if (GameObject* portcullis = me->FindNearestGameObject(GO_DR_PORTCULLIS, 50.0f))
                portcullisGUID = portcullis->GetGUID();
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == WAYPOINT_MOTION_TYPE)
            {
                switch (id)
                {
                    case 5:
                        events.ScheduleEvent(EVENT_TELEPORT_1, 2000);
                        break;
                    case 11:
                        if (Creature* gyth = me->FindNearestCreature(NPC_GYTH, 10.0f, true))
                            gyth->AI()->SetData(1, 1);
                        me->DespawnOrUnsummon(1000);
                        break;
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            //While rend is not in progress then the glyh event is happening.
            if (instance->GetBossState(DATA_WARCHIEF_REND_BLACKHAND) != EncounterState::IN_PROGRESS
                && instance->GetBossState(DATA_WARCHIEF_REND_BLACKHAND) != EncounterState::DONE)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_START_1:
                            //Close Event Door
                            if (ObjectGuid door = instance->GetGuidData(GO_PORTCULLIS_ACTIVE))
                                instance->HandleGameObject(door, false);

                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_0);
                            events.ScheduleEvent(EVENT_START_2, 4000);
                            break;
                        case EVENT_START_2:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
                            events.ScheduleEvent(EVENT_START_3, 4000);
                            break;
                        case EVENT_START_3:
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_1);
                            events.ScheduleEvent(EVENT_WAVE_1, 2000);
                            events.ScheduleEvent(EVENT_TURN_TO_REND, 4000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_1, 56000 * waveTimeMultiplier);
                            break;
                        case EVENT_TURN_TO_REND:
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                            {
                                victor->SetFacingToObject(me);
                                victor->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            }
                            break;
                        case EVENT_TURN_TO_PLAYER:
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                if (Unit* player = victor->SelectNearestPlayer(60.0f))
                                    victor->SetFacingToObject(player);
                            break;
                        case EVENT_TURN_TO_FACING_1:
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->SetFacingTo(1.518436f);
                            break;
                        case EVENT_TURN_TO_FACING_2:
                            me->SetFacingTo(1.658063f);
                            break;
                        case EVENT_TURN_TO_FACING_3:
                            me->SetFacingTo(1.500983f);
                            break;
                        case EVENT_WAVES_EMOTE_1:
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                            break;
                        case EVENT_WAVES_EMOTE_2:
                                me->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
                            break;
                        case EVENT_WAVES_TEXT_1:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                    victor->AI()->Talk(SAY_NEFARIUS_2);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVES_EMOTE_1, 5000);
                            events.ScheduleEvent(EVENT_WAVE_2, 2000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_2, 30000 * waveTimeMultiplier);
                            break;
                        case EVENT_WAVES_TEXT_2:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_3);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVE_3, 2000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_3, 50000 * waveTimeMultiplier);
                            break;
                        case EVENT_WAVES_TEXT_3:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_4);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVE_4, 2000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_4, 56000 * waveTimeMultiplier);
                            break;
                        case EVENT_WAVES_TEXT_4:
                            Talk(SAY_BLACKHAND_1);
                            events.ScheduleEvent(EVENT_WAVES_EMOTE_2, 4000);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_3, 8000);
                            events.ScheduleEvent(EVENT_WAVE_5, 2000);
                            events.ScheduleEvent(EVENT_WAVES_TEXT_5, 56000 * waveTimeMultiplier);
                            break;
                        case EVENT_WAVES_TEXT_5:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_5);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVE_6, 2000);
                            //Schedule the final wave
                            events.ScheduleEvent(EVENT_WAVE_7, 45000 * waveTimeMultiplier);
                            break;
                        case EVENT_WAVES_COMPLETE_TEXT_1:
                            events.ScheduleEvent(EVENT_TURN_TO_PLAYER, 0);
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_6);
                            events.ScheduleEvent(EVENT_TURN_TO_FACING_1, 4000);
                            events.ScheduleEvent(EVENT_WAVES_COMPLETE_TEXT_2, 13000 * waveTimeMultiplier);
                            break;
                        case EVENT_WAVES_COMPLETE_TEXT_2:
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_7);
                            Talk(SAY_BLACKHAND_2);
                            events.ScheduleEvent(EVENT_PATH_REND, 1000);
                            events.ScheduleEvent(EVENT_WAVES_COMPLETE_TEXT_3, 4000 * waveTimeMultiplier);
                            break;
                        case EVENT_WAVES_COMPLETE_TEXT_3:
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->AI()->Talk(SAY_NEFARIUS_8);
                            events.ScheduleEvent(EVENT_PATH_NEFARIUS, 1000);
                            events.ScheduleEvent(EVENT_PATH_REND, 1000);
                            break;
                        case EVENT_PATH_NEFARIUS:
                            if (Creature* victor = ObjectAccessor::GetCreature(*me, victorGUID))
                                victor->GetMotionMaster()->MovePath(NEFARIUS_PATH_1, true);
                            break;
                        case EVENT_PATH_REND:
                            me->GetMotionMaster()->MovePath(REND_PATH_1, false);
                            break;
                        case EVENT_TELEPORT_1:
                            me->NearTeleportTo(194.2993f, -474.0814f, 121.4505f, -0.01225555f);
                            events.ScheduleEvent(EVENT_TELEPORT_2, 50000 * waveTimeMultiplier);
                            break;
                        case EVENT_TELEPORT_2:
                            me->NearTeleportTo(216.485f, -434.93f, 110.888f, -0.01225555f);
                            me->SummonCreature(NPC_GYTH, 211.762f, -397.5885f, 111.1817f, 4.747295f);
                            break;
                        case EVENT_WAVE_2:
                        case EVENT_WAVE_1:
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            me->SummonCreatureGroup(0);
                            break;
                        case EVENT_WAVE_5:
                        case EVENT_WAVE_3:
                        case EVENT_WAVE_4:
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            me->SummonCreatureGroup(1);
                            break;
                        case EVENT_WAVE_6:
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            me->SummonCreatureGroup(2);
                            break;
                        case EVENT_WAVE_7:
                            isFinalWave = true;
                            if (GameObject* portcullis = me->GetMap()->GetGameObject(portcullisGUID))
                                portcullis->UseDoorOrButton();
                            me->SummonCreatureGroup(3);
                            break;
                        default:
                            break;
                    }
                }
            }

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_WHIRLWIND:
                        DoCast(SPELL_WHIRLWIND);
                        events.ScheduleEvent(EVENT_WHIRLWIND, urand(13000, 18000));
                        break;
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, urand(10000, 14000));
                        break;
                    case EVENT_MORTAL_STRIKE:
                        DoCastVictim(SPELL_MORTAL_STRIKE);
                        events.ScheduleEvent(EVENT_MORTAL_STRIKE, urand(14000, 16000));
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }
            DoMeleeAttackIfReady();
        }

        private:
            bool isFinalWave;
            ObjectGuid victorGUID;
            ObjectGuid portcullisGUID;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<boss_rend_blackhandAI>(creature);
    }
};

class npc_rendevent_chromatic_handler : public CreatureScript
{
    enum HandlerSpells
    {
        SPELL_MEND_DRAGON = 16637,
    };

    enum HandlerEvent
    {
        EVENT_CHECK_HEALING = 0,
    };

public:
    npc_rendevent_chromatic_handler() : CreatureScript("npc_rendevent_chromatic_handler") { }

    struct npc_rendevent_chromatic_handlerAI : public ScriptedAI
    {
        npc_rendevent_chromatic_handlerAI(Creature* creature) : ScriptedAI(creature) { }

        //Called at creature aggro either by MoveInLOS or Attack Start
        void EnterCombat(Unit* /*victim*/) override 
        { 
            events.Reset();
            events.ScheduleEvent(EVENT_CHECK_HEALING, Seconds(1));
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            if (events.ExecuteEvent() == EVENT_CHECK_HEALING)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                {
                    //if we're not near a healable target cancel the channel
                    if (!IsNearHealableTarget(NPC_CHROMATIC_DRAGONSPAWN))
                    {
                        me->CastStop();
                    }

                    events.ScheduleEvent(EVENT_CHECK_HEALING, urand(1000, 3000)); //use random to make the action feel less robotic
                }
                else
                {
                    //If there is a dragonspawn nearby tha needs heals then we should cast
                    if (IsNearHealableTarget(NPC_CHROMATIC_DRAGONSPAWN))
                        me->CastSpell(me, SPELL_MEND_DRAGON);

                    events.ScheduleEvent(EVENT_CHECK_HEALING, urand(2000, 5000)); //use random to make the action feel less robotic
                }
                    
            }
            
            //If we're not channeling then melee
            if (!me->HasUnitState(UNIT_STATE_CASTING))
                DoMeleeAttackIfReady();
        }

    private:
        EventMap events;

        bool IsNearHealableTarget(uint32 creatureEntry)
        {
            std::vector<Creature*> creatures;
            creatures.reserve(5); //preallocate for approximately one wave.
            GetCreatureListWithEntryInGrid(creatures, me, creatureEntry, 15.0f); //should be less than actual range otherwise you get issues

            //originally we did some std::remove erasing for dead targets and then sorted for most damaged. That was costly. Now we do simple linear search

            if (creatures.size() > 0)
            {
                for (Creature* creature : creatures)
                {
                    //living and damaged only
                    if (creature->IsAlive() && creature->GetHealth() < creature->GetMaxHealth())
                        return true;
                }
            }

            return false;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_rendevent_chromatic_handlerAI(creature);
    }
};

void AddSC_boss_rend_blackhand()
{
    new boss_rend_blackhand();
    new npc_rendevent_chromatic_handler();
}

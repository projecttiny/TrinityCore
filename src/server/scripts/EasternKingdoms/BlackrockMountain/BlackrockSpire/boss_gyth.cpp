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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackrock_spire.h"

enum Spells
{
    SPELL_REND_MOUNTS               = 16167, // Change model
    SPELL_CORROSIVE_ACID            = 16359, // Combat (self cast)
    SPELL_FLAMEBREATH               = 16390, // Combat (Self cast)
    SPELL_FREEZE                    = 16350, // Combat (Self cast)
    SPELL_KNOCK_AWAY                = 10101, // Combat
    SPELL_SUMMON_REND               = 16328  // Summons Rend near death
};

enum Misc
{
    NEFARIUS_PATH_2                 = 1379671,
    NEFARIUS_PATH_3                 = 1379672,
    GYTH_PATH_1                     = 1379681,
};

enum Events
{
    EVENT_CORROSIVE_ACID            = 1,
    EVENT_FREEZE                    = 2,
    EVENT_FLAME_BREATH              = 3,
    EVENT_KNOCK_AWAY                = 4,
    EVENT_SUMMONED_1                = 5,
    EVENT_SUMMONED_2                = 6,
    EVENT_DESPAWN_ON_NO_ENEMY_FOUND = 7,
};

class boss_gyth : public CreatureScript
{
public:
    boss_gyth() : CreatureScript("boss_gyth") { }

    struct boss_gythAI : public BossAI
    {
        bool SummonedRend;
        bool isJustSpawned;

        boss_gythAI(Creature* creature) : BossAI(creature, DATA_GYTH) { }

        void InitializeAI() override
        {
            SummonedRend = false;
            isJustSpawned = false;
        }

		void Reset() override
		{
			BossAI::Reset();

			me->GetMotionMaster()->MoveTargetedHome();
		}

        void EnterEvadeMode(EvadeReason why) override
        {
            //reset the event if it's inprogress and if Gyth evaded because it couldn't find players.
            if (instance->GetBossState(DATA_GYTH) == IN_PROGRESS && why == EvadeReason::EVADE_REASON_NO_HOSTILES)
            {
                instance->SetBossState(DATA_GYTH, EncounterState::FAIL); //if a reset happens during the fight then we failed

                //Only despawn and reset if rend isn't dead
                if (instance->GetBossState(DATA_WARCHIEF_REND_BLACKHAND) != EncounterState::DONE)
                {
                    summons.DespawnAll();
                    SummonedRend = false;
                    me->DespawnOrUnsummon();
                }
            }
            else
            {
                //Don't remove aura or anything
                me->DeleteThreatList();
                me->CombatStop(true);
                me->LoadCreaturesAddon();
                me->SetLootRecipient(NULL);
                me->ResetPlayerDamageReq();
                me->SetLastDamagedTime(0);

                //heal him up
                me->SetFullHealth();
            }
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();

            //set zone combat to prevent vanishes or deaths from reseting the encounter
            me->SetInCombatWithZone();

            //Force inprogress even if we already are. There could have been a wipe
            instance->SetBossState(DATA_GYTH, EncounterState::IN_PROGRESS);

            //cancel the despawn event
            events.CancelEvent(EVENT_DESPAWN_ON_NO_ENEMY_FOUND);

            events.ScheduleEvent(EVENT_CORROSIVE_ACID, urand(8000, 16000));
            events.ScheduleEvent(EVENT_FREEZE, urand(8000, 16000));
            events.ScheduleEvent(EVENT_FLAME_BREATH, urand(8000, 16000));
            events.ScheduleEvent(EVENT_KNOCK_AWAY, urand(12000, 18000));
        }
        
        //Implement this so that we can summon Rend should we be damaged to the point of death before getting the summon off
        void DamageTaken(Unit* attacker, uint32& damage) override 
        {
			if (me->GetHealth() <= damage)
			{
				Beep(700, 1000);

				if(!SummonedRend)
					SummonRend();
				else
					if (instance->GetBossState(DATA_WARCHIEF_REND_BLACKHAND) != EncounterState::IN_PROGRESS
						&& instance->GetBossState(DATA_WARCHIEF_REND_BLACKHAND) != EncounterState::DONE) //if rend isn't in progress or done we should stall
					{
						//Basically make him immume to damage until Rend has spawned
						damage = me->GetHealth() - 1;
						return;
					}
			}
				

			BossAI::DamageTaken(attacker, damage);
        }

        void JustDied(Unit* /*killer*/) override
        {
            instance->SetBossState(DATA_GYTH, EncounterState::DONE);
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
        }

        void SetData(uint32 /*type*/, uint32 data) override
        {
            switch (data)
            {
                case 1:
                    events.ScheduleEvent(EVENT_SUMMONED_1, 1000);
                    break;
                default:
                    break;
            }
        }

		void SummonRend()
		{
			DoCast(me, SPELL_SUMMON_REND, true); //if you don't trigger than he won't summon during a cast
			me->RemoveAura(SPELL_REND_MOUNTS);
			SummonedRend = true;
		}

        void UpdateAI(uint32 diff) override
        {
            if (!SummonedRend && HealthBelowPct(25))
            {
				SummonRend();
            }

            if (!UpdateVictim())
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUMMONED_1:
                            me->AddAura(SPELL_REND_MOUNTS, me);
                            if (GameObject* portcullis = me->FindNearestGameObject(GO_DR_PORTCULLIS, 40.0f))
                                portcullis->UseDoorOrButton();
                            if (Creature* victor = me->FindNearestCreature(NPC_LORD_VICTOR_NEFARIUS, 75.0f, true))
                                victor->AI()->SetData(1, 1);
                            events.ScheduleEvent(EVENT_SUMMONED_2, 2000);
                            break;
                        case EVENT_SUMMONED_2:
                            me->GetMotionMaster()->MovePath(GYTH_PATH_1, false);

                            //prepare a despawn event if Gyth isn't 
                            events.ScheduleEvent(EVENT_DESPAWN_ON_NO_ENEMY_FOUND, Seconds(40));
                            break;
                        case EVENT_DESPAWN_ON_NO_ENEMY_FOUND:
                            this->EnterEvadeMode(EvadeReason::EVADE_REASON_NO_HOSTILES);
                            break;
                        default:
                            break;
                    }
                }
                return;
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CORROSIVE_ACID:
                        DoCast(me, SPELL_CORROSIVE_ACID);
                        events.ScheduleEvent(EVENT_CORROSIVE_ACID, urand(10000, 16000));
                        break;
                    case EVENT_FREEZE:
                        DoCast(me, SPELL_FREEZE);
                        events.ScheduleEvent(EVENT_FREEZE, urand(10000, 16000));
                        break;
                    case EVENT_FLAME_BREATH:
                        DoCast(me, SPELL_FLAMEBREATH);
                        events.ScheduleEvent(EVENT_FLAME_BREATH, urand(10000, 16000));
                        break;
                    case EVENT_KNOCK_AWAY:
                        DoCastVictim(SPELL_KNOCK_AWAY);
                        events.ScheduleEvent(EVENT_KNOCK_AWAY, urand(14000, 20000));
                        break;
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<boss_gythAI>(creature);
    }
};

void AddSC_boss_gyth()
{
    new boss_gyth();
}

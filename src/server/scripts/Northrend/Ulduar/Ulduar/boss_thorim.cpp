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
#include "ulduar.h"
#include "Player.h"

enum Yells
{
    SAY_AGGRO = 0,
    SAY_SPECIAL_1 = 1,
    SAY_SPECIAL_2 = 2,
    SAY_SPECIAL_3 = 3,
    SAY_JUMPDOWN = 4,
    SAY_SLAY = 5,
    SAY_BERSERK = 6,
    SAY_WIPE = 7,
    SAY_DEATH = 8,
    SAY_END_NORMAL_1 = 9,
    SAY_END_NORMAL_2 = 10,
    SAY_END_NORMAL_3 = 11,
    SAY_END_HARD_1 = 12,
    SAY_END_HARD_2 = 13,
    SAY_END_HARD_3 = 14
};

enum Events
{
    EVENT_TRIGGERED_TRANSITION_PHASE2 = 1,
};

enum Actions
{
    ACTION_THORIM_START_TRANSITION_PHASE2 = 1,
};

class boss_thorim : public CreatureScript
{
public:
    boss_thorim() : CreatureScript("boss_thorim") { }

    struct boss_thorimAI : public BossAI
    {
        boss_thorimAI(Creature* creature) : BossAI(creature, BOSS_THORIM)
        {
        }

        void Reset() override
        {
            _Reset();
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            Talk(SAY_WIPE);
            _EnterEvadeMode(why);
        }

        void KilledUnit(Unit* who) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            _JustDied();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            _EnterCombat();
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_TRIGGERED_TRANSITION_PHASE2:
                    me->KillSelf();
                    //Blah blah, impertenent whelps. You DARE challenge me atop my pedestal?
                    //He then should wait 5 or 6 seconds before he jumps down.
                    //Then 2 seconds after he jumps down he should start maintaining a hostile/threat list
                    //and generating threat
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
            case ACTION_THORIM_START_TRANSITION_PHASE2:
                _beep(500, 100);
                events.ScheduleEvent(EVENT_TRIGGERED_TRANSITION_PHASE2, 1);
                break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetUlduarAI<boss_thorimAI>(creature);
    }
};

// 5357 Trigger Thorim's p2 engage
class at_thorim_platform_engage : public AreaTriggerScript
{
public:
    at_thorim_platform_engage() : AreaTriggerScript("at_thorim_platform_engage") { }

    bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/) override
    {
        _beep(1000, 500);

        //If the player is alive we should find thorim and activate his p2
        if (player->IsAlive())
            if (InstanceScript* instance = player->GetInstanceScript())
                if (Creature* thorim = ObjectAccessor::GetCreature(*player, instance->GetGuidData(BOSS_THORIM)))
                {
                    if (thorim->IsAlive())
                    {
                        thorim->AI()->DoAction(ACTION_THORIM_START_TRANSITION_PHASE2);
                        return true;
                    }
                }
        return false;
    }
};

void AddSC_boss_thorim()
{
    new boss_thorim();
    new at_thorim_platform_engage();
}
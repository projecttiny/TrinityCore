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
#include "blackrock_spire.h"
#include "SpellAuras.h"
#include "SpellScript.h"

enum Spells
{
    SPELL_FIRENOVA                  = 23462,
    SPELL_CLEAVE                    = 20691,
    SPELL_CONFLIGURATION            = 16805,
    SPELL_THUNDERCLAP               = 15548, //Not sure if right ID. 23931 would be a harder possibility.
};

enum Events
{
    EVENT_FIRE_NOVA                = 1,
    EVENT_CLEAVE                   = 2,
    EVENT_CONFLIGURATION           = 3,
    EVENT_THUNDERCLAP              = 4,
    EVENT_GUARD_CALL_PULSE         = 5
};

class boss_drakkisath : public CreatureScript
{
public:
    boss_drakkisath() : CreatureScript("boss_drakkisath") { }

    struct boss_drakkisathAI : public BossAI
    {
        boss_drakkisathAI(Creature* creature) : BossAI(creature, DATA_GENERAL_DRAKKISATH) { }

        void Reset() override
        {
            _Reset();
        }

        void EnterCombat(Unit* who) override
        {
            _EnterCombat();

            ForceGuardsIntoCombat(who);

            events.ScheduleEvent(EVENT_GUARD_CALL_PULSE, 2000);
            events.ScheduleEvent(EVENT_FIRE_NOVA, 6000);
            events.ScheduleEvent(EVENT_CLEAVE,    8000);
            events.ScheduleEvent(EVENT_CONFLIGURATION, 15000);
            events.ScheduleEvent(EVENT_THUNDERCLAP,    17000);
        }

        void ForceGuardsIntoCombat(Unit* who)
        {
            //Tries to handle the only unsolved case of pulling Drakk and 1 guard. It's not 100% but it'll help prevene the last small chance way to reduce the encounter difficulty.
            std::vector<Creature*> creatureList;
            creatureList.reserve(2);

            GetCreatureListWithEntryInGrid(creatureList, me, NPC_CHROMATIC_ELITE_GUARD, 1000);

            if (creatureList.size() > 0)
                for (Creature* creature : creatureList)
                {
                    if (!creature->IsInCombat() && !creature->IsInEvadeMode())
                        creature->Attack(who, false);
                }
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_FIRE_NOVA:
                        DoCastVictim(SPELL_FIRENOVA);
                        events.ScheduleEvent(EVENT_FIRE_NOVA, 10000);
                        break;
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, 8000);
                        break;
                    case EVENT_CONFLIGURATION:
                        DoCastVictim(SPELL_CONFLIGURATION);
                        events.ScheduleEvent(EVENT_CONFLIGURATION, 18000);
                        break;
                    case EVENT_THUNDERCLAP:
                        DoCastVictim(SPELL_THUNDERCLAP);
                        events.ScheduleEvent(EVENT_THUNDERCLAP, 20000);
                        break;
                    case EVENT_GUARD_CALL_PULSE:
                        if(me->GetVictim())
                            ForceGuardsIntoCombat(me->GetVictim());
                        events.ScheduleEvent(EVENT_GUARD_CALL_PULSE, 2000);
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
        return new boss_drakkisathAI(creature);
    }
};

// 10814 Drakk guard's behavior. Switched from SAI to implement better evade/reset handling.
class npc_drakkisath_chromatic_elite_guard : public CreatureScript
{
public:
    npc_drakkisath_chromatic_elite_guard() : CreatureScript("npc_drakkisath_chromatic_elite_guard") { }

    struct npc_drakkisath_chromatic_elite_guardAI : public CreatureAI
    {
        EventMap events;

        npc_drakkisath_chromatic_elite_guardAI(Creature* creature) : CreatureAI(creature) { }

        void EnterCombat(Unit* victim) override
        { 
            CreatureAI::EnterCombat(victim);                  

            //schedule the events/spells that were defined by the old SAI
            events.ScheduleEvent(EVENT_MORTAL_STRIKE, urand(5000, 12800));
            events.ScheduleEvent(EVENT_STRIKE, urand(12000, 20800));
            events.ScheduleEvent(EVENT_KNOCKDOWN, urand(5600, 15400));

            events.ScheduleEvent(EVENT_TRY_FORCE_DRAKK_INTO_COMBAT, 1000);
        }

        void Reset() override
        {
            events.Reset();
            CreatureAI::Reset();
        }

        /*# entryorguid, source_type, id, link, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, action_type, action_param1, action_param2, action_param3, action_param4, action_param5, action_param6, target_type, target_param1, target_param2, target_param3, target_x, target_y, target_z, target_o, comment
        '10814', '0', '0', '0', '0', '0', '100', '2', '5000', '12800', '13000', '13000', '11', '15708', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Chromatic Elite Guard - In Combat - Cast \'Mortal Strike\' (No Repeat) (Normal Dungeon)'
        '10814', '0', '1', '0', '0', '0', '100', '2', '5600', '15400', '11200', '25700', '11', '16790', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Chromatic Elite Guard - In Combat - Cast \'Knockdown\' (No Repeat) (Normal Dungeon)'
        '10814', '0', '2', '0', '0', '0', '80', '2', '12000', '20800', '9000', '9000', '11', '15580', '0', '0', '0', '0', '0', '2', '0', '0', '0', '0', '0', '0', '0', 'Chromatic Elite Guard - In Combat - Cast \'Strike\' (No Repeat) (Normal Dungeon)'*/
        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                //Just copying the SAI
                switch (eventId)
                {
                case SPELL_MORTAL_STRIKE:
                    DoCastVictim(SPELL_MORTAL_STRIKE);
                    events.ScheduleEvent(EVENT_MORTAL_STRIKE, urand(13000, 13000));
                    break;
                case EVENT_KNOCKDOWN:
                    DoCastVictim(SPELL_KNOCKDOWN);
                    events.ScheduleEvent(EVENT_KNOCKDOWN, urand(11200, 25700));
                    break;
                case EVENT_STRIKE:
                    DoCastVictim(SPELL_STRIKE);
                    events.ScheduleEvent(EVENT_STRIKE, urand(9000, 9000));
                    break;
                default:
                case EVENT_TRY_FORCE_DRAKK_INTO_COMBAT:
                    if (Creature* creature = ObjectAccessor::GetCreature(*me, me->GetInstanceScript()->GetObjectGuid(DATA_GENERAL_DRAKKISATH)))
                        if (creature->IsAlive() && !creature->IsInCombat() && !creature->IsInEvadeMode())
                            if (me->GetVictim())
                                creature->CombatStart(me->GetVictim());
                    events.ScheduleEvent(EVENT_TRY_FORCE_DRAKK_INTO_COMBAT, 3000);
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<npc_drakkisath_chromatic_elite_guardAI>(creature);
    }

private:
    enum GuardSpell
    {
        SPELL_MORTAL_STRIKE = 15708,
        SPELL_STRIKE = 15580,
        SPELL_KNOCKDOWN = 16790
    };

    enum GuardEvent
    {
        EVENT_MORTAL_STRIKE = 0,
        EVENT_STRIKE = 1,
        EVENT_KNOCKDOWN = 2,
        EVENT_TRY_FORCE_DRAKK_INTO_COMBAT = 3
    };
};


void AddSC_boss_drakkisath()
{
    new boss_drakkisath();
    new npc_drakkisath_chromatic_elite_guard();
}

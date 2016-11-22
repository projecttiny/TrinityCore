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

enum Spells
{
    SPELL_FLAMEBREAK                = 16785,
    SPELL_IMMOLATE                  = 20294,
    SPELL_TERRIFYINGROAR            = 14100,
    SPELL_BESERKER_CHARGE           = 16636,
    SPELL_FIREBALL                  = 16788
};

enum Events
{
    EVENT_FLAME_BREAK              = 1,
    EVENT_IMMOLATE                 = 2,
    EVENT_TERRIFYING_ROAR          = 3,
    EVENT_BERSERKER_CHARGE         = 4,
    EVENT_FIRE_BALL                = 5
};

class boss_the_beast : public CreatureScript
{
public:
    boss_the_beast() : CreatureScript("boss_the_beast") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_thebeastAI(creature);
    }

    struct boss_thebeastAI : public BossAI
    {
        boss_thebeastAI(Creature* creature) : BossAI(creature, DATA_THE_BEAST) { }

        void Reset() override
        {
            _Reset();
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_BERSERKER_CHARGE, Seconds(2)); //according to a 2006 video he used the charge 2 seconds in
            events.ScheduleEvent(EVENT_FIRE_BALL, Seconds(6));
            events.ScheduleEvent(EVENT_FLAME_BREAK, Seconds(10));
            events.ScheduleEvent(EVENT_IMMOLATE, Seconds(12));
            events.ScheduleEvent(EVENT_TERRIFYING_ROAR, Seconds(23));
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
                    case EVENT_FLAME_BREAK:
                        DoCastVictim(SPELL_FLAMEBREAK);
                        events.ScheduleEvent(EVENT_FLAME_BREAK, urand(8000, 10000)); //video shows interavals between 8 and 10 seconds.
                        break;
                    case EVENT_IMMOLATE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true, -1 * SPELL_IMMOLATE)) //cast immolate on a target without debuff
                            DoCast(target, SPELL_IMMOLATE);
                        events.ScheduleEvent(EVENT_IMMOLATE, Seconds(8));
                        break;
                    case EVENT_TERRIFYING_ROAR:
                        DoCastVictim(SPELL_TERRIFYINGROAR);
                        events.ScheduleEvent(EVENT_TERRIFYING_ROAR, Seconds(20));
                        break;
                    case EVENT_BERSERKER_CHARGE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 38, true))
                        {
                            //randomly charge a target within 38 yards
                            me->CastSpell(target, SPELL_BESERKER_CHARGE);
                        }
                        events.ScheduleEvent(EVENT_BERSERKER_CHARGE, urand(3000, 4500));
                        break;
                    case EVENT_FIRE_BALL:
                        //Videos show he hardcasts it. Don't trigger but ignore mana cost
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 38, true))
                        {
                            me->CastSpell(target, SPELL_FIREBALL, (TriggerCastFlags)(TriggerCastFlags::TRIGGERED_IGNORE_AURA_INTERRUPT_FLAGS | TriggerCastFlags::TRIGGERED_IGNORE_POWER_AND_REAGENT_COST));
                        }
                        events.ScheduleEvent(EVENT_FIRE_BALL, urand(5000, 7000));
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_thebeast()
{
    new boss_the_beast();
}

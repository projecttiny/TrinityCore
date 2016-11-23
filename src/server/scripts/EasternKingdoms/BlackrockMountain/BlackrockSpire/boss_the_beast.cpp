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
#include "Player.h"

enum Spells
{
    SPELL_FLAMEBREAK                = 16785,
    SPELL_IMMOLATE                  = 20294,
    SPELL_TERRIFYINGROAR            = 14100,
    SPELL_BESERKER_CHARGE           = 16636,
    SPELL_FIREBALL                  = 16788
};

enum Say
{
    SAY_BLACKHAND_DOOMED = 0
};

enum Events
{
    EVENT_FLAME_BREAK              = 1,
    EVENT_IMMOLATE                 = 2,
    EVENT_TERRIFYING_ROAR          = 3,
    EVENT_BERSERKER_CHARGE         = 4,
    EVENT_FIRE_BALL                = 5,
    EVENT_EXIT_CAVE_MOVEMENT       = 6,
    EVENT_ENTER_BEAST_ROOM         = 7
};

enum PreEventFlags
{
    BEAST_NONE = 0,
    BEAST_HAS_SCARED_BLACKHAND_ELITES = 1 << 0,
    BEAST_HAS_KILLED_BLACKHAND_ELITES = 1 << 1,
    BEAST_HAS_EXITED_THE_CAVE = 1 << 2

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
        PreEventFlags preEventFlags;

        boss_thebeastAI(Creature* creature) : BossAI(creature, DATA_THE_BEAST) 
        {
            preEventFlags = PreEventFlags::BEAST_NONE;
        }

        void Reset() override
        {
            _Reset();

            //Walk back home
            me->GetMotionMaster()->MoveTargetedHome();

            //If we haven't killed the elites then reset the scare flag
            if ((preEventFlags & PreEventFlags::BEAST_HAS_KILLED_BLACKHAND_ELITES) == 0)
                preEventFlags = (PreEventFlags)(preEventFlags & ~PreEventFlags::BEAST_HAS_SCARED_BLACKHAND_ELITES);
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

        void SetData(uint32 eventId, uint32 /*data*/) override
        {
            if (eventId == EVENT_EXIT_CAVE_MOVEMENT && (preEventFlags & PreEventFlags::BEAST_HAS_EXITED_THE_CAVE) == 0)
            {
                me->GetMotionMaster()->MovePath(NPC_THE_BEAST * 10, false);
                preEventFlags = (PreEventFlags)(preEventFlags | PreEventFlags::BEAST_HAS_EXITED_THE_CAVE);

                //The Beast should also kill everyone around him.
                //Find all the Blackhand Elites nearby.
                std::vector<Creature*> creatureList;
                creatureList.reserve(2);
                GetCreatureListWithEntryInGrid(creatureList, me, NPC_BLACKHAND_ELITE, 20.0f);

                if (creatureList.size() > 0)
                {
                    //Kill them all
                    for (Creature* c : creatureList)
                    {
                        me->Kill(c, false);
                    }
                }
            }
            else if (eventId == EVENT_ENTER_BEAST_ROOM && (preEventFlags & PreEventFlags::BEAST_HAS_SCARED_BLACKHAND_ELITES) == 0) //if the beast hasn't scared them
            {
                preEventFlags = (PreEventFlags)(preEventFlags | PreEventFlags::BEAST_HAS_SCARED_BLACKHAND_ELITES);

                //Find all the Blackhand Elites nearby.
                std::vector<Creature*> creatureList;
                creatureList.reserve(2);
                GetCreatureListWithEntryInGrid(creatureList, me, NPC_BLACKHAND_ELITE, 20.0f);

                //Scare them
                if (creatureList.size() > 0)
                    creatureList[0]->AI()->Talk(SAY_BLACKHAND_DOOMED);  //Have first one shout. Don't kill them right away so they can speak
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                return;
            }        

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
                            DoCast(target, SPELL_IMMOLATE, (TriggerCastFlags)TriggerCastFlags::TRIGGERED_IGNORE_POWER_AND_REAGENT_COST);
                        events.ScheduleEvent(EVENT_IMMOLATE, Seconds(8));
                        break;
                    case EVENT_TERRIFYING_ROAR:
                        DoCastVictim(SPELL_TERRIFYINGROAR);
                        events.ScheduleEvent(EVENT_TERRIFYING_ROAR, Seconds(20));
                        break;
                    case EVENT_BERSERKER_CHARGE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, [](Unit* u) { return u && u->ToPlayer() && u->IsWithinCombatRange(u, 38) && u->IsWithinLOSInMap(u); }))
                        {
                            //randomly charge a target within 38 yards
                            me->CastSpell(target, SPELL_BESERKER_CHARGE);
                        }
                        events.ScheduleEvent(EVENT_BERSERKER_CHARGE, urand(6000, 8000));
                        break;
                    case EVENT_FIRE_BALL:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 38.0f))
                        { 
                            me->CastSpell(target, SPELL_FIREBALL);
                        }
                        events.ScheduleEvent(EVENT_FIRE_BALL, urand(8500, 10000));
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            DoMeleeAttackIfReady();
        }
    };
};

// 2066 Trigger for The Beast pathing event.
class at_nearby_the_beast_cave_enterance : public AreaTriggerScript
{
public:
    at_nearby_the_beast_cave_enterance() : AreaTriggerScript("at_nearby_the_beast_cave_enterance") { }

    bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/) override
    {
        if (player->IsAlive())
            if (InstanceScript* instance = player->GetInstanceScript())
                if (Creature* beast = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_THE_BEAST)))
                {
                    beast->AI()->SetData(EVENT_EXIT_CAVE_MOVEMENT, 0);

                    return true;
                }

        return false;
    }
};

// 2067 Trigger for The Beast pathing event.
class at_enter_the_beast_room : public AreaTriggerScript
{
public:
    at_enter_the_beast_room() : AreaTriggerScript("at_enter_the_beast_room") { }

    bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/) override
    {
        if (player->IsAlive())
            if (InstanceScript* instance = player->GetInstanceScript())
                if (Creature* beast = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_THE_BEAST)))
                {
                    beast->AI()->SetData(EVENT_ENTER_BEAST_ROOM, 0);

                    return true;
                }

        return false;
    }
};

void AddSC_boss_thebeast()
{
    new boss_the_beast();
    new at_nearby_the_beast_cave_enterance();
    new at_enter_the_beast_room();
}

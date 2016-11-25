-- Adds the encounter script to the runes for the first room of UBRS
UPDATE `gameobject_template` SET `ScriptName`='go_dragonspire_hall_room_rune' WHERE `entry`='175200';
UPDATE `gameobject_template` SET `ScriptName`='go_dragonspire_hall_room_rune' WHERE `entry`='175194';
UPDATE `gameobject_template` SET `ScriptName`='go_dragonspire_hall_room_rune' WHERE `entry`='175195';
UPDATE `gameobject_template` SET `ScriptName`='go_dragonspire_hall_room_rune' WHERE `entry`='175196';
UPDATE `gameobject_template` SET `ScriptName`='go_dragonspire_hall_room_rune' WHERE `entry`='175197';
UPDATE `gameobject_template` SET `ScriptName`='go_dragonspire_hall_room_rune' WHERE `entry`='175198';
UPDATE `gameobject_template` SET `ScriptName`='go_dragonspire_hall_room_rune' WHERE `entry`='175199';

-- Removes the static spawn creatures standing inside the Rend gate. This may not be blizzlike but it's easier to deal with.
DELETE FROM creature where guid in (23,21,20,22);

-- These are the summon groups for the Rend event
DELETE FROM creature_summon_groups WHERE summonerId = 10429;
INSERT INTO creature_summon_groups VALUES
(10429, 0, 0, 10447, 202.511, -421.307, 110.987, 3.12414, 4, 40000),
(10429, 0, 0, 10442, 204.015, -418.443, 110.989, 3.19395, 4, 40000),
(10429, 0, 0, 10442, 203.142, -423.999, 110.986, 3.07178, 4, 40000),
(10429, 0, 0, 10442, 201.008, -416.648, 110.974, 3.22886, 4, 40000),
(10429, 0, 1, 10447, 202.511, -421.307, 110.987, 3.12414, 4, 40000),
(10429, 0, 1, 10442, 204.015, -418.443, 110.989, 3.19395, 4, 40000),
(10429, 0, 1, 10742, 203.142, -423.999, 110.986, 3.07178, 4, 40000),
(10429, 0, 1, 10442, 201.008, -416.648, 110.974, 3.22886, 4, 40000),
(10429, 0, 2, 10447, 202.511, -421.307, 110.987, 3.12414, 4, 40000),
(10429, 0, 2, 10442, 204.015, -418.443, 110.989, 3.19395, 4, 40000),
(10429, 0, 2, 10442, 205.015, -418.443, 110.989, 3.19395, 4, 40000),
(10429, 0, 2, 10742, 203.142, -423.999, 110.986, 3.07178, 4, 40000),
(10429, 0, 2, 10442, 201.008, -416.648, 110.974, 3.22886, 4, 40000),
(10429, 0, 3, 10447, 202.511, -421.307, 110.987, 3.12414, 4, 40000),
(10429, 0, 3, 10442, 204.015, -418.443, 110.989, 3.19395, 4, 40000),
(10429, 0, 3, 10442, 205.015, -418.443, 110.989, 3.19395, 4, 40000),
(10429, 0, 3, 10742, 203.142, -423.999, 110.986, 3.07178, 4, 40000),
(10429, 0, 3, 10447, 201.008, -416.648, 110.974, 3.22886, 4, 40000);


-- Adds the AI script for the chromatic handler
UPDATE `creature_template` SET `ScriptName`='npc_rendevent_chromatic_handler' WHERE `entry`='10742';

-- Adds spell conditions to the DB for the Mend Dragon spell the chromatic handler uses

DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`='17' and`SourceGroup`='0' and`SourceEntry`='16637' and`SourceId`='0' and`ElseGroup`='0' and`ConditionTypeOrReference`='29' and`ConditionTarget`='0' and`ConditionValue1`='10447' and`ConditionValue2`='19' and`ConditionValue3`='0';
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`='13' and`SourceGroup`='1' and`SourceEntry`='16637' and`SourceId`='0' and`ElseGroup`='0' and`ConditionTypeOrReference`='38' and`ConditionTarget`='0' and`ConditionValue1`='95' and`ConditionValue2`='4' and`ConditionValue3`='0';
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`) VALUES ('13', '1', '16637', '0', '0', '38', '0', '95', '4', '0', '0', 'Implementing Mend Dragon for Chromatic Dragonspawn healing.');
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES ('17', '0', '16637', '0', '0', '29', '0', '10447', '19', '0', '0', '0', '0', '', 'Prevents casting of Mend Dragon outside of range of a Dragonspawn.');

-- Adds scripts for the area triggers found in The Beast's room
DELETE FROM `areatrigger_scripts` WHERE entry = '2066' OR entry = '2067';
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('2066', 'at_nearby_the_beast_cave_enterance');
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('2067', 'at_enter_the_beast_room');

-- Cata sniffed The Beast waypoints. They seem close but the data is truncated. We don't have a full path. Also, the height values were slighly off.
DELETE FROM `waypoint_data` WHERE id = '104300';
INSERT INTO `waypoint_data` (`id`, `point`, `position_x`, `position_y`, `position_z`, `orientation`, `delay`, `move_type`, `action`, `action_chance`, `wpguid`) VALUES 
('104300', '1', '119.644', '-562.441', '108.59', '0', '0', '0', '0', '100', '0'),
('104300', '2', '106.394', '-558.941', '108.84', '0', '0', '0', '0', '100', '0'),
('104300', '3', '105.464', '-557.138', '107.85', '0', '0', '0', '0', '100', '0'),
('104300', '4', '103.714', '-556.388', '107.9', '0', '0', '0', '0', '100', '0'),
('104300', '5', '102.214', '-555.638', '108.5', '0', '0', '0', '0', '100', '0'),
('104300', '6', '98.2141', '-553.888', '110', '0', '0', '0', '0', '100', '0'),
('104300', '7', '96.7141', '-553.388', '110.5', '0', '0', '0', '0', '100', '0'),
('104300', '8', '93.4641', '-551.888', '110.9', '0', '0', '0', '0', '100', '0'),
('104300', '9', '88.9641', '-550.638', '110.509', '0', '0', '0', '0', '100', '0'),
('104300', '10', '82.6473', '-548.55', '110.938', '0', '0', '0', '0', '100', '0'),
('104300', '11', '66.0555', '-539.752', '110.519', '0', '0', '0', '0', '100', '0'),
('104300', '12', '60.8055', '-537.503', '110.519', '0', '0', '0', '0', '100', '0'),
('104300', '13', '59.0555', '-536.752', '110.519', '0', '0', '0', '0', '100', '0'),
('104300', '14', '57.5555', '-536.002', '110.769', '0', '0', '0', '0', '100', '0'),
('104300', '15', '55.509', '-534.937', '110.941', '0', '0', '0', '0', '100', '0');

-- Adds The Beast blackhand elites speech
DELETE FROM `creature_text` WHERE entry = '10317';
INSERT INTO `creature_text` (`entry`, `groupid`, `id`, `text`, `type`, `language`, `probability`, `emote`, `duration`, `sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES ('10317', '0', '0', 'We\'re doomed!', '14', '0', '0', '0', '0', '0', '5622', '0', 'UBRS - The Beast tramples the elites causing them to shout about their doom.');


-- Removes SAI implementation of Drakk adds so that evade behavior can be implemented
DELETE FROM `smart_scripts` WHERE `entryorguid`='10814' and`source_type`='0' and`id`='0' and`link`='0';
DELETE FROM `smart_scripts` WHERE `entryorguid`='10814' and`source_type`='0' and`id`='1' and`link`='0';
DELETE FROM `smart_scripts` WHERE `entryorguid`='10814' and`source_type`='0' and`id`='2' and`link`='0';
UPDATE `creature_template` SET `AIName`='', `ScriptName`='npc_drakkisath_chromatic_elite_guard' WHERE `entry`='10814';

-- Adds an aura script for the spell conflaguration that handles the threat drop mechanic
DELETE FROM `spell_script_names` WHERE spell_id = '16805';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES ('16805', 'spell_gen_boss_conflagration');
-- Adds Thorim's p2 area trigger
DELETE FROM `areatrigger_scripts` WHERE entry = '5357';
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES ('5357', 'at_thorim_platform_engage');

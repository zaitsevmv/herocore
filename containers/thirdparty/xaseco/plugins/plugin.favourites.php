<?php

/**
 * @name plugin.favourites.php
 * @date 04-05-2011
 * @version v0.1.0
 * @website mp.klaversma.eu
 *
 * @author Max "TheM" Klaversma
 * @copyright 2010 - 2012
 *
 * ---------------------------------------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ---------------------------------------------------------------------
 * You are allowed to change things of use this in other projects, as
 * long as you leave the information at the top (name, date, version,
 * website, package, author, copyright) and publish the code under
 * the GNU General Public License version 3.
 * ---------------------------------------------------------------------
 *
 * The following action id's are used in this plugin:
 * 6850000 - 6851900		for deleting max. 1900 maps from the favourites list
 */

Aseco::registerEvent('onStartup', 'favourite_init');
Aseco::addChatCommand('favourites', 'Shows you a list of your favourite maps');
Aseco::addChatCommand('addfavourite', 'Adds the current map or some other map to your favourites list');
Aseco::addChatCommand('removefavourite', 'Removes the current map or some other map from your favourites list');
Aseco::addChatCommand('fav', 'Shows you a list of your favourite maps');
Aseco::addChatCommand('af', 'Adds the current map or some other map to your favourites list');
Aseco::addChatCommand('rf', 'Removes the current map or some other map from your favourites list');
Aseco::registerEvent('onPlayerManialinkPageAnswer', 'favo_onPlayerManialinkPageAnswer');

function favourite_init($aseco) {
	$aseco->console('[Favourite] [MySQL] Creating table if needed . . . ');
	mysql_query("CREATE TABLE IF NOT EXISTS `favourites` (
  `Id` int(255) NOT NULL AUTO_INCREMENT,
  `PlayerId` mediumint(9) NOT NULL,
  `MapId` mediumint(9) NOT NULL,
  `AddedAt` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  PRIMARY KEY (`Id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Added by plugin.favourite.php' AUTO_INCREMENT=1 ;");
	$aseco->console('[Favourite] [MySQL] Table creation done!');

	// Register this to the global version pool (for up-to-date checks)
	$aseco->plugin_versions[] = array(
		'plugin'   => 'plugin.favourites.php',
		'author'   => 'TheM',
		'version'   => '0.1.0'
	);
} // favourite_init

function chat_favourites($aseco, $command) {
	global $maxrecs, $jb_buffer;

	$player = $command['author'];
	$login = $player->login;
	$playera = $aseco->server->players->player_list[$login];
	$playerid = $playera->id;

	// show the list
	$player->maplist = array();

	// get list of ranked records
	$reclist = get_recs($player->id);
	// get new/cached list of maps
	$newlist = getMapsCache($aseco);

	$envids = array('Canyon' => 11, 'Valley' => 12);
	$head = 'Your Favourite Maps On This Server:';
	$msg = array();
	if($aseco->server->packmask != 'Canyon')
		$msg[] = array('Id', 'Name', 'Author', 'Added at', 'Env', 'Remove');
	else
		$msg[] = array('Id', 'Name', 'Author', 'Added at', 'Remove');

	$tid = 1;
	$lines = 0;
	$player->msgs = array();
	// reserve extra width for $w tags
	$extra = ($aseco->settings['lists_colormaps'] ? 0.2 : 0);
	if($aseco->server->packmask != 'Canyon')
		$player->msgs[0] = array(1, $head, array(1.44+$extra, 0.12, 0.5+$extra, 0.25, 0.25, 0.17, 0.15), array('Icons128x128_1', 'NewTrack', 0.02));
	else
		$player->msgs[0] = array(1, $head, array(1.27+$extra, 0.12, 0.5+$extra, 0.25, 0.25, 0.15), array('Icons128x128_1', 'NewTrack', 0.02));

	$mapsonserver = array();
	$mapsperuid = array();

	foreach($newlist as $row) {
		$mapsonserver[] = $row['UId'];
		$mapsperuid[$row['UId']] = $row;
	}

	$query = mysql_query("SELECT * FROM `favourites` WHERE `PlayerId` = '".$playerid."' ORDER BY `AddedAt` DESC");
	while($info = mysql_fetch_object($query)) {
		// check for wildcard, map name or author name
		$pos = 0;
		$pose = 0;

		$row = mysql_fetch_array(mysql_query("SELECT * FROM `maps` WHERE `Id` = '".$info->MapId."'"));

		// check for any match
		if($pos !== false && $pose !== false && in_array($row['Uid'], $mapsonserver)) {
			// store map in player object for jukeboxing
			$serverinfo = $mapsperuid[$row['Uid']];
			$trkarr = array();
			$trkarr['name'] = $row['Name'];
			$trkarr['author'] = $row['Author'];
			$trkarr['environment'] = $row['Environment'];
			$trkarr['filename'] = $serverinfo['FileName'];
			$trkarr['uid'] = $row['Uid'];
			$trkarr['addedat'] = $info->AddedAt;

			$player->maplist[] = $trkarr;

			// format map name
			$mapname = $row['Name'];
			if(!$aseco->settings['lists_colormaps'])
				$mapname = stripColors($mapname);
			// grey out if in history
			if(in_array($row['Uid'], $jb_buffer)) {
				$mapname = '{#grey}' . stripColors($mapname);
			} else {
				$mapname = '{#black}' . $mapname;
				// add clickable button
				if ($aseco->settings['clickable_lists'] && $tid <= 1900)
					$mapname = array($mapname, $tid+100);  // action id
			}

			// format author name
			$mapauthor = $row['Author'];
			// add clickable button
			if($aseco->settings['clickable_lists'] && $tid <= 1900)
				$mapauthor = array($mapauthor, -100-$tid);  // action id
			// format added at date/time
			$addedat = $info->AddedAt;
			$date = strtotime($addedat);
			$addedat = date('d-m-Y H:i:s', $date);
			// format env name
			$mapenv = $row['Environment'];
			// add clickable button
			if($aseco->settings['clickable_lists'])
				$mapenv = array($mapenv, $envids[$row['Environment']]);  // action id
			// remove button
			$remove = array('Remove', (6850000+$tid));  // action id

			if($aseco->server->packmask != 'Canyon')
				$msg[] = array(str_pad($tid, 3, '0', STR_PAD_LEFT) . '.',
					$mapname, $mapauthor, $addedat, $mapenv, $remove);
			else
				$msg[] = array(str_pad($tid, 3, '0', STR_PAD_LEFT) . '.',
					$mapname, $mapauthor, $addedat, $remove);
			$tid++;
			if(++$lines > 14) {
				$player->msgs[] = $msg;
				$lines = 0;
				$msg = array();
				if ($aseco->server->packmask != 'Canyon')
					$msg[] = array('Id', 'Name', 'Author', 'Added at', 'Env', 'Remove');
				else
					$msg[] = array('Id', 'Name', 'Author', 'Added at', 'Remove');
			}
		}
	}
	// add if last batch exists
	if(count($msg) > 1)
		$player->msgs[] = $msg;

	if(empty($player->maplist)) {
		$message = '{#server}> {#error}No maps found, try again!';
		$aseco->client->query('ChatSendServerMessageToLogin', $aseco->formatColors($message), $login);
		return;
	}

	// display ManiaLink message
	display_manialink_multi($player);
}

function chat_fav($aseco, $command) {
	chat_favourites($aseco, $command);
}

function chat_addfavourite($aseco, $command) {
	$player = $command['author'];
	$login = $player->login;
	$playera = $aseco->server->players->player_list[$login];
	$playerid = $playera->id;

	$listq = mysql_query("SELECT * FROM `favourites` WHERE `PlayerId` = '".$playerid."'");
	$favourites = array();
	while($favo = mysql_fetch_object($listq)) {
		$favourites[] = $favo->MapId;
	}

	if(!isset($command['params']) || $command['params'] == '') {
		// add current map
		$maptoadd = $aseco->server->map->id;
		if(!in_array($maptoadd, $favourites)) {
			mysql_query("INSERT INTO `favourites` (`PlayerId`, `MapId`, `AddedAt`) VALUES ('".$playerid."', '".$maptoadd."', '".date('Y-m-d H:i:s')."');");
			$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $fa0You added $fff'.$aseco->server->map->name.'$z$s$fa0 by $fff'.$aseco->server->map->author.'$z$s$fa0 to your favourites list!', $login);
		} else {
			$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00This map is already in your favourites list!', $login);
		}
	} elseif(is_numeric($command['params'])) {
		// add map from maplist
		$maptoadd = ($command['params']-1);
		if(is_array($player->maplist)) {
			if(isset($player->maplist[$maptoadd])) {
				$dbinfo = mysql_fetch_object(mysql_query("SELECT * FROM `maps` WHERE `Uid` = '".$player->maplist[$maptoadd]['uid']."'"));
				if(!in_array($dbinfo->Id, $favourites)) {
					$trackname = $player->maplist[$maptoadd]['name'];
					$author = $player->maplist[$maptoadd]['author'];
					mysql_query("INSERT INTO `favourites` (`PlayerId`, `MapId`, `AddedAt`) VALUES ('".$playerid."', '".$dbinfo->Id."', '".date('Y-m-d H:i:s')."');");
					$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $fa0You added $fff'.$trackname.'$z$s$fa0 by $fff'.$author.'$z$s$fa0 to your favourites list!', $login);
				} else {
					$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00That map is already in your favourites list!', $login);
				}
			} else {
				$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00Unknown map!', $login);
			}
		} else {
			$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00Use a maplist function first before adding a favourite!', $login);
		}
	} else {
		$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00Unknown action!', $login);
	}
}

function chat_af($aseco, $command) {
	chat_addfavourite($aseco, $command);
}

function chat_removefavourite($aseco, $command) {
	$player = $command['author'];
	$login = $player->login;
	$playera = $aseco->server->players->player_list[$login];
	$playerid = $playera->id;

	$listq = mysql_query("SELECT * FROM `favourites` WHERE `PlayerId` = '".$playerid."'");
	$favourites = array();
	while($favo = mysql_fetch_object($listq)) {
		$favourites[] = $favo->MapId;
	}

	if(!empty($favourites)) {
		if(!isset($command['params']) || $command['params'] == '') {
			// add current map
			$maptorem = $aseco->server->map->id;
			if(in_array($maptorem, $favourites)) {
				mysql_query("DELETE FROM `favourites` WHERE `PlayerId` = '".$playerid."' AND `MapId` = '".$maptorem."';");
				$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $fa0You removed $fff'.$aseco->server->map->name.'$z$s$fa0 by $fff'.$aseco->server->map->author.'$z$s$fa0 from your favourites list!', $login);
			} else {
				$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00This map is not in your favourites list, so you can\'t remove it!', $login);
			}
		} elseif(is_numeric($command['params'])) {
			// add map from maplist
			$maptorem = ($command['params']-1);
			if(is_array($player->maplist)) {
				if(isset($player->maplist[$maptorem])) {
					$dbinfo = mysql_fetch_object(mysql_query("SELECT * FROM `maps` WHERE `Uid` = '".$player->maplist[$maptorem]['uid']."'"));
					if(in_array($dbinfo->Id, $favourites)) {
						$trackname = $player->maplist[$maptorem]['name'];
						$author = $player->maplist[$maptorem]['author'];
						//mysql_query("INSERT INTO `favourites` (`PlayerId`, `MapId`, `AddedAt`) VALUES ('".$playerid."', '".$dbinfo->Id."', '".date('Y-m-d H:i:s')."');");
						mysql_query("DELETE FROM `favourites` WHERE `PlayerId` = '".$playerid."' AND `MapId` = '".$dbinfo->Id."';");
						$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $fa0You removed $fff'.$trackname.'$z$s$fa0 by $fff'.$author.'$z$s$fa0 from your favourites list!', $login);
					} else {
						$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00That map is not in your favourites list, so you can\'t remove it!', $login);
					}
				} else {
					$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00Unknown map!', $login);
				}
			} else {
				$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00Use a maplist function first before removing a favourite!', $login);
			}
		} else {
			$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00Unknown action!', $login);
		}
	} else {
		$aseco->client->query('ChatSendServerMessageToLogin', '$ff0> $i$f00You don\'t have any favourites, so you can\'t remove one!', $login);
	}
}

function chat_rf($aseco, $command) {
	chat_removefavourite($aseco, $command);
}

// called @ onPlayerManialinkPageAnswer
// Handles ManiaLink Favourites responses
// [0]=PlayerUid, [1]=Login, [2]=Answer, [3]=Entries
function favo_onPlayerManialinkPageAnswer($aseco, $answer) {
	// only use 685****
	$action = (int) $answer[2];
	if ($action >= 6850000 && $action <= 6851900) {
		// get player
		$player = $aseco->server->players->getPlayer($answer[1]);

		// log clicked command
		$aseco->console('player {1} clicked command "/removefavourite {2}"',
			$player->login, $action-6850000);

		// remove selected map
		$command = array();
		$command['author'] = $player;
		$command['params'] = $action-6850000;
		chat_removefavourite($aseco, $command);
	}
}
?>
#!/bin/sh

server=$1

sed "s|<password_sa></password>|<password>$(./get_secret.sh $server-secrets master_pw)</password>|g" GameData/Config/dedicated_cfg.txt > GameData/Config/new_cfg
sed -i "s|<password_ad></password>|<password>$(./get_secret.sh $server-secrets admin_pw)</password>|g" GameData/Config/new_cfg
sed -i "s|<login_server></login>|<login>$server</login>|g" GameData/Config/new_cfg
sed -i "s|<name_server></name>|<name>$server</name>|g" GameData/Config/new_cfg
sed -i "s|<password_us></password>|<password>$(./get_secret.sh $server-secrets user_pw)</password>|g" GameData/Config/new_cfg
sed -i "s|<password_server></password>|<password>$(./get_secret.sh $server-secrets server_pw)</password>|g" GameData/Config/new_cfg

cat GameData/Config/new_cfg

./TrackmaniaServer /game_settings=/tm-server/GameData/Tracks/MatchSettings/tracklist.txt /dedicated_cfg=new_cfg > logs

sleep 1

SERVER_PID=$(grep -o 'pid=[0-9]*' logs | head -n 1 | cut -d= -f2)

while kill -0 "$SERVER_PID" 2>/dev/null; do
    sleep 5
done

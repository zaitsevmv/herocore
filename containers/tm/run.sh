cp -r ../../thirdparty/tm server_data
cd server_data

server=$1

sed "s|<password_sa></password>|<password>$(sec $server-secrets master_pw)</password>|g" ../config_tm > GameData/Config/new_cfg
sed -i "s|<password_ad></password>|<password>$(sec $server-secrets admin_pw)</password>|g" GameData/Config/new_cfg
sed -i "s|<password_us></password>|<password>$(sec $server-secrets user_pw)</password>|g" GameData/Config/new_cfg
sed -i "s|<password_server></password>|<password>$(sec $server-secrets server_pw)</password>|g" GameData/Config/new_cfg

./TrackmaniaServer /game_settings=MatchSettings/CustomSettings/cswh.txt /dedicated_cfg=new_cfg
#!/bin/sh

server=$1

sed -i "s|<password_sa></password>|<password>$(./get_secret.sh $server-secrets master_pw)</password>|g" config.xml
sed -i "s|<ip>127.0.0.1</ip>|<ip>$server-base</ip>|g" config.xml

sed -i "s|<password_server></password>|<password>$(./get_secret.sh $server-secrets server_pw)</password>|g" dedimania.xml
sed -i "s|<login></login>|<login>$server</login>|g" dedimania.xml

sed -i "s|<mysql_server></mysql_server>|<mysql_server>$server-mysql</mysql_server>|g" localdatabase.xml
sed -i "s|<mysql_password></mysql_password>|<mysql_password>$(./get_secret.sh $server-secrets mysql_pw)</mysql_password>|g" localdatabase.xml

php aseco.php TMF </dev/null >&1 2>&1 &

SERVER_PID=$!

while kill -0 "$SERVER_PID" 2>/dev/null; do
    sleep 5
done

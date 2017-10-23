#! /bin/sh
# /var/www/chauffage/scripts

cd /var/www/chauffage
sudo cp log_temperature.log log/
sudo cp log_bruleur.log log/
sudo python purge.py
sleep 2
./temperature&
sleep 2
./chaudiere&
sleep 2
python web.py&

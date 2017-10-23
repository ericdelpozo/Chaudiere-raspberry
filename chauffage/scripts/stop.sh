#! /bin/sh
# /var/www/chauffage/scripts

echo 1 > /var/www/chauffage/log/chaudiere.stop
echo 1 > /var/www/chauffage/log/log_temperature.stop
echo  "attente de 20 secondes que les services se terminent proprement"
sleep 20
sudo killall python
sudo kill chaudiere
sudo kill temperature
sudo cp /var/www/chauffage/log/log_temperature.log /var/www/chauffage/log_temperature.log
sudo cp /var/www/chauffage/log/log_bruleur.log /var/www/chauffage/log_bruleur.log

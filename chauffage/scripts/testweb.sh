#! /bin/sh
# /var/www/chauffage/scripts

cd /var/www/chauffage/log
sudo rm /var/www/chauffage/log/get_pid
wget -T 5 -t 3 192.168.0.78:5000/get_pid
fichier="get_pid"
fichier3="web.pid"
if [ -e $fichier ]
then
echo "pas de problème"
else
date >> testweb.log
echo "********************************probleme serveur***************************" >> ../log/testweb.log
if [ -e $fichier3 ]
then
while IFS=$'\n' read ligne
do
sudo kill $ligne
done <  "web.pid"
sudo rm web.pid
sudo rm get_pid
cd ..
python web.py&
else
cd ..
python web.py&
fi
fi

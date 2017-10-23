echo "installation des paquets supplémentaires : nano, git, wiringpi"
read -p "proceder à l'installation de ces paquets ? (O/N) : " rep
if [ $rep = "O" ] || [ $rep = "o" ] 
then

sudo apt-get update
sudo apt-get install -y nano
sudo apt-get install -y git-core
git clone git://git.drogon.net/wiringPi
fi

echo "##########################################################################"
echo ""
read -p "1) Renseignement de l'adresse ip (O/N)? : " rep
if [ $rep = "o" ] || [ $rep = "O" ]
then
ifconfig 
read -p "Saisir l'adresse ip: " adrip
echo ""
fi
read -p  "2) Recherche de l'adresse de la sonde(O/N)? : " rep
if [ $rep = "o" ] || [ $rep = "O" ]
then
echo "s'assurer de la présence de ces deux lignes dans /etc/modules"
echo "w1-therm"
echo "w1-gpio pullup=1"
read -p "presser entrée: "
sudo nano /etc/modules

echo "Liste des sondes présentes dans /sys/bus/w1/devices"
ls /sys/bus/w1/devices/28* -d
read -p  "Saisir l'adresse  de la sonde: " adrsonde
echo ""
fi

read -p  "3) Réglage du Pin gpio du relai(numérotation physique)? (O/N) : " rep
if [ $rep = "o" ] || [ $rep = "O" ]
then
echo "Classiquement: 7,11,12,13,15,16,18,22,29,31,32,33,25,26,27,38,40"
read -p  "Pin: " adrpin
echo ""

read -p  "4) Generation du fichier config.xml? (O/N) :  " rep
if [ $rep = "o" ] || [ $rep = "O" ]
then
echo "<?xml version=\"1.0\"?>" > config.xml
echo "<setting  nom=\"chauffage chaudiere\">" >> config.xml
echo "<ip>"$adrip"</ip>" >> config.xml
echo "<port>5000</port>" >> config.xml
echo "<temperature_haute>20</temperature_haute>" >> config.xml
echo "<temperature_basse>17</temperature_basse>" >> config.xml
echo "<programme_defaut>defaut</programme_defaut>" >> config.xml
echo "<sonde>"$adrsonde"</sonde>" >> config.xml
echo "<autostart>1</autostart>" >> config.xml
echo "<pin_gpio>"$adrpin"</pin_gpio>" >> config.xml
echo "<seuil>0.4</seuil>" >> config.xml
echo "</setting>" >> config.xml
else
echo "editer le fichier config.xml et verifier les informations"
fi

read -p  "5) Lancement automatique au démarrage ? (O/N) : " rep
if [ $rep = "o" ] || [ $rep = "O" ]
then
sudo cp chauffage_boot /etc/init.d/chauffage_boot
sudo chmod 755 /etc/init/d/chauffage_boot
sudo update-rc.d chauffage_boot defaults
echo ""
else
echo "Il faudra lancer manuellement les services, par exemple:"
echo "cd /var/www/chauffage"
echo "cp *.log log/"
echo "sudo python web.py&"
echo "sudo ./temperature&"
echo "sudo ./chaudiere&"
echo ""
fi
read -p  "6) Mise en place de la crontan ?(O/N) :  " rep
if [ $rep = "o" ] || [ $rep = "O" ]
then
echo "Ajouter manuellement ces lignes à la fin de /etc/crontab"
echo ""
echo "*/20 *  * * *   root    /var/www/chauffage/scripts/reconnect.sh"
echo "*/10 *  * * *   root    /var/www/chauffage/scripts/testweb.sh"
echo "0 0     * * *   root    /var/www/chauffage/scripts/cplog.sh"
read -p "presser une touche"
sudo nano /etc/crontab
else
echo "sans les tâches de la crontab, en cas de deconnexion internet, le serveur ne se reconnectera pas"
echo "le serveur web, s'il plante, n'est plus relancé"
fi

read -p  "7) Monter en ram le repertoire /var/www/chauffage/log ? (O/N) : " rep
if [ $rep = "o" ] || [ $rep = "O" ]
then
echo "Ajouter dans /etc/fstab la ligne suivante:"
echo "tmpfs /var/www/chauffage/log tmpfs defaults 0,0"
read "Presser Entrée "
sudo nano /etc/fstab
rm /var/www/chauffage/log/*
read -p "redemarrer maintenant (conseillé) O/N ?: " rep
if [ $rep = "O" ] || [ $rep = "o" ]
then
sudo reboot
fi
else
echo "plusieurs fichiers sont régulièrement écrits dans le /var/www/chauffage/log"
echo "pour limiter l'usure de la carte SD, c'est intéressant de monter ce répertoire en RAM"
fi

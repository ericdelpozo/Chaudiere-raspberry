#/usr/bin/env python
# -*- coding:utf-8 -*-

from flask import Flask, request,redirect , render_template, url_for
from libchaudiere import *
import RPi.GPIO as GPIO
import xml.etree.ElementTree as ET
import os
import io
import time
import sys
import socket
import gc
from datetime import date
app = Flask(__name__)



@app.route('/edit/')
def vueedit():
	return render_template('editprgm.html')


@app.route('/courbes/')
def vue():
#creation des dates personalisées

	maintenant=time.localtime()
	maintenant=(maintenant.tm_year,maintenant.tm_mon,maintenant.tm_mday,0,0,0,0,0,1)
	maintenant=time.mktime(maintenant)	
	all_dates=[]
	for i in range(30):
		dt=time.localtime(maintenant)
		maintenant-=86400
		all_dates.append(str(dt.tm_mday)+"/"+str(dt.tm_mon)+"/"+str(dt.tm_year))
	return render_template('courbes.html',dates=all_dates)

@app.route('/detailprgm/')
def vueprg():
	detailprgm=get_detail_programme()
	return render_template('detailprgm.html',detail=detailprgm)




@app.route('/get_pid/')
def pross():
	return (str(os.getpid()))

@app.route('/courbe/', methods=['GET', 'POST'])
def vue_courbe():
	reboot_if_low_memory(True)
	
	if request.method == 'POST':
		if 	request.form['action'] == "1j":
			im=makepicture(500,300,time.time()-86400,time.time())
		if 	request.form['action'] == "2j":
			im=makepicture(500,300,time.time()-86400*2,time.time())
		if 	request.form['action'] == "1s":
			im=makepicture(500,300,time.time()-86400*7,time.time())
		if 	request.form['action'] == "-->Ok":
			
			d=request.form['sel_date'].split('/')
			d=(int(d[2]),int(d[1]),int(d[0]),0,0,0,0,0,1)
			d=time.mktime(d)
			im=makepicture(500,300,d,d+86400)
			
		if im!=None:
			reponse = make_response(im.getvalue())
			reponse.mimetype = "image/bmp"  # à la place de "text/html"
			return reponse
		else:
			return "Pas de courbe sur cette période"
	else:return redirect("/courbes/", code=302)
		
	

@app.route('/', methods=['GET', 'POST'])
def serveur():
#récupère les paramètres actuels :listes programmes, programme actif, temperature et etat de la chaudière
	reboot_if_low_memory(True)
	prg=liste_programmes()
	bruleur=get_etat_relai()
	temp_actuel=get_temperature(get_sonde())
	if temp_actuel!=None:
		temp_actuel=round(temp_actuel,1)
	else:
		temp_actuel=None
#traitement des actions du serveur
	if request.method == 'POST':
		if 	request.form['action'] == "Stopper le programme":
			stop_chauffage()
			bruleur=get_etat_relai()
		if 	request.form['action'] == "Eteinde le serveur":
			log_activite("extiction du system demandé")
			termine_services()
			os.system("sudo halt")
			sys.exit(0)
			return "Arrêt serveur."
		if 	request.form['action'] == "Relancer le serveur":
			log_activite("Redemarrage demandé")
			termine_services()
			os.system("sudo reboot")
			sys.exit(0)
			return "Redemarrage..."
		if 	request.form['action'] == "Executer":
			launch_newprgm(request.form['sel_prgm']);
			time.sleep(10)
			bruleur=get_etat_relai()
		return render_template('index.html', pactif=programme_actif(),temp=temp_actuel,prgm=prg,etat_bruleur=bruleur)
	else:
		return render_template('index.html', pactif=programme_actif(),temp=temp_actuel,prgm=prg,etat_bruleur=bruleur)


if __name__ == '__main__':
	if os.path.exists("log/web.pid"):
	# log_activite("web.pid deja présent, stoppé puis relancé")
		fichier=open("log/web.pid",'r')
		p=fichier.readline()
		fichier.close()
		os.system("sudo kill "+p)
		os.system("sudo rm log/web.pid")
#	log_activite("Demarrage de web.py")
	tree = ET.parse('config.xml')
	root = tree.getroot()
	ip=root.find("ip")
	tentative=0
	succes=False
	t1=time.time()
	t2=t1
	a=-1
	print("Attente de la connexion reseau "+ip.text)
	while t2<t1+120 and a!=0:
		t2=time.time()
		time.sleep(1)
		a=os.system("ping -c1 "+ip.text)
		if a==0:
			succes=True
	if succes==False:
		log_activite("après 2minutes, l'adresse ip spécifiée dans config.xml est injoignable, problème réseau ")
	else:
		try:
			fichier=open("log/web.pid",'w')
			fichier.write(str(os.getpid())+"\n")
			fichier.close()
			app.run(host=ip.text)
		except KeyboardInterrupt:
			log_activite("interruption par le clavier de web.py")

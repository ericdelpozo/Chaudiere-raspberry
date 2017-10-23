#!/usr/bin/env python
# -*- coding: utf-8 -*-
import xml.etree.ElementTree as ET
import io,os
import time
import RPi.GPIO as GPIO
from PIL import Image
import ImageDraw
import ImageFont
from StringIO import StringIO
from flask import make_response



def get_etat_relai():
	GPIO.setmode(GPIO.BOARD)
	GPIO.setup(get_pin_gpio(),GPIO.OUT)
	return GPIO.input(get_pin_gpio())
	

def asked_stop_log_temp():
	return os.path.exists("log/log_temperature.stop")

def asked_stop_web():
	return os.path.exists("log/web.stop")

def asked_stop_chauffage():
	return os.path.exists("log/chaudiere.stop")


#ne conserve que les date datant de moins de 1 mois
def purge_log_temperature():
	d=time.time()
	if os.path.exists("log/log_temperature.log"):
		os.system("cp log/log_temperature.log log/log_temperature.log2")
		f1=open("log/log_temperature.log2","r")
		f2=open("log/log_temperature.log3","w")
		t1=f1.readlines()
		f1.close()
		for l in t1:
			line=l.split("/")
			try:
				if int(float(line[0]))>d-86400*30:
					f2.write(l)
			except ValueError:
				print("\n erreur de valeur temperature.log:"+ l)
		f2.close()
		os.system("sudo mv log/log_temperature.log3 log/log_temperature.log")
		os.system("sudo rm log/log_temperature.log2")

def purge_log_bruleur():
	d=time.time()
	if os.path.exists("log/log_bruleur.log"):
		os.system("cp log/log_bruleur.log log/log_bruleur.log2")
		f1=open("log/log_bruleur.log2","r")
		f2=open("log/log_bruleur.log3","w")
		t1=f1.readlines()
		f1.close()
		for l in t1:
			line=l.split("/")
			try:
				if int(float(line[0]))>d-86400*30:
					f2.write(l)
			except ValueError:
				print("\n erreur de valeur bruleur.log:"+ l)
		f2.close()
		os.system("sudo mv log/log_bruleur.log3 log/log_bruleur.log")
		os.system("sudo rm log/log_bruleur.log2")




def round_sup(x):
	r=int(x*10)-10*int(x)
	if r==0:
		return x
	if r<=5:
		return int(x)*1.0+0.5
	else:
		return int(x)+1

#création du graphique des températres
def makepicture(sx,sy,date1,date2):
	margex=50
	margey=30
	tab=None
	tabb=None
	if not (os.path.exists("log/log_temperature.log")):
		return None
	os.system("cp log/log_temperature.log log/log_tmp")
	fichier=open("log/log_tmp","r") 
	tab=fichier.readlines()
	fichier.close()
	if os.path.exists("log/log_bruleur.log"):
		os.system("cp log/log_bruleur.log log/log_tmp")
		fichier=open("log/log_tmp","r")
		tabb=fichier.readlines()
		fichier.close()
	os.remove("log/log_tmp")
	x=[]
	y=[]
	z=[]
	if tab!=None:
		for l in tab:
			if float(l.split('/')[0])>=date1 and float(l.split('/')[0])<date2:
					try:
						x.append(float(l.split('/')[0]))
						y.append(float(l.split('/')[1]))
					except ValueError:
						print("erreur log_temperature, convertion en float ")
	
	if len(x)>0 and len(y)>0:
		nbpoint=min(len(x),len(y))
		starty=int(min(y)-0.5)
		endy=int(max(y)+1)
		startx=x[0]
		endx=x[nbpoint-1]
		if startx==endx or starty==endy or nbpoint<2:
			os.remove("log/im.png")
			return None
		im = Image.new("RGB", (sx,sy), "lightgrey")
		draw = ImageDraw.Draw(im)	
		
		#fonctions affines qui permettent de construire le graphique avec la bonne echelle
		
		fx=lambda x: (x-startx)/(endx-startx)*(sx-2.0*margex)+margex
	
		fx2=lambda x: (x-margex)/(sx-2.0*margex)*(endx-startx)+startx
		
		fy=lambda x: (x-starty)/(endy-starty)*(2*margey-sy)+sy-margey
	
			
		#determine les graduations visibles sur les abscisses 
		grad_heures=fx(3600)-fx(0)>5
		grad_jours=fx(86400)-fx(0)>5
		
		#détermine la graduation des odronnées
		grad_y=round_sup(float((float(endy)-float(starty))/10))
		
		font = ImageFont.truetype('Vera.ttf',10)

		

	#Ligne brisée inférieure
	
		p=0
		while (p<nbpoint-2):
			d=fx2(fx(x[p])+1)
			p2=p+1
			while (p2<nbpoint-1 and x[p2]<d):
				p2=p2+1
			if x[p2]>=d:
				draw.polygon([fx(x[p]),sy-margey,fx(x[p]),fy(y[p]),fx(x[p2]),fy(y[p2]),fx(x[p2]),sy-margey],fill="green")
			p=p2		
#création des partie où le bruleur est actif	
		duree_seconde=0
		if tabb!=None:
			for l in tabb:
				if float(l.split('/')[0])<endx and  float(l.split('/')[0])>startx:
					try:
						z.append(float(l.split('/')[0]))
						z.append(float(l.split('/')[1]))
					except ValueError:
						print("erreur log_bruleur, convertion")
		for p in range(len(z)/2):
			duree_seconde+=z[p*2+1]-z[p*2]
			draw.rectangle((fx(z[2*p]),sy-margey,fx(z[p*2+1]),margey),fill="red")			
		print("duree totale; ",duree_seconde)		
#Ligne brisée supérieure
		p=0
		while (p<nbpoint-2):
			d=fx2(fx(x[p])+1)
			p2=p+1
			while (p2<nbpoint-1 and x[p2]<d):
				p2=p2+1
			if x[p2]>=d:
				draw.polygon([fx(x[p]),margey,fx(x[p]),fy(y[p])-1,fx(x[p2]),fy(y[p2])-1,fx(x[p2]),margey],fill="lightgrey")
			p=p2		


#graduation des ordonnées
		if grad_y!=0:
			gy=round_sup(starty)
			while gy<=endy:
				draw.line((margex-3,fy(gy),margex+3,fy(gy)),fill="black")
				draw.line((margex+3,fy(gy),sx-margex,fy(gy)),fill="grey")
				draw.text((margex-30,fy(gy)-5),unicode(str(gy),'UTF-8'), font=font,fill='black')
				gy=gy+grad_y
			draw.line((margex-3,fy(endy),margex+3,fy(endy)),fill="black")
		
		
#graduations des abscisses
		if grad_heures:
			initx=time.localtime(startx)
			initx=(initx.tm_year,initx.tm_mon,initx.tm_mday,initx.tm_hour,0,0,0,0,1)
			initx=int(time.mktime(initx)+3600)
			for gx in range(initx, int(endx),3600):
				draw.line((fx(gx),sy-margey+3,fx(gx),sy-margey-3),fill="black")							
				if (time.localtime(gx).tm_hour)==12:
					draw.text((fx(gx)-5,sy-margey+5), "12h", font=font,fill='black')
	
		
		if grad_jours:
			initx=time.localtime(startx)
			initx=(initx.tm_year,initx.tm_mon,initx.tm_mday,0,0,0,0,0,1)
			initx=int(time.mktime(initx)+86400)
			for gx in range(initx, int(endx),86400):
				draw.line((fx(gx),sy-margey+7,fx(gx),sy-margey-7),fill="red")
				draw.text((fx(gx)-10,sy-margey+15), unicode(str(time.localtime(gx).tm_mday)+"/"+str(time.localtime(gx).tm_mon),'UTF-8'), font=font,fill='black')	
		
				
#axex
		draw.line((margex,sy-margey,sx-margex,sy-margey),fill="black")

		draw.line((sx-margex,sy-margey+3,sx-margex,sy-margey-3),fill="black")
		draw.line((sx-margex,sy-margey+3,sx-margex+6,sy-margey),fill="black")
		draw.line((sx-margex,sy-margey-3,sx-margex+6,sy-margey),fill="black")
		
#axey
		draw.line((margex,margey,margex,sy-margey),fill="black")		
		draw.line((margex-3,margey,margex+3,margey),fill="black")		
		draw.line((margex-3,margey,margex,margey-6),fill="black")		
		draw.line((margex+3,margey,margex,margey-6),fill="black")	
#legendes des axes	
		
		#draw.text((10,margey-5), unicode(str(round(endy,1))+"°C",'UTF-8'), font=font,fill='black')
		#draw.text((10,sy-margey-5), unicode(str(round(starty,1))+"°C",'UTF-8'), font=font,fill='black')
	
		#draw.text((margex-10,sy-margey+5), unicode(str(time.localtime(startx).tm_hour)+"h"+str(time.localtime(startx).tm_min),'UTF-8'), font=font,fill='black')
		#draw.text((margex-10,sy-margey+15), unicode(str(time.localtime(startx).tm_mday)+"/"+str(time.localtime(startx).tm_mon),'UTF-8'), font=font,fill='black')
	
		#draw.text((sx-margex-10,sy-margey+5), unicode(str(time.localtime(endx).tm_hour)+"h"+str(time.localtime(endx).tm_min),'UTF-8'), font=font,fill='black')	
		#draw.text((sx-margex-10,sy-margey+15), unicode(str(time.localtime(endx).tm_mday)+"/"+str(time.localtime(endx).tm_mon),'UTF-8'), font=font,fill='black')	
		
#Ligne brisée 
		p=0
		while (p<nbpoint-2):
			d=fx2(fx(x[p])+1)
			p2=p+1
			while (p2<nbpoint-1 and x[p2]<d):
				p2=p2+1
			if x[p2]>=d:
				draw.line((fx(x[p]),fy(y[p]),fx(x[p2]),fy(y[p2])),fill="black")		
			p=p2
	
#affichage dure bruleur
		print(duree_seconde)
		duree_heure = int(duree_seconde /3600)
		duree_seconde %= 3600
		duree_minute = int(duree_seconde /60)
		duree_seconde %= 60
		draw.text((sx/2,10),unicode("Durée activité chaudière: "+str(int(duree_heure))+"h"+str(int(duree_minute))+"m"+str(int(duree_seconde))+"s",'UTF-8'), font=font,fill='blue')


		del draw
		mon_image = StringIO()
		im.save(mon_image,"png")
		return mon_image
	return None
	

def stop_chauffage():
	os.system("echo 1 > log/chaudiere.stop")
	time.sleep(10)
	os.system("sudo kill chaudiere")
	os.system("rm log/chaudiere.stop")
	set_gpio_out(get_pin_gpio(),GPIO.HIGH)


def stop_log_temperature():
	os.system("echo 1 > log/temperature.stop")
	time.sleep(10)
	os.system("sudo kill temperature")
	os.system("rm log/temperature.stop")

def launch_newprgm(prgm):
	os.system('echo "'+prgm+'" > log/chaudiere.prgm');
	
	


def get_pin_gpio():
	tree = ET.parse('config.xml')
	root = tree.getroot()
	pin=root.find("pin_gpio")
	if pin!=None:
		try: p=int(pin.text)
		except ValueError:
			p=None
	return p


def reboot_if_low_memory(reboot):
	with open('/proc/meminfo', 'r') as mem:
		ret = {}
		tmp = 0
		for i in mem:
			sline = i.split()
			if str(sline[0]) == 'MemTotal:':
				ret['total'] = int(sline[1])
			elif str(sline[0]) in ('MemFree:', 'Buffers:', 'Cached:'):
				tmp += int(sline[1])
		ret['free'] = tmp
	if reboot==True:
		if float(ret['free'])/float(ret['total'])<0.2:
			stop_log_temperature()
			termine_chauffage()
#			log_activite("reboot à cause de la memoire trop faibe")
			time.sleep(2)
			os.system("sudo reboot")
			exit(-1)
	else:
		print(ret) 

def get_temperatures_defaut():
	tree = ET.parse('config.xml')
	root = tree.getroot()
	th=root.find("temperature_haute")
	tl=root.find("temperature_basse")
	if th==None or tl==None:
		print("problème temperature par defaut")
#		log_activite("probleme avec temperature par defaut dans config.xml")
		return None
	else:
		return [tl.text,th.text]
	
def termine_services():
	os.system("echo 1 > log/chaudiere.stop")
	os.system("echo 1 > log/temperature.stop")
	time.sleep(10)
	os.system("sudo cp log/log_temperature.log ../")
	os.system("sudo cp log/log_bruleur.log ../")
	os.system("sudo kill chaudiere")
	os.system("rm log/chaudiere.stop")
	os.system("sudo kill temperature")
	os.system("rm log/temperature.stop")
	initialise_gpio(get_pin_gpio())
	set_gpio_out(get_pin_gpio(),GPIO.HIGH)
	GPIO.setup(get_pin_gpio(), GPIO.IN)
	GPIO.cleanup()
	return 
	
def get_detail_programme():
	semaine=["lundi","mardi","mercredi","jeudi","vendredi","samedi","dimanche"]
	tree = ET.parse('programmes.xml')
	root = tree.getroot()
	programme=[]
	horaire_found=False
	listep=liste_programmes()
	for nom in listep:
#recherche le programme actif
		programme.append (nom)
		find=None
		for p in root.iter('programme'):
			if p.get('nom')==nom:
				find=p
				break
		if find==None:
			return None
		tmpl=p.find("temperature_basse")
		tmph=p.find("temperature_haute")
		[tl,th]=get_temperatures_defaut()
		if tmph!=None:
			th=tmph.text
		if tmpl!=None:
			tl=tmpl.text
		if tl!=None and th!=None:		
			programme.append ("Temperature de confort: "+th)
			programme.append ("Temperature minimale: "+tl)
		else:
			return None
	
		horaire_found=False
		for l in p:
			if l.tag=="jour":
				j=l.find("heures_hautes")
				if j!=None:
					if horaire_found==False:
						programme.append("Horaires de confort")
					horaire_found=True
					programme.append([l.get("nom"),j.text])
		if horaire_found==False:
			programme.append("Aucune plage de confort")
		programme.append(" ")
		programme.append("#################################")
		programme.append(" ")
	return programme
	
def programme_actif():
	if os.path.exists("log/prog_actif"):
		fichier=open("log/prog_actif")
		nom=fichier.readline()
		fichier.close()
	else:
		nom=None
	return nom
	
	
	

def liste_programmes():
	tree = ET.parse('programmes.xml')
	root = tree.getroot()
	prgm=[]
	for p in root.iter('programme'):
		prgm.append(p.get('nom'))
	return prgm

def demarrage_auto():
	tree = ET.parse('config.xml')
	root = tree.getroot()
	autostart=root.find("autostart")
	if autostart!=None:
		if autostart.text=="1":
			return True
	return False

def get_programme_defaut():
	tree = ET.parse('config.xml')
	root = tree.getroot()
	prg_def=root.find("programme_defaut")
	if prg_def!=None:
		return prg_def.text
	else:
		return None 



def initialise_gpio(pin):
	GPIO.setmode(GPIO.BOARD)
	GPIO.setup(pin, GPIO.OUT, initial=GPIO.HIGH)

def set_gpio_out(pin,etat_out):
	if GPIO.getmode()!=GPIO.BOARD:
#		log_activite("le système d'adressage des pin GPIO a changé")
		GPIO.setmode(GPIO.BOARD)
	if  GPIO.gpio_function(pin)!=GPIO.OUT:
#		log_activite("le parametrage GPIO n'est plus le bon, rereglage ")
		GPIO.setup(pin, GPIO.OUT, initial=GPIO.LOW)
	if  etat_out!=GPIO.HIGH and etat_out!=GPIO.LOW:
		etat_out=GPIO.LOW
#		log_activite("erreur de paramete etat_out dans set_gpio_out") 
	GPIO.output(pin,etat_out)

def get_sonde():
	tree = ET.parse('config.xml')
	root = tree.getroot()
	sonde=root.find("sonde")
	if sonde!=None:
		return sonde.text
	else:
		return None

def get_temperature(sonde):
	if os.path.exists(sonde) and sonde!=None:
		fichier=open(sonde,'r')
		text=fichier.read()
		fichier.close()
		secondline = text.split("\n")[1]
		temperaturedata = secondline.split(" ")[9]
		temperature = float(temperaturedata[2:])
		temperature = temperature / 1000
		return temperature
	else:
		return None

def log_activite(str_log):
#	fichier=open("log/journal.log",'a')
#	temps=time.localtime()
#	fichier.write("\n"+ str(temps.tm_mday)+"/"+str(temps.tm_mon)+": "+str(temps.tm_hour)+"h"+str(temps.tm_min)+": "+str_log)
#	fichier.close()
	return	

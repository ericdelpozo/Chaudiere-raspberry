#include <libxml/parser.h>
#include <libxml/tree.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <wiringPi.h>


/*
 *To compile this file using gcc you can type
 *gcc `xml2-config --cflags --libs` -o flile.out file.c
 */

//valeurs par défaut remplacées eventuellement par le programme choisi
float temp_inf_defaut=15.0;  //temperature basse par defaut
float temp_sup_defaut=21.0;  //temperature haute par defaut

char path_capteur[100];
const char * path_log="log/log_bruleur.log";
float marge_temp=1.0;



/* 0 --> temperature basse  1 signifie que c'est une heure de temperature haute  */
//  contient les horaire des jours de la semaine
int pin_relai=-1;// par défaut pas initialisé



struct prgm_spec{
char nom_programme[50];
float temp_inf;
float temp_sup;
int heures[7][24];
};

struct prgm_spec prog_actif;

int GetDS18B20(float * temperature)
{
char fname[1024];
char buffer[1024];
int  fhandle;
int nread=0;
char * pointeur;
int rcode;
int itemp;

  *temperature=0.0;
  fhandle = open(path_capteur,O_RDONLY);
  if(fhandle<0) return 0;
  nread= read(fhandle,buffer,sizeof(buffer));
  close(fhandle);
if((nread>0)  && (nread<1024))
    {
      // force dernier octet a etre null , au cas ou
      buffer[nread]=0;
     // vérifie si CRC est ok . Regarde pour YES
     pointeur = strstr(buffer,"YES");
     if(pointeur==NULL)
      return 2;
     // ok trouve le terme t=
     pointeur =strstr(buffer,"t=");
  if(pointeur == NULL)
      return 3;
     pointeur+=2;
     // converti la valeur ascii en entier
     rcode=sscanf(pointeur,"%d",&itemp);
     if(rcode==0) return 0;
     // converti la valeur en point flottant
     *temperature = (float) itemp / 1000.0;
     return 1;
  }
  return(0);
}




xmlNode* find_first_element(xmlNode * start_node,const char *nom);
int is_heure(char *s);

xmlNode* find_programme(xmlDocPtr doc,  const char *nom_prg)
{
	xmlNode *programmes=NULL;
 	xmlNode *prog=NULL;
	xmlNode *cur_node;
	xmlChar *p;
  	if (doc==NULL) {return (NULL);}
	programmes=find_first_element(xmlDocGetRootElement(doc),"programme");
	if (programmes==NULL){return (NULL);}
    	for (cur_node = programmes; cur_node; cur_node = cur_node->next) 
	{
        	if (cur_node->type == XML_ELEMENT_NODE) 
		{
            		p=xmlGetProp(cur_node,"nom");
        		if (p!=NULL)
        		{
        			if (strcmp(p,nom_prg)==0)
				{
					printf("trouvé: %s",p);
					xmlFree(p);
					return (cur_node);
				}
	        	xmlFree(p);
        		}
        	}
    	}
	return (NULL);
}


xmlNode* find_first_element(xmlNode * start_node,const char *nom)
{
	xmlNode *cur_node = NULL;
	xmlNode *recherche;
	if (start_node==NULL) {return NULL;}
    for (cur_node = start_node; cur_node; cur_node = cur_node->next) 
{
        if (cur_node->type == XML_ELEMENT_NODE) 
	{
    		if (strcmp(cur_node->name,nom)==0)
		{
		return (cur_node);
		}
	}
	recherche=find_first_element(cur_node->children,nom);
        if (recherche!=NULL) {return (recherche);}
}
return (NULL);
}

void print_child_names(xmlNode *a_node, char *prop)
{
 xmlNode *cur_node = NULL;
 xmlChar *p;
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            printf("node type: Element, name: %s\n", cur_node->name);
	p=xmlGetProp(cur_node,prop);
	if (p!=NULL)
	{
	printf("attribut %s: %s",prop,p);
	xmlFree(p);
	}
        }
    }

}

void clean_strn(char *dest,  char *source,int max)
{
	int i=0;
	char *pos=source;
	if (source==NULL) {return;}
	while (*pos!='\0' && i<max)
	{
		if (*pos!=' ')
		{
			dest[i]=*pos;
			i++;
		}
		pos++;
	}
	dest[i]='\0';
	return;
}

int nb_car(char *source,char car)
{
	int i=0;
	int j=0;
	if (source==NULL) return 0;
	for (i=0;i<strlen(source);i++)  {if (source[i]==car) {j=j+1;}}
	return j;
}

//teste si la chaine est convertible en numérique float.
int is_num(char *s)
{
	int contient_point=0;	
	int contient_des_zeros=0;
	int nb_points,nb_zeros;
	int long_s2;
	float conv;
	if (s==NULL) {return 0;}
	char *s2=malloc(strlen(s)+1);
	clean_strn(s2,s,strlen(s)+1);
	long_s2=strlen(s2);
	if (long_s2==0) {free(s2);return 0;}
	nb_points=nb_car(s2,'.');
	nb_zeros=nb_car(s2,'0');
	conv=atof(s2);
	free(s2);
	if (conv!=0) {return 1;}
	else { if (nb_points<=1 && long_s2==nb_zeros+nb_points ) {return 1;} else {return 0;}}
	return 0;
}

int is_int(char *s)
{
return (is_num(s) && nb_car(s,'.')==0);
}

int is_heure(char *s)
{
	int h;
	if (!is_int(s)) {return (-1);}
	h=atoi(s);
	return (h>=0 && h<=23);
}	

/*
	parse le fichier programme.xml
	recherche du programme 
	lecture des elements du programme
	retourne -1 en cas d'erreur
*/

int init( char *nom_programme)
{
	struct prgm_spec prog;
	prog.temp_inf=temp_inf_defaut;
	prog.temp_sup=temp_sup_defaut;
	strncpy(prog.nom_programme,nom_programme,49);
	prog.nom_programme[49]='\0';
	int i,j;
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	xmlNode *trouve=NULL;
	xmlNode *cur_node=NULL;
	xmlChar *p=NULL;
	int indice_jour=0;
	float try_conv;
	char *token=NULL;
	char heure[4];
	xmlChar *plages;
	int h;
	for (i=0;i<7;i++)
	{
		for (j=0;j<24;j++)
		{
			prog.heures[i][j]=0;
		}
	}
	doc = xmlReadFile("programmes.xml", NULL, 0);
	if (doc == NULL) 
    	{
		printf("problème avec programmes.xml");
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return (-1);    
	}
	trouve=find_programme(doc,nom_programme);
	if (trouve==NULL)
	{
		printf("ne trouve pas le programme %s dans programme.xml",nom_programme);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return (0);
	}
	for (cur_node = trouve->children; cur_node; cur_node = cur_node->next)
	{
        	if (cur_node->type == XML_ELEMENT_NODE)
		{
	        	if (strcmp(cur_node->name,"temperature_basse")==0)
			{
				if (is_num(xmlNodeGetContent(cur_node)))
				{
				prog.temp_inf=atof(xmlNodeGetContent(cur_node));
				}
			}
			if (strcmp(cur_node->name,"temperature_haute")==0)
			{
 				if (is_num(xmlNodeGetContent(cur_node)))
				{
				prog.temp_sup=atof(xmlNodeGetContent(cur_node));
				}
			}
			if (strcmp(cur_node->name,"jour")==0)
			{
				p=xmlGetProp(cur_node,"nom");
				if (p!=NULL)
				{
					indice_jour=-1;
					if (strcmp(p,"lundi")==0) {indice_jour=1;}
					if (strcmp(p,"mardi")==0) {indice_jour=2;}
					if (strcmp(p,"mercredi")==0) {indice_jour=3;}
					if (strcmp(p,"jeudi")==0) {indice_jour=4;}
					if (strcmp(p,"vendredi")==0) {indice_jour=5;}
					if (strcmp(p,"samedi")==0) {indice_jour=6;}
					if (strcmp(p,"dimanche")==0) {indice_jour=0;}
					if (indice_jour>=0)
					{
						xmlNode *heures_hautes=find_first_element(cur_node->children,"heures_hautes");
						plages=xmlNodeGetContent(heures_hautes);
					//	printf("plage:%s:  %s\n ",p,plages);
						if (plages!=NULL)
						{
							token = strtok(plages, ",");
						//	printf("\nheures hautes du %s: ",p);
							while( token != NULL )
							{
								//printf("token : %s\n",token);
								if (is_heure(token))
						      		{
									clean_strn(heure,token,3);
									h=atoi(heure);
									printf(":%d",h);
									prog.heures[indice_jour][h]=1;
								}
								token = strtok(NULL, ",");
							}
							xmlFree(plages);
						}
					}
				}
				xmlFree(p);
			}
    		}
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
	memcpy(&prog_actif,&prog,sizeof(prog_actif));
	return (1);
}


int  prog_defaut_enabled()
{
        xmlDoc *doc = NULL;
        xmlNode *autostart=NULL;
        xmlChar *p=NULL;
        doc = xmlReadFile("config.xml", NULL, 0);
        if (doc == NULL) 
        {
                printf("problème avec config.xml");
                xmlFreeDoc(doc);
                xmlCleanupParser();
				return 0;
	    }
		autostart=find_first_element(xmlDocGetRootElement(doc),"autostart");
        if (autostart==NULL) {xmlFreeDoc(doc);xmlCleanupParser();return 0;}
        else
        {
                p=xmlNodeGetContent(autostart);
                int ret=(strcmp(p,"1")==0);
				xmlFree(p);xmlFreeDoc(doc);xmlCleanupParser();return ret;
		}
}

int get_pin(void)
{
		xmlDoc *doc = NULL;
        xmlNode *xmlpin=NULL;
        xmlChar *p=NULL;
        int pingpio=-1;
        doc = xmlReadFile("config.xml", NULL, 0);
        if (doc == NULL)
        {
                printf("problème avec config.xml");
                xmlFreeDoc(doc);
                xmlCleanupParser();
              	return -1;
        }
	xmlpin=find_first_element(xmlDocGetRootElement(doc),"pin_gpio");
	if (xmlpin==NULL) {xmlFreeDoc(doc);xmlCleanupParser();return -1;}
	else{
			p=xmlNodeGetContent(xmlpin);
			if (p!=NULL)
			{
				char pp[10];
				clean_strn(pp,p,10);
				if (is_int(pp))
				{
					pingpio=atoi(pp);
					if (pingpio<0 || pingpio>16) {pingpio=-1;}
				}
				else {pingpio=-1;}
			}
			else {pingpio=-1;}
			xmlFree(p);
			xmlFreeDoc(doc);
			xmlCleanupParser();
			return pingpio;
	    }
}


int get_sonde(char* sonde)
{
		xmlDoc *doc = NULL;
        xmlNode *xmlpin=NULL;
        xmlChar *p=NULL;
        int pingpio=-1;
        doc = xmlReadFile("config.xml", NULL, 0);
        if (doc == NULL)
        {
                printf("problème avec config.xml");
                xmlFreeDoc(doc);
                xmlCleanupParser();
              	return -1;
        }
	xmlpin=find_first_element(xmlDocGetRootElement(doc),"sonde");
	if (xmlpin==NULL) {xmlFreeDoc(doc);xmlCleanupParser();return -1;}
	else{
			p=xmlNodeGetContent(xmlpin);
			if (p!=NULL)
			{
			strncpy(sonde,p,100);
			sonde[99]='\0';	
			}			
			else {sonde[0]='\0';}
			xmlFree(p);
			xmlFreeDoc(doc);
			xmlCleanupParser();
			
			return 	(access( sonde, F_OK ) != -1 );

	   }
}


//retourne -1 en cas d'erreur ou de valeur incirrecte.
float get_marge(void)
{
		xmlDoc *doc = NULL;
        xmlNode *xmlval=NULL;
        xmlChar *p=NULL;
        float val=-1.0;
        doc = xmlReadFile("config.xml", NULL, 0);
        if (doc == NULL)
        {
                printf("problème avec config.xml");
                xmlFreeDoc(doc);
                xmlCleanupParser();
              	return -1;
        }
	xmlval=find_first_element(xmlDocGetRootElement(doc),"seuil");
	if (xmlval==NULL) {xmlFreeDoc(doc);xmlCleanupParser();return -1;}
	else{
			p=xmlNodeGetContent(xmlval);
			if (p!=NULL)
			{
				char pp[10];
				clean_strn(pp,p,10);
				if (is_num(pp))
				{
					val=atof(pp);
					if (val<0 || val>10) {val=-1;}
				}
				else {val=-1;}
			}
			else {val=-1;}
			xmlFree(p);
			xmlFreeDoc(doc);
			xmlCleanupParser();
			return val;
	    }
}


int asked_stop()
{
return ( access( "log/chaudiere.stop", F_OK ) != -1 ) ;
}


int asked_newprgm()
{
return ( access( "log/chaudiere.prgm", F_OK ) != -1 ) ;
}

void get_newprgm(char *dest)
{
FILE* fichier= NULL ;
fichier = fopen("log/chaudiere.prgm", "r");
dest[0]='\0';

if (fichier!=NULL)
{
fgets(dest,50,fichier);
dest[49]='\0';
fclose(fichier);	
}
if (strlen(dest)>0)
{if (dest[strlen(dest)-1]=='\n') {dest[strlen(dest)-1]='\0';}  }
remove("log/chaudiere.prgm");
}


void write_prog_actif()
{
FILE* fichier= NULL ;
fichier = fopen("log/prog_actif", "w");
if (fichier!=NULL)
{
fprintf(fichier, "%s",prog_actif.nom_programme);
fclose(fichier);
}
}




void get_defaut_prgm_actif(char *nom)
{
	if (nom==NULL) {return ;}
		xmlDoc *doc = NULL;
        xmlNode *root_element = NULL;
        xmlNode *prog_defaut=NULL;
        xmlNode *cur_node=NULL;
        xmlChar *p=NULL;

		doc = xmlReadFile("config.xml", NULL, 0);
        if (doc == NULL)
        {
                printf("problème avec config.xml");
                xmlFreeDoc(doc);
                xmlCleanupParser();
                nom[0]='\0';
				return;
        }
        prog_defaut=find_first_element(xmlDocGetRootElement(doc),"programme_defaut");
	if (prog_defaut==NULL) {nom[0]='\0';xmlFreeDoc(doc);xmlCleanupParser();return;}
	else{
		p=xmlNodeGetContent(prog_defaut);
		if (p!=NULL)
		{
			strncpy(nom,p,49);
			nom[49]='\0';
			xmlFree(p);
			xmlFreeDoc(doc);
			xmlCleanupParser();
			return;	
		}

	    }
}


void start_time_count(time_t* t_start, int *started)
{
if (*started==0)
	{time(t_start);
	*started=1;
	}
}


void end_time_count(time_t start,int *started)
{
FILE* fichier=NULL;
time_t t;
if (*started==1)
{
	fichier = fopen(path_log, "a+");
	if (fichier!=NULL)
	{
		fprintf(fichier, "%d/%d\n", start ,time(&t) );
		fclose(fichier);
	}
	*started=0;
}
}
void chauffage()
{
	float temp;
	int started;
	time_t t,t_start;
	int d,h;
	float temperature_cible=prog_actif.temp_inf;
	struct tm instant;		
	wiringPiSetupPhys() ;
    pinMode (pin_relai,OUTPUT);
    char newp[50];
	newp[49]='\0';
	digitalWrite(pin_relai,HIGH);
	t_start=0;
	started=0;
	while (1)
	{
		if (asked_stop()){ printf("\nstop demandé"); remove("log/chaudiere.stop");digitalWrite(pin_relai,HIGH);end_time_count(t_start,&started);remove("log/prog_actif");return;}
		if (asked_newprgm()){printf("\nnew prgm demandé");get_newprgm(newp); printf("\n%s, %d",newp, strlen(newp)); if (init(newp)==1) {digitalWrite(pin_relai,HIGH);end_time_count(t_start,&started);printf("\nnouveau programme %s\n",newp); write_prog_actif(newp);} }
		sleep(5);
		if (GetDS18B20(&temp)==1)
		{
			time(&t);
			instant=*localtime(&t);
			d=instant.tm_wday;
			h=instant.tm_hour;
			temperature_cible=(prog_actif.heures[d][h]==1)? prog_actif.temp_sup:prog_actif.temp_inf;
			printf("\ntempérature actelle: %f, temperature cible:%f",temp,temperature_cible);
			if (temp<temperature_cible-marge_temp){
				digitalWrite(pin_relai,LOW);
				start_time_count(&t_start,&started);
				}
			if (temp>temperature_cible){
				digitalWrite(pin_relai,HIGH);
				end_time_count(t_start,&started);
				}	
		}
		else
		{
		digitalWrite(pin_relai,HIGH);
		end_time_count(t_start,&started);
		}
	}		
}



int main(int argc, char **argv)
{
	
FILE* fichier= NULL ;
char nom_programme[50];
int i=0;


if (get_sonde(path_capteur)!=1)
{
printf("La sonde indiquée dans config.xml n'est pas accessible");
return -1;
}

nom_programme[0]='\0';
marge_temp=get_marge();

if (marge_temp==-1) {marge_temp=1;}
pin_relai=get_pin();
if (pin_relai==-1) {printf("Erreur pin_gpio renseignée dans config_.xml");exit(-1);return -1;}
	

if (argc==2) {strncpy(nom_programme,argv[1],50);nom_programme[49]='\0';}
if (argc==1) {get_defaut_prgm_actif(nom_programme);}

while (1)
{
	if (strlen(nom_programme)!=0)
	{
		if (init(nom_programme)==1)
			{	write_prog_actif();
				printf("\nlancement du démon chauffage");
				chauffage();
				nom_programme[0]='\0';
				printf("\nsortie du demon chauffage");}
	}
	else
	{
		sleep(10);
		if (asked_newprgm())
		{
		printf("\nNouveau programme demandé");
		get_newprgm(nom_programme);
		}
	}
}
return 0;
}

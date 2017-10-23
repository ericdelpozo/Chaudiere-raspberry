#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

char path_capteur[100];
const char * path_log="log/log_temperature.log";
const int log_step=60;



xmlNode* find_first_element(xmlNode * start_node,const char *nom);

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


int get_sonde(char *sonde)
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

int asked_stop()
{
return ( access( "log/temperature.stop", F_OK ) != -1 ) ;
}


int main(int argc, char* argv[])
{
  int loop;
  float temp;
  int rcode;
  int p;
  
  FILE* fichier= NULL ;
   	time_t t;
	time_t t2;
	time(&t2);
	path_capteur[0]='0';
	if (get_sonde(path_capteur)!=1)
	{
	printf("La sonde indiquée dans config.xml n'est pas accessible");
	return -1;
	}
	
	while (1)
	{
		if (asked_stop()){remove("log/temperature.stop");exit(0);return 0;}
		sleep(5);
		time(&t);
		if(t-t2>log_step)
		{
			t2=t;
			rcode= GetDS18B20(&temp);
		       switch(rcode)
		       {
		      	case 1:
			fichier = fopen(path_log, "a+");
			if (fichier!=NULL)
			{
				fprintf(fichier, "%d/%6.2f\n", time(&t), temp);
				fclose(fichier);
			}
			break;
          		case 2: printf("--- Erreur CRC\n");break;
			case 3: printf("--- Valeur non trouvée\n");break;
			default: printf("---\n");

		       }
		}
	 }
  return(0);
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>  
#include "acsmx.h"


unsigned char text[512];


  int
MatchFound (unsigned id, int index, void *data) 
{
	
  //fprintf (stdout, "%s:%d index %d \n",__func__,__LINE__, index);
  fprintf (stdout, "%s\n", (char *) id);
  return 0;
}
void enter_cli(ACSM_STRUCT * acsm)
{
	char text[512]={0};
	char cmd[512]={0};
	int n;
	int r_size;
	int state;
	while(1)
	{
		printf("please input:\n");

		n = scanf("%s %s",&cmd,&text);

//		printf("n is %d,cmd %s text %s\n",n,cmd,text);
		if(n != 2)
		{
			if (errno != 0) {
               perror("scanf");
			}
			printf("TOL  text\n");
			printf("SUB  text\n");
			printf("nTOL  text\n");
			printf("exit  it\n");
			
			continue;
		}
		if (strcmp(cmd,"TOL") == 0)
		{
			acsmSearch (acsm, text, strlen (text), MatchFound,  (void *) 0, &state,1);
		}
		else if (strcmp(cmd,"SUB") == 0)
		{
			if(acsmSearch2(acsm, text, strlen(text)) !=0)
			{
				printf("find <%s>\n",text);
			}
		}
		else if (strcmp(cmd,"nTOL") == 0)
		{
			acsmSearch (acsm, text, strlen (text), MatchFound,  (void *) 0, &state,0);
			
		}
		else if (strcmp(cmd,"exit") == 0)
		{
			break;
		}
		else
		{
			printf("unknown cmd\n");
		}
	}
}
void usage(void)
{
	fprintf (stderr,"Usage: acsmx  key-1 key-2 ... key-n  -SUB -TOL \n");
		
	fprintf (stderr," -TOL : match the key from the article's top.\n");
	fprintf (stderr," -TOL : 只匹配从文章头开始的关键字。\n");
	fprintf (stderr," -SUB : source-article 是其中一个key[0-n]的内容\n");
     
      exit (0);
}
int main (int argc, char **argv) 
{
  int i, nocase = 0;
  int tol = 0;
  int sub = 0;
  int state;
  ACSM_STRUCT * acsm;
  
  
  
  
  acsm = acsmNew ();
  for (i = 1; i < argc; i++)    
  {

      acsmAddPattern (acsm, argv[i], strlen (argv[i]), nocase, 0, 0,
            argv[i], i-1);
    }
  acsmCompile (acsm);
  enter_cli(acsm);
  acsmFree (acsm);
  printf ("normal pgm end\n");
  return (0);
}
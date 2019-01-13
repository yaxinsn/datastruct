

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
  
#include "acsmx.h"


unsigned char text[512];


  int
MatchFound (unsigned id, int index, void *data) 
{
	
  fprintf (stdout, "%s:%d index %d \n",__func__,__LINE__, index);
  fprintf (stdout, "%s\n", (char *) id);
  return 0;
}

void usage(void)
{
	fprintf (stderr,"Usage: acsmx source-article key-1 key-2 ... key-n  -SUB -TOL \n");
		
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
  
  if (argc < 3)
  {
	 usage();
  }
  
  for (i = 1; i < argc; i++)
  {
    if (strcmp (argv[i], "-TOL") == 0)
      tol = 1;
	if (strcmp (argv[i], "-SUB") == 0)
      sub = 1;
  }
  
  strcpy (text, argv[1]);
  
  
  acsm = acsmNew ();
  for (i = 2; i < argc; i++)
    
    {
      if (argv[i][0] == '-')
    continue;
      acsmAddPattern (acsm, argv[i], strlen (argv[i]), nocase, 0, 0,
            argv[i], i - 2);
    }
  acsmCompile (acsm);
  if(sub == 0)
  {
		acsmSearch (acsm, text, strlen (text), MatchFound,  (void *) 0, &state,tol);
  }
  else
  {
		if(acsmSearch2(acsm, text, strlen(text)) !=0)
		{
			printf("find it.\n");
		}
  }
  acsmFree (acsm);
  printf ("normal pgm end\n");
  return (0);
}
/*
**
** $Id$
**
** Multi-Pattern Search Engine
**
** Aho-Corasick State Machine -  uses a Deterministic Finite Automata - DFA
**
** Copyright (C) 2002 Sourcefire,Inc.
** Marc Norton
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
**
**   Reference - Efficient String matching: An Aid to Bibliographic Search
**               Alfred V Aho and Margaret J Corasick
**               Bell Labratories
**               Copyright(C) 1975 Association for Computing Machinery,Inc
**
**   Implemented from the 4 algorithms in the paper by Aho & Corasick
**   and some implementation ideas from 'Practical Algorithms in C'
**
**   Notes:
**     1) This version uses about 1024 bytes per pattern character - heavy  on the memory.
**     2) This algorithm finds all occurrences of all patterns within a
**        body of text.
**     3) Support is included to handle upper and lower case matching.
**     4) Some comopilers optimize the search routine well, others don't, this makes all the difference.
**     5) Aho inspects all bytes of the search text, but only once so it's very efficient,
**        if the patterns are all large than the Modified Wu-Manbar method is often faster.
**     6) I don't subscribe to any one method is best for all searching needs,
**        the data decides which method is best,
**        and we don't know until after the search method has been tested on the specific data sets.
**
**
**  May 2002  : Marc Norton 1st Version
**  June 2002 : Modified interface for SNORT, added case support
**  Aug 2002  : Cleaned up comments, and removed dead code.
**  Nov 2,2002: Fixed queue_init() , added count=0
**
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "acsmx.h"
//#include "util.h"
#define MEMASSERT(p,s) if(!p){fprintf(stderr,"ACSM-No Memory: %s!\n",s);exit(0);}

#ifdef DEBUG_AC
static int max_memory = 0;
#endif

/*static void Print_DFA( ACSM_STRUCT * acsm );*/

/*
*
*/
static void *
AC_MALLOC (int n)
{
  void *p;
  //p = calloc (1,n);
  p = malloc(n);
#ifdef DEBUG_AC
  if (p)
    max_memory += n;
#endif
  return p;
}


/*
*
*/
static void
AC_FREE (void *p)
{
  if (p)
    free (p);
}


/*
*    Simple QUEUE NODE
*/
typedef struct _qnode
{
  int state;
   struct _qnode *next;
}
QNODE;

/*
*    Simple QUEUE Structure
*/
typedef struct _queue
{
  QNODE * head, *tail;
  int count;
}
QUEUE;

/*
*
*/
static void
queue_init (QUEUE * s)
{
  s->head = s->tail = 0;
  s->count = 0;
}


/*
*  Add Tail Item to queue
*/
static void
queue_add (QUEUE * s, int state)
{
  QNODE * q;
  if (!s->head)
    {
      q = s->tail = s->head = (QNODE *) AC_MALLOC (sizeof (QNODE));
      MEMASSERT (q, "queue_add");
      if(q){
        q->state = state;
        q->next = 0;
      }
    }
  else
    {
      q = (QNODE *) AC_MALLOC (sizeof (QNODE));
      MEMASSERT (q, "queue_add");
      if(q){
          q->state = state;
          q->next = 0;
          s->tail->next = q;
          s->tail = q;
      }
    }
  s->count++;
}


/*
*  Remove Head Item from queue
*/
static int
queue_remove (QUEUE * s)
{
  int state = 0;
  QNODE * q;
  if (s->head)
    {
      q = s->head;
      state = q->state;
      s->head = s->head->next;
      s->count--;
      if (!s->head)
      {
          s->tail = 0;
          s->count = 0;
      }
      AC_FREE (q);
    }
  return state;
}


/*
*
*/
static int
queue_count (QUEUE * s)
{
  return s->count;
}


/*
*
*/
static void
queue_free (QUEUE * s)
{
  while (queue_count (s))
    {
      queue_remove (s);
    }
}
#ifdef NEXTSTATE_ARRAY
int InitNextState(ACSM_STATETABLE* a)
{
    int i;
    int j = 0;
    a->__NextState = AC_MALLOC(sizeof(int)*ALPHABET_SIZE);
    if(a->__NextState == NULL)
        goto FAIL;

    memset(a->__NextState,0,ALPHABET_SIZE);

    return 0;
FAIL:

        AC_FREE(a->__NextState);
    return -1;
}

int FreeNextState(ACSM_STATETABLE* a)
{
    AC_FREE(a->__NextState);

    return 0;

}

void     SetNextState(ACSM_STATETABLE* a,unsigned char index, int v)
{
    int array_index = index%ALPHABET_SIZE;

    a->__NextState[array_index] =v;
}
int GetNextState(ACSM_STATETABLE* a, unsigned char index)
{

    int array_index = index%ALPHABET_SIZE;

    return a->__NextState[array_index];
}
#endif

/*
** Case Translation Table
*/
#if 0
static unsigned char xlatcase[256];

/*
*
*/
  static void
init_xlatcase ()
{
  int i;
  for (i = 0; i < 256; i++)
    {
      xlatcase[i] = (unsigned char)toupper (i);
    }
}

/*
*
*/
static inline void
ConvertCaseEx (unsigned char *d, unsigned char *s, int m)
{
  int i;

  for (i = 0; i < m; i++)
    {
  //    d[i] = xlatcase[s[i]];
        d[i] = s[i];
    }
}

#endif


/*
*
*/
static ACSM_PATTERN *
CopyMatchListEntry (ACSM_PATTERN * px)
{
  ACSM_PATTERN * p;
  p = (ACSM_PATTERN *) AC_MALLOC (sizeof (ACSM_PATTERN));
  MEMASSERT (p, "CopyMatchListEntry");
  if(p){
      memcpy (p, px, sizeof (ACSM_PATTERN));
      p->next = 0;
  }
  return p;
}


/*
*  Add a pattern to the list of patterns terminated at this state.
*  Insert at front of list.
*/
static void
AddMatchListEntry (ACSM_STRUCT * acsm, int state, ACSM_PATTERN * px)
{
  ACSM_PATTERN * p;
  p = (ACSM_PATTERN *) AC_MALLOC (sizeof (ACSM_PATTERN));
  MEMASSERT (p, "AddMatchListEntry");
  if(p){
      memcpy (p, px, sizeof (ACSM_PATTERN));
      p->next = acsm->acsmStateTable[state].MatchList;//error.
      acsm->acsmStateTable[state].MatchList = p;
  }
}


/*
   Add Pattern States
*/
static void
AddPatternStates (ACSM_STRUCT * acsm, ACSM_PATTERN * p)
{
  unsigned char *pattern;
  int state=0, next, n;
  n = p->n;
  pattern = p->patrn;

    /*
     *  Match up pattern with existing states
     */
    for (; n > 0; pattern++, n--)
    {

#ifndef NEXTSTATE_ARRAY
      next = acsm->acsmStateTable[state].NextState[*pattern];
#else
    next = GetNextState(&acsm->acsmStateTable[state], *pattern);
#endif
      if (next == ACSM_FAIL_STATE)
        break;
      state = next;
    }

    /*
     *   Add new states for the rest of the pattern bytes, 1 state per byte
     */
    for (; n > 0; pattern++, n--)
    {
      acsm->acsmNumStates++;

#ifndef NEXTSTATE_ARRAY
      acsm->acsmStateTable[state].NextState[*pattern] = acsm->acsmNumStates;
#else
    SetNextState(&acsm->acsmStateTable[state], *pattern, acsm->acsmNumStates);
#endif
      state = acsm->acsmNumStates;
    }

  AddMatchListEntry (acsm, state, p);
}


/*
*   Build Non-Deterministic Finite Automata
*/
static void
Build_NFA (ACSM_STRUCT * acsm)
{
  int r, s;
  int i;
  QUEUE q, *queue = &q;
  ACSM_PATTERN * mlist=0;
  ACSM_PATTERN * px=0;

    /* Init a Queue */
    queue_init (queue);

    /* Add the state 0 transitions 1st */
    for (i = 0; i < ALPHABET_SIZE; i++)
    {

#ifndef NEXTSTATE_ARRAY
      s = acsm->acsmStateTable[0].NextState[i];
#else
    s = GetNextState (&acsm->acsmStateTable[0],i);
#endif
      if (s)
      {
        queue_add (queue, s);
        acsm->acsmStateTable[s].FailState = 0;
      }
    }

    /* Build the fail state transitions for each valid state */
    while (queue_count (queue) > 0)
    {
      r = queue_remove (queue);

      /* Find Final States for any Failure */
      for (i = 0; i < ALPHABET_SIZE; i++)
      {
        int fs, next;
#ifndef NEXTSTATE_ARRAY
        if ((s = acsm->acsmStateTable[r].NextState[i]) != ACSM_FAIL_STATE)
#else
        if ((s = GetNextState(&acsm->acsmStateTable[r],i)) != ACSM_FAIL_STATE)
#endif
        {
          queue_add (queue, s);
          fs = acsm->acsmStateTable[r].FailState;

          /*
           *  Locate the next valid state for 'i' starting at s
           */
#ifndef NEXTSTATE_ARRAY
         while ((next=acsm->acsmStateTable[fs].NextState[i]) ==
                 ACSM_FAIL_STATE)
#else
        while ((next=GetNextState(&acsm->acsmStateTable[fs],i)) ==
                 ACSM_FAIL_STATE)
#endif
          {
            fs = acsm->acsmStateTable[fs].FailState;
          }

          /*
           *  Update 's' state failure state to point to the next valid state
           */
          acsm->acsmStateTable[s].FailState = next;

          /*
           *  Copy 'next'states MatchList to 's' states MatchList,
           *  we copy them so each list can be AC_FREE'd later,
           *  else we could just manipulate pointers to fake the copy.
           */
          for (mlist  = acsm->acsmStateTable[next].MatchList;
               mlist != NULL ;
               mlist  = mlist->next)
          {
              px = CopyMatchListEntry (mlist);

              if( !px )
              {
               // FatalError
		printf("*** Out of memory Initializing Aho Corasick in acsmx.c ****");
              }

              /* Insert at front of MatchList */
              px->next = acsm->acsmStateTable[s].MatchList;
              acsm->acsmStateTable[s].MatchList = px;
          }
        }
      }
    }

    /* Clean up the queue */
    queue_free (queue);
}


/*
*   Build Deterministic Finite Automata from NFA
*/
static void
Convert_NFA_To_DFA (ACSM_STRUCT * acsm)
{
  int r, s;
  int i;
  QUEUE q, *queue = &q;

    /* Init a Queue */
    queue_init (queue);

    /* Add the state 0 transitions 1st */
    for (i = 0; i < ALPHABET_SIZE; i++)
    {
#ifndef NEXTSTATE_ARRAY
      s = acsm->acsmStateTable[0].NextState[i];
#else
      s = GetNextState(&acsm->acsmStateTable[0],i);
#endif
      if (s)
      {
        queue_add (queue, s);
      }
    }

    /* Start building the next layer of transitions */
    while (queue_count (queue) > 0)
    {
      r = queue_remove (queue);

      /* State is a branch state */
      for (i = 0; i < ALPHABET_SIZE; i++)
      {
#ifndef NEXTSTATE_ARRAY
        if ((s = acsm->acsmStateTable[r].NextState[i]) != ACSM_FAIL_STATE)
#else
        if((s = GetNextState(&acsm->acsmStateTable[r],i)) != ACSM_FAIL_STATE)
#endif
        {
            queue_add (queue, s);
        }
        else
        {
#ifndef NEXTSTATE_ARRAY
            acsm->acsmStateTable[r].NextState[i] =
            acsm->acsmStateTable[acsm->acsmStateTable[r].FailState].
			NextState[i];
#else

            SetNextState(&acsm->acsmStateTable[r], i,
                GetNextState(&acsm->acsmStateTable[acsm->acsmStateTable[r].FailState],i));
#endif

        }
      }
    }

    /* Clean up the queue */
    queue_free (queue);
}


/*
*
*/
ACSM_STRUCT * acsmNew ()
{
  ACSM_STRUCT * p;
 // init_xlatcase ();
  p = (ACSM_STRUCT *) AC_MALLOC (sizeof (ACSM_STRUCT));
  MEMASSERT (p, "acsmNew");
  if (p)
    memset (p, 0, sizeof (ACSM_STRUCT));
  return p;
}


/*
*   Add a pattern to the list of patterns for this state machine
*/
int
acsmAddPattern (ACSM_STRUCT * p, unsigned char *pat, int n, int nocase,
            int offset, int depth, void * id, int iid)
{
  ACSM_PATTERN * plist;
  plist = (ACSM_PATTERN *) AC_MALLOC (sizeof (ACSM_PATTERN));
  MEMASSERT (plist, "acsmAddPattern");
  if(plist){
#ifndef __CASE__
  plist->patrn = (unsigned char *) AC_MALLOC (n);
  ConvertCaseEx (plist->patrn, pat, n);
  plist->nocase = nocase;
  plist->casepatrn = (unsigned char *) AC_MALLOC (n);
  memcpy (plist->casepatrn, pat, n);
  plist->id = plist->casepatrn; //具体这个id为安装时，关键字对应的value信息。--刘丹
#else
    plist->patrn = (unsigned char *) AC_MALLOC (n);
    if(!plist->patrn)
	  		goto error;
    memcpy (plist->patrn, pat, n);
	plist->nocase = 0;
	plist->id = plist->patrn; //具体这个id为安装时，关键字对应的value信息。--刘丹
#endif

  plist->n = n;

  plist->offset = offset;
  plist->depth = depth;
//  plist->id = id;
  plist->iid = iid;
  plist->next = p->acsmPatterns;
  p->acsmPatterns = plist;
  }
  else
    return -1;

  return 0;
error:
	AC_FREE(p);
	return -1;
}


/*
*   Compile State Machine
*/
int
acsmCompile (ACSM_STRUCT * acsm)
{
    int i, k;
    ACSM_PATTERN * plist;

    /* Count number of states */
    acsm->acsmMaxStates = 1;
    for (plist = acsm->acsmPatterns; plist != NULL; plist = plist->next)
    {
        acsm->acsmMaxStates += plist->n;
    }
    acsm->acsmStateTable =
        (ACSM_STATETABLE *) AC_MALLOC (sizeof (ACSM_STATETABLE) *
                                        acsm->acsmMaxStates);
    MEMASSERT (acsm->acsmStateTable, "acsmCompile");


    if(!acsm->acsmStateTable)
        return -1;
    memset (acsm->acsmStateTable, 0,
        sizeof (ACSM_STATETABLE) * acsm->acsmMaxStates);
#ifdef NEXTSTATE_ARRAY
    for(i = 0;i<acsm->acsmMaxStates;i++)
    {
        if(-1 == InitNextState(&acsm->acsmStateTable[i]))
            goto FAIL;
    }
#endif
    /* Initialize state zero as a branch */
    acsm->acsmNumStates = 0;

    /* Initialize all States NextStates to FAILED */
    for (k = 0; k < acsm->acsmMaxStates; k++)
    {
        for (i = 0; i < ALPHABET_SIZE; i++)
        {
#ifndef NEXTSTATE_ARRAY
            acsm->acsmStateTable[k].NextState[i] = ACSM_FAIL_STATE;
#else
            SetNextState(&acsm->acsmStateTable[k], i, ACSM_FAIL_STATE);
#endif
        }
    }

    /* Add each Pattern to the State Table */
    for (plist = acsm->acsmPatterns; plist != NULL; plist = plist->next)
    {
        AddPatternStates (acsm, plist);
    }

    /* Set all failed state transitions to return to the 0'th state */
    for (i = 0; i < ALPHABET_SIZE; i++)
    {
#ifndef NEXTSTATE_ARRAY
        if (acsm->acsmStateTable[0].NextState[i] == ACSM_FAIL_STATE)
#else
        if(GetNextState(&acsm->acsmStateTable[0],i) == ACSM_FAIL_STATE)
#endif
        {
#ifndef NEXTSTATE_ARRAY
            acsm->acsmStateTable[0].NextState[i] = 0;
#else
            SetNextState(&acsm->acsmStateTable[0],i,0);
#endif
        }
    }

    /* Build the NFA  */
    Build_NFA (acsm);

    /* Convert the NFA to a DFA */
    Convert_NFA_To_DFA (acsm);

    /*
      printf ("ACSMX-Max Memory: %d bytes, %d states\n", max_memory,
        acsm->acsmMaxStates);
     */

    //Print_DFA( acsm );

    return 0;
#ifdef NEXTSTATE_ARRAY

FAIL:
    for(i = 0;i<acsm->acsmMaxStates;i++)
    {
        FreeNextState(&acsm->acsmStateTable[i]);
    }
    return -2;
#endif
}


//static unsigned char Tc[64*1024];

/*
*   Search Text or Binary Data for Pattern matches
*/
int
acsmSearch (ACSM_STRUCT * acsm, unsigned char *Tx, int n,
            int (*Match) (void *  id, int index, void *data), void *data,
            int* current_state ,int tol)
{
    int state = 0;
    ACSM_PATTERN * mlist;
    unsigned char *Tend;
    ACSM_STATETABLE * StateTable = acsm->acsmStateTable;
    int nfound = 0;
    unsigned char *T;
    int index;
 #ifndef __CASE__
    /* Case conversion */
    ConvertCaseEx (Tc, Tx, n);
    T = Tc;

#else
	T = Tx;
#endif
	Tend = T + n;
    if ( !current_state )
    {
        return 0;
    }

//    state = *current_state;

    for (; T < Tend; T++)
    {
#ifndef NEXTSTATE_ARRAY
        state = StateTable[state].NextState[*T];
#else
        state = GetNextState(&StateTable[state],*T);
#endif
	     if(tol &&state == 0) //V4.2 add it .当发现state =0，则关键字已经不在源的TOP了。
		     return 0;

        if( StateTable[state].MatchList != NULL )
        {
            for( mlist=StateTable[state].MatchList; mlist!=NULL;
                 mlist=mlist->next )
            {
#ifndef __CASE__
                index = T - mlist->n + 1 - Tc;
                if( mlist->nocase )
#else
				index = T - mlist->n + 1 - Tx;
#endif
                {
                    nfound++;
                    {
                        if (Match (mlist->id, index, data) > 0)
                        {
                            *current_state = state;
                            return nfound;
                        }
                    }

                }
#ifndef __CASE__
                else
                {

                    if( memcmp (mlist->casepatrn, Tx + index, mlist->n) == 0 )
                    {
                        nfound++;
                        if (Match (mlist->id, index, data) > 0)
                        {
                            *current_state = state;
                            return nfound;
                        }
                    }
                }
#endif
            }
        }
    }
    *current_state = state;
    return nfound;
}

int
acsmSearch2 (ACSM_STRUCT * acsm, unsigned char *Tx,int len)
{
    int state = 0;
    int NextState = 0;
    ACSM_PATTERN * mlist;
    unsigned char *Tend;
    ACSM_STATETABLE * StateTable = acsm->acsmStateTable;
    int nfound = 0;
    unsigned char *T;
    int index;

	T = Tx;

	Tend = T + len;

    for (; T < Tend; T++)
    {
#ifndef NEXTSTATE_ARRAY
        state = StateTable[state].NextState[*T];
#else
        state = GetNextState(&StateTable[state],*T);
#endif
		if(state == 0)
			break;
    }
	if((T-Tx) == len)
		return 1;
	else
		return 0;
    //return T-Tx;
}

/*
*   Free all memory
*/
void
acsmFree (ACSM_STRUCT * acsm)
{
    int i;
    ACSM_PATTERN * mlist, *ilist;
    for (i = 0; i < acsm->acsmMaxStates; i++)
    {
        mlist = acsm->acsmStateTable[i].MatchList;
#ifdef NEXTSTATE_ARRAY
        FreeNextState(&acsm->acsmStateTable[i]);
#endif
        while (mlist)
        {
            ilist = mlist;
            mlist = mlist->next;
            AC_FREE (ilist);
        }
    }
    AC_FREE (acsm->acsmStateTable);
    mlist = acsm->acsmPatterns;
    while(mlist)
    {
        ilist = mlist;
        mlist = mlist->next;
#ifndef __CASE__
        AC_FREE(ilist->casepatrn);

#endif
		AC_FREE(ilist->patrn);
        AC_FREE(ilist);
    }
    AC_FREE (acsm);
}
void acsmLoopMlistUserData (ACSM_STRUCT * acsm, int (*freeUser) (void * dataNeedFree))
{
    ACSM_PATTERN * mlist, *ilist;

    mlist = acsm->acsmPatterns;
    while(mlist)
    {
        ilist = mlist;
        mlist = mlist->next;
        freeUser(ilist->id);

    }
}

/*
 *
 */
/*
static void Print_DFA( ACSM_STRUCT * acsm )
{
    int k;
    int i;
    int next;

    for (k = 0; k < acsm->acsmMaxStates; k++)
    {
      for (i = 0; i < ALPHABET_SIZE; i++)
    {
      next = acsm->acsmStateTable[k].NextState[i];

      if( next == 0 || next ==  ACSM_FAIL_STATE )
      {
           if( isprint(i) )
             printf("%3c->%-5d\t",i,next);
           else
             printf("%3d->%-5d\t",i,next);
      }
    }
      printf("\n");
    }

}
*/


int acsmPrintDetailInfo(ACSM_STRUCT * p)
{
    if(p)
        p = p;
    return 0;
}

int acsmPrintSummaryInfo(void)
{
#ifdef XXXXX
    char * fsa[]={
      "TRIE",
      "NFA",
      "DFA",
    };

    ACSM_STRUCT2 * p = &summary.acsm;

    if( !summary.num_states )
        return;

    printf("+--[Pattern Matcher:Aho-Corasick Summary]----------------------\n");
    printf("| Alphabet Size    : %d Chars\n",p->acsmAlphabetSize);
    printf("| Sizeof State     : %d bytes\n",sizeof(acstate_t));
    printf("| Storage Format   : %s \n",sf[ p->acsmFormat ]);
    printf("| Num States       : %d\n",summary.num_states);
    printf("| Num Transitions  : %d\n",summary.num_transitions);
    printf("| State Density    : %.1f%%\n",100.0*(double)summary.num_transitions/(summary.num_states*p->acsmAlphabetSize));
    printf("| Finite Automatum : %s\n", fsa[p->acsmFSA]);
    if( max_memory < 1024*1024 )
    printf("| Memory           : %.2fKbytes\n", (float)max_memory/1024 );
    else
    printf("| Memory           : %.2fMbytes\n", (float)max_memory/(1024*1024) );
    printf("+-------------------------------------------------------------\n");

#endif
    return 0;
}

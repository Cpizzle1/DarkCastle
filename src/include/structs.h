/************************************************************************
| $Id: structs.h,v 1.2 2002/06/13 04:41:15 dcastle Exp $
| structs.h
| Description:  This file should go away someday - it's stuff that I
|   wasn't sure how to break up.  --Morc XXX
*/
#ifndef STRUCTS_H_
#define STRUCTS_H_

extern "C" {
#include <sys/types.h>
#include <stdio.h> // FILE
}

FILE * dc_fopen(const char *filename, const char *type);
int dc_fclose(FILE * fl);

typedef signed char		sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;


typedef unsigned char			byte;

typedef	struct char_data	CHAR_DATA;
typedef	struct obj_data		OBJ_DATA;

#define MAX_STRING_LENGTH   8192
#define MAX_INPUT_LENGTH     160
#define MAX_MESSAGES          70

#define MESS_ATTACKER 1
#define MESS_VICTIM   2
#define MESS_ROOM     3

/* ======================================================================== */
struct txt_block
{
    char *text;
    struct txt_block *next;
    int aliased;
};

typedef struct txt_q
{
    struct txt_block *head;
    struct txt_block *tail;
} TXT_Q;

struct snoop_data
{
    CHAR_DATA *snooping; 
    CHAR_DATA *snoop_by;
};


struct msg_type 
{
    char *attacker_msg;  /* message to attacker */
    char *victim_msg;    /* message to victim   */
    char *room_msg;      /* message to room     */
};

struct message_type
{
    struct msg_type die_msg;      /* messages when death            */
    struct msg_type miss_msg;     /* messages when miss             */
    struct msg_type hit_msg;      /* messages when hit              */
    struct msg_type sanctuary_msg;/* messages when hit on sanctuary */
    struct msg_type god_msg;      /* messages when hit on god       */
    struct message_type *next;/* to next messages of ths kind.*/
};

struct message_list
{
    int a_type;               /* Attack type				*/
    int number_of_attacks;    /* # messages to chose from		*/
    struct message_type *msg; /* List of messages			*/
};

/*
 * TO types for act() output.
 */
/* OLD
#define TO_ROOM    0
#define TO_VICT    1
#define TO_NOTVICT 2
#define TO_CHAR    3
#define TO_GODS    4
*/


#endif

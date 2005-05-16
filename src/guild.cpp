/************************************************************************
| $Id: guild.cpp,v 1.79 2005/05/16 11:04:51 shane Exp $
| guild.C
| This contains all the guild commands - practice, gain, etc..
*/
#include <character.h>
#include <structs.h>
#include <spells.h>
#include <utility.h>
#include <levels.h>
#include <player.h>
#include <db.h> // exp_table
#include <interp.h>
#include <string.h>
#include <returnvals.h>
#include <ki.h>
#include <mobile.h>
#include <room.h>
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif
extern CWorld world;
extern struct class_skill_defines g_skills[];
extern struct class_skill_defines w_skills[];
extern struct class_skill_defines t_skills[];
extern struct class_skill_defines d_skills[];
extern struct class_skill_defines b_skills[];
extern struct class_skill_defines a_skills[];
extern struct class_skill_defines p_skills[];
extern struct class_skill_defines r_skills[];
extern struct class_skill_defines k_skills[];
extern struct class_skill_defines u_skills[];
extern struct class_skill_defines c_skills[];
extern struct class_skill_defines m_skills[];
extern struct index_data *mob_index;

char *str_nospace(char *stri);
int get_max(CHAR_DATA *ch, int skill);

int guild(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *owner);

int do_practice(CHAR_DATA *ch, char *arg, int cmd)
{
  /* Call "guild" with a null string for an argument.
     This displays the character's skills. */

  if (arg[0] != '\0'){
    send_to_char("You can only practice in a guild.\n\r", ch);
  } else {
    guild(ch, 0, cmd, "", ch);
  }
  return eSUCCESS;
}

char *how_good(int percent)
{
  if (percent == 0)
    return ( " (not learned)");
  if (percent <= 5)
    return ( " (horrible)");
  if (percent <= 10)
    return ( " (crappy)");
  if (percent <= 15)
    return ( " (meager)");
  if (percent <= 20)
    return ( " (bad)");
  if (percent <= 30)
    return ( " (poor)");
  if (percent <= 40)
    return ( " (decent)");
  if (percent <= 50)
    return ( " (average)");
  if (percent <= 60)
    return ( " (fair)");
  if (percent <= 70)
    return ( " (good)");
  if (percent <= 80)
    return ( " (very good)");
  if(percent <= 85)
    return ( " (excellent)");
  if (percent <= 90)
    return (" (superb)");

  return (" (Masterful)");
}
	
// return a 1 if I just learned skill for first time
// else 0
int learn_skill(char_data * ch, int skill, int amount, int maximum)
{
  struct char_skill_data * curr = ch->skills;
  int old;
  while(curr)
    if(curr->skillnum == skill)
      break;
    else curr = curr->next;

  if(curr) 
  {
    old = curr->learned;
    curr->learned += amount;
    if (skill == SKILL_MAGIC_RESIST)
      barb_magic_resist(ch, old, curr->learned);
    if(curr->learned > maximum)
      curr->learned = maximum;
  }
  else 
  {
#ifdef LEAK_CHECK
    curr = (char_skill_data *)calloc(1, sizeof(char_skill_data));
#else
    curr = (char_skill_data *)dc_alloc(1, sizeof(char_skill_data));
#endif
    if (skill == SKILL_MAGIC_RESIST)
      barb_magic_resist(ch, 0, amount);
    curr->skillnum = skill;
    curr->learned = amount;
    curr->next = ch->skills;
    ch->skills = curr;
    return 1;
  }
  return 0;
}

int search_skills2(int arg, class_skill_defines * list_skills)
{
  for(int i = 0; *list_skills[i].skillname != '\n'; i++)
    if(arg == list_skills[i].skillnum)
      return i;

  return -1;
}

int search_skills(char * arg, class_skill_defines * list_skills)
{
  for(int i = 0; *list_skills[i].skillname != '\n'; i++)
    if(is_abbrev(arg, list_skills[i].skillname))
      return i;

  return -1;
}

int default_master[] = {
  -1,        // no class 0
   1937,     // CLASS_MAGIC_USER   1
   1930,     // CLASS_CLERIC       2   
   1928,     // CLASS_THIEF        3   
   1926,     // CLASS_WARRIOR      4   
   1920,     // CLASS_ANTI_PAL     5   
   1935,     // CLASS_PALADIN      6   
   1922,     // CLASS_BARBARIAN    7   
   1932,     // CLASS_MONK         8   
   1924,     // CLASS_RANGER       9   
   1939,     // CLASS_BARD        10   
   1941,     // CLASS_DRUID       11
   0,        // CLASS_PSIONIC     12
   0         // CLASS_NECROMANCER 13
};

class_skill_defines * get_skill_list(char_data * ch)
{
  class_skill_defines * skilllist = NULL;

  switch(GET_CLASS(ch)) {
    case CLASS_THIEF:      skilllist = t_skills;  break;  
    case CLASS_WARRIOR:    skilllist = w_skills;  break;  
    case CLASS_BARBARIAN:  skilllist = b_skills;  break;  
    case CLASS_PALADIN:    skilllist = p_skills;  break;  
    case CLASS_ANTI_PAL:   skilllist = a_skills;  break;  
    case CLASS_RANGER:     skilllist = r_skills;  break;  
    case CLASS_BARD:       skilllist = d_skills;  break;  
    case CLASS_MONK:       skilllist = k_skills;  break;  
    case CLASS_DRUID:      skilllist = u_skills;  break;
    case CLASS_CLERIC:     skilllist = c_skills;  break;
    case CLASS_MAGIC_USER: skilllist = m_skills;  break;
    default:
      break;
  }
  return skilllist;
}
void debug_here()
{}

char *attrname(int attr)
{
  switch (attr)
  {
    case STR: return "Str";
    case INT: return "Int";
    case WIS: return "Wis";
    case DEX: return "Dex";
    case CON: return "Con";
    default: return "Err";
  }
}
int skillmax(struct char_data *ch, int skill, int eh)
{
  class_skill_defines * skilllist = get_skill_list(ch);
  if (IS_NPC(ch)) return eh;
  if(!skilllist)
    return eh; // scan etc
  int i = search_skills2(skill, skilllist);
  if (i==-1) return eh; // imm trying it out, doesn't have the skill
  if (skilllist[i].maximum < eh)
	return skilllist[i].maximum;
  return eh;
}

int skills_guild(struct char_data *ch, char *arg, struct char_data *owner)
{
  char buf[160];
  int known, x;
  int skillnumber;
  int percent;
  int specialization;
  int loop;

  class_skill_defines * skilllist = get_skill_list(ch);
  if(!skilllist)
    return eFAILURE;  // no skills to train

  if (!*arg) // display skills that can be learned
  {
    sprintf(buf,"You have %d practice sessions left.\n\r", ch->pcdata->practices);
    send_to_char(buf, ch);
    send_to_char("You can practice any of these skills:\n\r", ch);
    for(int i=0; *skilllist[i].skillname != '\n';i++) 
    {
      known = has_skill(ch, skilllist[i].skillnum);
      if(!known && GET_LEVEL(ch) < skilllist[i].levelavailable)
          continue;
      specialization = known / 100;
      known = known % 100;
      sprintf(buf, " %-20s%14s   (Level %2d)", skilllist[i].skillname,
                   how_good(known), skilllist[i].levelavailable);
      send_to_char(buf, ch);
      if(skilllist[i].skillnum >= 1 && skilllist[i].skillnum <= MAX_SPL_LIST) 
      {
	if (skilllist[i].skillnum == SPELL_PORTAL && GET_CLASS(ch) == CLASS_CLERIC)
	sprintf(buf," Mana: %4d ", 150);
	else
        sprintf(buf," Mana: %4d ",use_mana(ch,skilllist[i].skillnum));
        send_to_char(buf, ch);
      }
      else send_to_char("            ", ch);
      if (skilllist[i].attrs[0])
      {
	sprintf(buf, " %s ",attrname(skilllist[i].attrs[0]));
	send_to_char(buf, ch);
      }
      if (skilllist[i].attrs[1])
      {
        sprintf(buf, " (%s) ",attrname(skilllist[i].attrs[1]));
        send_to_char(buf, ch);
      }
      if(specialization) 
      {
        for(loop = 0; loop < specialization; loop++)
          send_to_char("*", ch);
        send_to_char("\r\n", ch);
      }
      else send_to_char("\n\r", ch);
    }
    send_to_char("\n\r* denotes a point of specialization.\n\r", ch);
    return eSUCCESS;
  }
  if (GET_POS(ch) == POSITION_SLEEPING) {
   send_to_char("You can't practice in your sleep.\r\n",ch);
   return eSUCCESS;
  }
  skillnumber = search_skills(arg, skilllist);
    
  if (skillnumber == -1) {
    return eFAILURE;
  }

  if(GET_LEVEL(ch) < skilllist[skillnumber].levelavailable) {
    send_to_char("You aren't advanced enough for that yet.\r\n", ch);
    return eSUCCESS;
  }

  x = skilllist[skillnumber].skillnum;
  known = has_skill(ch, x);

  // we can only train them if they already know it, or if we're the trainer for that skill
  if (!known)
  {
   if (default_master[GET_CLASS(ch)] != mob_index[owner->mobdata->nr].virt) {
	do_say(owner, "I'm sorry, I can't teach you that.  You'll have to find another trainer.",9);
       return eSUCCESS;
   }
   else {
      struct skill_quest *sq;
     if ((sq=find_sq(str_nospace(skilllist[skillnumber].skillname))) != NULL && sq->message && IS_SET(sq->clas, 1<<(GET_CLASS(ch)-1)))
     {
	mprog_driver(sq->message, owner, ch, NULL, NULL);
	return eSUCCESS;
     }
   }

//    if(skilllist[skillnumber].clue && mob_index[owner->mobdata->nr].virt 
//== default_master[GET_CLASS(ch)])
 //     do_say(owner, skilllist[skillnumber].clue, 9);
   // else do_say(owner, "I'm sorry, I can't teach you that.  You'll have 
//to find another trainer.", 9);
//    return eSUCCESS;
  }
  if (ch->pcdata->practices <= 0) {
    send_to_char("You do not seem to be able to practice now.\n\r", ch);
    return eSUCCESS;
  }

  if(known >= get_max(ch, x)) {
     do_emote(owner, "eyes you up and down.", 9);
     do_say(owner, "Taking into account your current attributes, your",9);
     do_say(owner, "maximum proficiency in this ability has been reached.", 9);
     return eSUCCESS;
  }

  if (known >= skilllist[skillnumber].maximum) {
    send_to_char("You are already learned in this area.\n\r", ch);
    return eSUCCESS;
  }
  float maxlearn = (float)skilllist[skillnumber].maximum;
  maxlearn *= 0.5;

  if (known >= ( GET_LEVEL(ch) * 2 )) {
    send_to_char("You aren't experienced enough to practice that any further right now.\n\r", ch);
    return eSUCCESS;
  }
  if (known >= maxlearn)
  {
    send_to_char("You cannot learn more here.. you need to go out into the world and use it.\r\n",ch);
    return eSUCCESS;
  }
  switch(skillnumber)
  {
      case SPELL_VAMPIRIC_AURA:
      case SKILL_BULLRUSH:
      case SPELL_HOLY_AURA:
      case SKILL_SPELLCRAFT:
	do_say(owner, "I cannot teach you that. You need to learn it by yourself.\r\n",9);
	return eFAILURE;
      default: break;
  }
   if (!known)
  switch (GET_CLASS(ch))
  {
    case CLASS_WARRIOR:
      do_say(owner, "Yar! I can be teachin' ye that skill myself! It should only take but a moment.",9);
      break;
    case CLASS_BARBARIAN:
      do_say(owner,"Hah! That easy to learn! I teach you meself.",9);
      break;
    case CLASS_THIEF:
      do_say(owner,"So young rogue, you wish to advance your skills.  I can teach you of this particular one myself.",9);
	break;
     case CLASS_MONK:
      do_say(owner,"Ahh, well met grasshopper!  I can teach you of this from my own knowledge.",9);
	break;
     case CLASS_RANGER:
     do_say(owner,"My woodland lore is more than sufficient to teach you this myself young apprentice!",9);
	break;
     case CLASS_ANTI_PAL:
     do_say(owner,"Ahh, young dark lord, it is but a simple matter to learn this ability.  Follow my instruction.",9);
	break;
     case CLASS_PALADIN:
     do_say(owner,"This ability is one that I am capable of teaching you myself young novice.  Observe closely.",9);
	break;
     case CLASS_BARD:
      do_say(owner, "Ahh young prodigy, that is a tune with which I myself am familiar! Allow me to show you...",9);
	break;
     case CLASS_MAGIC_USER:
     do_say(owner,"Ahh young apprentice, this is a simple matter for me to teach you if you are capable of comprehending.",9);
	break;
     case CLASS_CLERIC:
     do_say(owner, "Well met Acolyte!  I shall pray for you to receive knowledge of the blessing you request.",9);
	break;
     case CLASS_DRUID:
     do_say(owner, "Nature be with you, young druid.  I can teach you the ability you seek myself if you are willing.",9);
	break;
  }
  send_to_char("You practice for a while...\n\r", ch);
  ch->pcdata->practices--;

  percent = 1 + (int)int_app[GET_INT(ch)].learn_bonus;

  learn_skill(ch, x, percent, skilllist[skillnumber].maximum);
    
  if (known >= skilllist[skillnumber].maximum) {
    send_to_char("You are now learned in this area.\n\r", ch);
    return eSUCCESS;
  }

  return eSUCCESS;
}

int guild(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  int64 exp_needed;
  int x = 0;

  if(cmd == 171 && !IS_MOB(ch)) {         /*   gain crap...  */

    if((GET_LEVEL(ch) >= IMMORTAL) || (GET_LEVEL(ch) == MAX_MORTAL)) {
      send_to_char("Only a god can level you now.\n\r", ch);
      return eSUCCESS;
    }

   // TODO - make it so you have to be at YOUR guildmaster to gain

    exp_needed = exp_table[(int)GET_LEVEL(ch) + 1];

    if(exp_needed > GET_EXP(ch)) {
      x = (int)(exp_needed - GET_EXP(ch));

      csendf(ch, "You need %d experience to level.\n\r", x);
      return eSUCCESS;
    }

    send_to_char ("You raise a level!\n\r", ch);

    GET_LEVEL(ch) +=1;
    advance_level(ch, 0);
    GET_EXP(ch) -= (int)exp_needed;

    return eSUCCESS;
  }
  
  if((cmd != 164)) 
   return eFAILURE;
  
  if(IS_MOB(ch)) {
    send_to_char("Why practice?  You're just going to die anyway...\r\n", ch);
    return eFAILURE;
  }

  for(; *arg==' '; arg++);  // skip whitespace
    
  if(!*arg)
  {
    skills_guild(ch, arg, owner);
  }
  else
  {
    if(IS_SET(skills_guild(ch, arg, owner), eSUCCESS))
      return eSUCCESS;
    send_to_char("You do not know of this ability...\n\r", ch);
  }

  return eSUCCESS;
}

int skill_master(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, 
                 struct char_data * invoker)
{

  char buf[MAX_STRING_LENGTH];
  int number, i, percent;
  int learned = 0;

  if(IS_MOB(ch)) {
    send_to_char("Why practice?  You're just going to die anyway...\r\n", ch);
    return eFAILURE;
  }

  if ((cmd != 164) && (cmd != 170)) 
    return eFAILURE;
  
  for(; *arg==' '; arg++);

  if (!*arg) {
      sprintf(buf,"You have %d practice sessions left.\n\r", ch->pcdata->practices);
      send_to_char(buf, ch);
      send_to_char("You can practice  any of these skills:\n\r", ch);
      for(i=0; *g_skills[i].skillname != '\n';i++) 
      {
        int known = has_skill(ch, g_skills[i].skillnum);
        if(GET_LEVEL(ch) < g_skills[i].levelavailable) 
          continue;
        sprintf(buf, " %-20s%14s   (Level %2d)\r\n", g_skills[i].skillname,
                   how_good(known), g_skills[i].levelavailable);
        send_to_char(buf, ch);
      }
      return eSUCCESS;
    }
    number = search_skills(arg,g_skills);
    
    if (number == -1) {
      send_to_char("You do not know of this skill...\n\r", ch);
      return eSUCCESS;
    }

    if (ch->pcdata->practices <= 0) {
      send_to_char("You do not seem to be able to practice now.\n\r", ch);
      return eSUCCESS;
    }
    learned = has_skill(ch, g_skills[number].skillnum);

    if (learned >= g_skills[number].maximum) {
      send_to_char("You are already learned in this area.\n\r", ch);
      return eSUCCESS;
    }
    if (learned >= ( GET_LEVEL(ch) * 3 ) ) {
      send_to_char("You aren't experienced enough to practice that any further right now.\n\r", ch);
      return eSUCCESS;
    }

    send_to_char("You practice for a while...\n\r", ch);
    ch->pcdata->practices--;
    
    percent = 1+(int)int_app[GET_INT(ch)].learn_bonus;

    learn_skill(ch, g_skills[number].skillnum, percent, g_skills[number].maximum);
    learned = has_skill(ch, g_skills[number].skillnum);
    
    if (learned >= g_skills[number].maximum) {
      send_to_char("You are now learned in this area.\n\r", ch);
      return eSUCCESS;
    }
    return eSUCCESS;
}

int get_stat(CHAR_DATA *ch, int stat)
{
      switch (stat)
      {
        case STRENGTH:
          return GET_RAW_STR(ch);
          break;
        case INTELLIGENCE:
          return GET_RAW_INT(ch);
          break;
        case WISDOM:
          return GET_RAW_WIS(ch);
          break;
        case DEXTERITY:
          return GET_RAW_DEX(ch);
          break;
        case CONSTITUTION:
          return GET_RAW_CON(ch);
          break;
      };
 return 0;
}


// TODO - go ahead and remove 'learned' from everywhere to use it.
// We can't always pass it in, since in 'group' type spells or
// object affects someone that doesn't have the skill is getting a
// valid amount passed in.

void skill_increase_check(char_data * ch, int skill, int learned, int difficulty)
{
   int chance, maximum;
   if (ch->in_room && IS_SET(world[ch->in_room].room_flags, NOLEARN))
	return;

   if (!difficulty) 
   {
     logf(IMMORTAL, LOG_BUG, "Illegal difficulty in skill %d. Tell someone.", skill);
     return; // Skill w/out difficulty.
   }
   if( ! ( learned = has_skill(ch, skill) ) )
      return; // get out if i don't have the skill

   if(learned > ( GET_LEVEL(ch) * 2 ))
      return;

   if(IS_MOB(ch))
      return;
   if (IS_SET(world[ch->in_room].room_flags, SAFE)) return;
   class_skill_defines * skilllist = get_skill_list(ch);
   if(!skilllist)
     return;  // class has no skills by default

   maximum = 0;
   int i;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     if(skilllist[i].skillnum == skill)
     {
       maximum = skilllist[i].maximum;
       break;
     }
   if (!maximum) {
    skilllist = g_skills;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     if(skilllist[i].skillnum == skill)
     {
       maximum = skilllist[i].maximum;
       break;
     }
   }
   if (!maximum) return;
   int to = (int)((float)maximum*(float)0.75);
   if (skilllist[i].attrs[0])
   {
	int thing = get_stat(ch,skilllist[i].attrs[0])-20;
	if (thing > 0)
 	 to += (int)((get_stat(ch,skilllist[i].attrs[0])-20)*2.5);
	if (to > 90) to = 90;
   }
   if (skilllist[i].attrs[1])
   {
	int thing = get_stat(ch,skilllist[i].attrs[1])-20;
	if (thing > 0)
         to += (get_stat(ch,skilllist[i].attrs[1])-20);
   }
   if (to > maximum) to = maximum;
   if(learned >= to)
     return;

   chance = number(1, 101);
   if(learned < 15)
      chance += 5;
   int oi = 101;
   if (difficulty > 500) { oi = 100; difficulty -= 500; }
   switch(difficulty) {
     case SKILL_INCREASE_EASY:
	 if (oi==101) oi = 94;
	 oi -= int_app[GET_INT(ch)].easy_bonus;
        break;
     case SKILL_INCREASE_MEDIUM:
	if (oi==101)oi = 96;
	oi -= int_app[GET_INT(ch)].medium_bonus;
        break;
     case SKILL_INCREASE_HARD:
	if (oi==101)oi = 98;
	oi -= int_app[GET_INT(ch)].hard_bonus;
        break;
     default:
       log("Illegal difficulty value sent to skill_increase_check", IMMORTAL, LOG_BUG);
       break;
   }
   if (oi > chance) return;
   // figure out the name of the affect (if any)
   char * skillname = get_skill_name(skill);

   if(!skillname) {
      csendf(ch, "Increase in unknown skill %d.  Tell a god. (bug)\r\n", skill);
      return;
   }

   // increase the skill by one
   learn_skill(ch, skill, 1, maximum);
   csendf(ch, "You feel more competent in your %s ability.\r\n", skillname);
}

int get_max(CHAR_DATA *ch, int skill)
{
   int maximum;
   class_skill_defines * skilllist = get_skill_list(ch);
   if(!skilllist)
     return -1;  // class has no skills by default
   maximum = 0;
   int i;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     if(skilllist[i].skillnum == skill)
     {
       maximum = skilllist[i].maximum;
       break;
     }
   int percent = (int) ((float)maximum * 0.75);
   if (skilllist[i].attrs[0])
   {
        int thing = get_stat(ch,skilllist[i].attrs[0])-20;
        if (thing > 0)
          percent += (int)((get_stat(ch,skilllist[i].attrs[0])-20)*2.5);
        if(percent > 90) percent = 90;
   }
   if (skilllist[i].attrs[1])
   {
        int thing = get_stat(ch,skilllist[i].attrs[1])-20;
        if (thing > 0)
        percent += (get_stat(ch,skilllist[i].attrs[1])-20);
   }
   return percent;
}

void check_maxes(CHAR_DATA *ch)
{
   int maximum;
   class_skill_defines * skilllist = get_skill_list(ch);
   if(!skilllist)
     return;  // class has no skills by default
   maximum = 0;
   int i;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     {
       maximum = skilllist[i].maximum;
	int to = (int)((float)maximum*0.75);
       if (skilllist[i].attrs[0])
       {
            int thing = get_stat(ch,skilllist[i].attrs[0])-20;
            if (thing > 0)
              to += (int)((get_stat(ch,skilllist[i].attrs[0])-20)*2.5);
	    if (to > 90) to = 90;
       }
       if (skilllist[i].attrs[1])
       {
            int thing = get_stat(ch,skilllist[i].attrs[1])-20;
            if (thing > 0)
              to += (get_stat(ch,skilllist[i].attrs[1])-20);
       }
       if (to > maximum) to = maximum;
       if (has_skill(ch,skilllist[i].skillnum) > to)
       {
	  struct char_skill_data * curr = ch->skills;

  	  while(curr)
    	  if(curr->skillnum == skilllist[i].skillnum)
      	  break;
    	  else curr = curr->next;

	  if (curr) curr->learned = to;
       }
   }
}

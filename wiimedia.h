/* Global Includes */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cwiid.h>
#include <unistd.h>

/* C++ Libraries */
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

#define toggle_bit(bf,b) (bf) = ((bf) & b) ? ((bf) & ~(b)) : ((bf) | (b))

void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state);
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode);

/* Outputs Battery Level to LEDs */
void led_battery_state(cwiid_wiimote_t *wiimote, struct cwiid_state *state);
/* Interprets Data From Wiimote */
void interpreter(struct cwiid_state *state);
/* Lists Artists For Current Letter of Alphabet */
void alphabet(int alpha);
/* Music Player Daemon Controller */
void mpdcontrol(int command);
/* Displays Currently Playing Song */
void nowplaying(int playstatus);
/* Adds New Song To Playlist */
void newsong(int type);
/* Lists Specific Artists */
void specificartist(int selection);
/* Waits For Specified Length of Time */
void wait(int seconds);
/* Print Message */
void gmessage(char * timeout, char * name, char * title, int file, char * output);
/* Delete Current Song */
void deletesong();


/* GLOBAL VARIABLES */
int play_status=0;
int artist=-1;
int artistselection=0;
string currentartist;

/* GLOBAL ALPHABET TABLE */
char alphatable[26]={'A','B','C','D','E','F','G','H',
		     'I','J','K','L','M','N','O','P',
	 	     'Q','R','S','T','U','V','W','X',
		     'Y','Z'};

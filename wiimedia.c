/* Local Includes */
#include "wiimedia.h"
#include "libmpdclient.h"

cwiid_mesg_callback_t cwiid_callback;
cwiid_err_t err;

void err(cwiid_wiimote_t *wiimote, const char *s, va_list ap)
{
	if (wiimote) printf("%d:", cwiid_get_id(wiimote));
	else printf("-1:");
	vprintf(s, ap);
	printf("\n");
}

int main(int argc, char *argv[])
{
	cwiid_wiimote_t *wiimote;	// wiimote handle
	struct cwiid_state state;	// wiimote state
	bdaddr_t bdaddr;		// bluetooth device address
	unsigned char rpt_mode = 0;
	int exit = 0;

	cwiid_set_err(err);

	// Connect to address given on command-line, if present
	if (argc > 1) 
	{
		str2ba(argv[1], &bdaddr);
	}
	else 
	{
		bdaddr = *BDADDR_ANY;
	}

	// Connect to the wiimote
	printf("Put Wiimote in discoverable mode now (press 1+2)...\n");
	if (!(wiimote = cwiid_open(&bdaddr, 0))) 
	{
		fprintf(stderr, "Unable to connect to wiimote\n");
		return -1;
	}
	if (cwiid_set_mesg_callback(wiimote, cwiid_callback)) 
	{
		fprintf(stderr, "Unable to set message callback\n");
	}

	// Set Accelerometer Reporting
        toggle_bit(rpt_mode, CWIID_RPT_ACC);
        set_rpt_mode(wiimote, rpt_mode);

	// Set Button Reporting
        toggle_bit(rpt_mode, CWIID_RPT_BTN);
        set_rpt_mode(wiimote, rpt_mode);

        // Set IR Reporting
        toggle_bit(rpt_mode, CWIID_RPT_IR);
        set_rpt_mode(wiimote, rpt_mode);


	// Display Battery Level
        if (cwiid_get_state(wiimote, &state))
        {
        	fprintf(stderr, "Error getting state\n");
        }
        led_battery_state(wiimote, &state);

	// While loop to constantly poll for the state from the wiimote
	while(!exit)
	{
		if (cwiid_get_state(wiimote, &state))
		{
			fprintf(stderr, "Error getting state\n");
		}
		interpreter(&state);

		// Show Battery Level On LEDs
		if (cwiid_get_state(wiimote, &state))
                {
                	fprintf(stderr, "Error getting state\n");
                }
		led_battery_state(wiimote, &state);

	}

	if (cwiid_close(wiimote)) 
	{
		fprintf(stderr, "Error on wiimote disconnect\n");
		return -1;
	}

	return 0;
}

void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state)
{
	if (cwiid_set_led(wiimote, led_state)) 
	{
		fprintf(stderr, "Error setting LEDs \n");
	}
	return;
}
	
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode)
{
	if (cwiid_set_rpt_mode(wiimote, rpt_mode)) 
	{
		fprintf(stderr, "Error setting report mode\n");
	}
	return;
}

// Counter
void wait(int seconds)
{
	clock_t endwait = clock () + seconds * CLOCKS_PER_SEC ;
	while (clock() < endwait) {}
	return;
}

// Input Interpreter
void interpreter(struct cwiid_state *state)
{
        int X = state->acc[CWIID_X];

	switch(state->buttons)
	{
		// Fast-Forward / Rewind -> (Home) + Rotate
		case 0x0080:
			// Rotate Right to FF
			if( X > 120 ) 
				mpdcontrol(5);
			// Rotate Left to RW
			else if( X < 116 ) 
				mpdcontrol(6);
			break;
		// List Artists -> (1) + Jerk
		case 0x0002:
			// Jerk Left
			if( (X > 180))
			{
				if(artist >=0)
					artist--;
				else
					artist=25;
				alphabet(artist);
			}
			// Jerk Right
			else if(X < 60)
			{
				if( artist < 26 )
					artist++;
				else
					artist=0;
                        	alphabet(artist);
			}
			break;
		// Choose Specific Artist -> (2) + Jerk
		case 0x0001:
			// Jerk Left
			if( X > 180 )
			{
				artistselection--;
				specificartist(artistselection);
			}
			// Jerk Right
			else if( X < 60 )
			{
				artistselection++;
				specificartist(artistselection);
			}
			break;
		// Volume Up -> (+)
		case 0x1000:
	        	// Was unable to find a volume API, so used system commands
			system("amixer -q -c 0 sset Master 1+");
			wait(1);
			break;
		// Volume Down -> (-)
		case 0x0010:
			// Was unable to find a volume API, so used system commands
			system("amixer -q -c 0 sset Master 1-");
			wait(1);
			break;
		// Play / Pause -> (A)
		case 0x0008:
			mpdcontrol(7);
			if(play_status==3 || play_status==1)
			{
                		mpdcontrol(3);
				nowplaying(1);
				play_status=2;
			}
			else if(play_status==2)
			{
                		mpdcontrol(4);
				nowplaying(0);
				play_status=3;
			}
			break;
		// Current Song + Play Status -> (A)+(B)
		case 0x000C:
        	        if(play_status==2)
                	{
                       		mpdcontrol(3);
                		nowplaying(1);
			}
                	else if(play_status==3)
                	{
                        	mpdcontrol(4);
                		nowplaying(0);
			}
        		break;
		// Previous Song -> (<-)
        	case 0x0100:
			mpdcontrol(2);
			nowplaying(1);
			break;
		// Next Song -> (->)
        	case 0x0200:
                	mpdcontrol(1);
			nowplaying(1);
			break;
		// Add Random Song -> (^)
		case 0x0800:
			newsong(0);
			break;
		// Add Random Song From Specific Artist -> (B)+(^)
		case 0x0804:
			newsong(1);
			break;
		// Remove Current Song From Playlist -> (v)
        	case 0x0400:
			deletesong();
			break;
		// Toggle Shuffle Play -> (1)+(2)
		case 0x0003:
        	       	mpdcontrol(8);
			break;
		default:
			break;
	}
	return;
}

// Music Player Daemon Control Function
void mpdcontrol(int command)
{
	int songid=0;
	int elapsed=0;

	// Music Player Daemon Connection Information 
	mpd_Connection * conn;
	char *hostname = getenv("MPD_HOST");
	char *port = getenv("MPD_PORT");

	// Set to defaults if undefined 
	if(hostname == NULL)
		hostname = "localhost";
	if(port == NULL)
		port = "6600";

        conn = mpd_newConnection(hostname,atoi(port),10);

	if(conn->error) 
	{
		fprintf(stderr,"%s\n",conn->errorStr);
		mpd_closeConnection(conn);
		return;
	}

	mpd_Status * status;
        
	mpd_sendCommandListOkBegin(conn);
        mpd_sendStatusCommand(conn);
        mpd_sendCurrentSongCommand(conn);
        mpd_sendCommandListEnd(conn);

        if((status = mpd_getStatus(conn))==NULL) 
        {
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
      	}

	songid=status->song;
	elapsed=status->elapsedTime;

	mpd_finishCommand(conn);
        if(conn->error)
        {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

	switch(command)
	{
		// Next 
		case 1:
			mpd_sendNextCommand(conn);
			break;
		// Prev 
		case 2: 
			mpd_sendPrevCommand(conn);
			break;
		// Play 
		case 3:
			mpd_sendPlayCommand(conn, MPD_PLAY_AT_BEGINNING);
			break;
		// Pause 
		case 4:
			mpd_sendPauseCommand(conn, 1);
			break;
		// FF 
		case 5:
			mpd_sendSeekCommand(conn, songid, elapsed+1);
			break;
		// RW 
		case 6:
			mpd_sendSeekCommand(conn, songid, elapsed-1);
                        break;
		// Set Play Status 
		case 7:
			play_status = status->state;
			break;
		// Toggle Random Mode 
		case 8:
			if(status->random==0)
			{
				mpd_sendRandomCommand(conn, 1);
				gmessage("1", "Shuffle Turned On", "Shuffle Turned On", 0, "Playlist Shuffle Turned On");
			}
			else if(status->random==1)
			{
				mpd_sendRandomCommand(conn, 0);
				gmessage("1", "Shuffle Turned Off", "Shuffle Turned Off", 0, "Playlist Shuffle Turned Off");
			}
			break;
		// Remove Song From Playlist 
		case 9:
			mpd_sendDeleteCommand(conn, songid);
			break;
		default:
			break;
	}
 	
	mpd_finishCommand(conn);
        if(conn->error) 
        {
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

	mpd_freeStatus(status);
	mpd_closeConnection(conn);
	return;
}

//Delete Current Song
void deletesong()
{
	ofstream list;
	string artist;
	string title;
	string artist2;
	string title2;

	mpd_Connection * conn;
        char *hostname = getenv("MPD_HOST");
        char *port = getenv("MPD_PORT");

        if(hostname == NULL)
                hostname = "localhost";
        if(port == NULL)
                port = "6600";

        conn = mpd_newConnection(hostname,atoi(port),10);

        if(conn->error) {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

	mpd_Status * status;
        mpd_InfoEntity * entity;

        mpd_sendCommandListOkBegin(conn);
        mpd_sendStatusCommand(conn);
        mpd_sendCurrentSongCommand(conn);
        mpd_sendCommandListEnd(conn);

        if((status = mpd_getStatus(conn))==NULL) {
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }
	mpd_nextListOkCommand(conn);

	// Get Current Artist and Track
	while((entity = mpd_getNextInfoEntity(conn))) {
        	mpd_Song * song = entity->info.song;

                if(entity->type!=MPD_INFO_ENTITY_TYPE_SONG) {
                	mpd_freeInfoEntity(entity);
                        continue;
                }

                artist=song->artist;
                title=song->title;

                mpd_freeInfoEntity(entity);
	}

	 if(conn->error) {
                fprintf(stderr,"%s\n",conn->errorStr);
         	mpd_closeConnection(conn);
        	return;
        }

        mpd_finishCommand(conn);
        if(conn->error) {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
        	return;
        }

	// Open File And Write Current Song
	list.open("/etc/wiimedia/deletesong.dat");
        if(list.is_open())
        {
		list << "Removed Song:\n";
	        list << artist << " - " << title;
	        list.close();

       		gmessage("1", "Removed Song", "Removed Song", 1, "/etc/wiimedia/deletesong.dat");
        }
        else
        {
		gmessage("2", "Unable To Get Current Song", "Unable To Get Current Song", 0, "Unable To Get Current Song");
        }

	mpdcontrol(9);

	nowplaying(1);

        mpd_freeStatus(status);

	return;
}

// Output Currently Playing Song 
void nowplaying(int playstatus)
{
	ofstream list;
	string artist;
	string title;

	mpd_Connection * conn;
        char *hostname = getenv("MPD_HOST");
        char *port = getenv("MPD_PORT");

        if(hostname == NULL)
                hostname = "localhost";
        if(port == NULL)
                port = "6600";

        conn = mpd_newConnection(hostname,atoi(port),10);

        if(conn->error) {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

	mpd_Status * status;
        mpd_InfoEntity * entity;

        mpd_sendCommandListOkBegin(conn);
        mpd_sendStatusCommand(conn);
        mpd_sendCurrentSongCommand(conn);
        mpd_sendCommandListEnd(conn);

        if((status = mpd_getStatus(conn))==NULL) {
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

	mpd_nextListOkCommand(conn);

	// Get Current Artist and Track
	while((entity = mpd_getNextInfoEntity(conn))) {
        	mpd_Song * song = entity->info.song;

                if(entity->type!=MPD_INFO_ENTITY_TYPE_SONG) {
                	mpd_freeInfoEntity(entity);
                        continue;
                }

                artist=song->artist;
                title=song->title;

                mpd_freeInfoEntity(entity);
	}

	 if(conn->error) {
                fprintf(stderr,"%s\n",conn->errorStr);
         	mpd_closeConnection(conn);
        	return;
        }

        mpd_finishCommand(conn);
        if(conn->error) {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
        	return;
        }

        mpd_freeStatus(status);


	// Open File And Write Current Song
	list.open("/etc/wiimedia/currentsong.dat");
        if(list.is_open())
        {
		if(playstatus==0)
			list << "Now Playing (Paused):\n";
		else if(playstatus==1)
			list << "Now Playing:\n";
	        list << artist << " - " << title;
                list.close();

		gmessage("1", "Current Song", "Current Song", 1, "/etc/wiimedia/currentsong.dat");
        }
        else
        {
		gmessage("2", "Unable To Get Current Song", "Unable To Get Current Song", 0, "Unable To Get Current Song");
        }

	return;
}

// Add a new song to the playlist 
void newsong(int type)
{	
	ofstream list;
	ifstream inlist;
        char * artist;
	string artistlist="";
	string chosensong="";
      	string line;
	string formattedartist;
      	int nlines = 0;

        // Music Player Daemon Connection Information
        mpd_Connection * conn;
        
	char *hostname = getenv("MPD_HOST");
        char *port = getenv("MPD_PORT");

        // Set to defaults if undefined
        if(hostname == NULL)
                hostname = "localhost";
        if(port == NULL)
                port = "6600";

        conn = mpd_newConnection(hostname,atoi(port),10);

        if(conn->error)
        {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

        mpd_Status * status;

        mpd_sendCommandListOkBegin(conn);
        mpd_sendStatusCommand(conn);
        mpd_sendCurrentSongCommand(conn);
        mpd_sendCommandListEnd(conn);

        if((status = mpd_getStatus(conn))==NULL)
        {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

        mpd_finishCommand(conn);

	if(conn->error)
        {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

	

	// List All Songs
	if(type==0)
        	mpd_sendListallCommand(conn, "/");
	else if(type==1)
        	mpd_sendListallCommand(conn, currentartist.c_str());
        if(conn->error) 
	{
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }
	
	// Get Filenames
	artist = mpd_getNextTag(conn,MPD_TAG_ITEM_FILENAME);
	artistlist+=artist;
	
        while((artist = mpd_getNextTag(conn,MPD_TAG_ITEM_FILENAME))) 
	{
		artistlist+="\n";
		artistlist+=artist;
                free(artist);
        }

	// Open File And Write Artists
        list.open("/etc/wiimedia/artistsonglist.dat");
	if(list.is_open())
	{	
		list << artistlist;
		list.close();
	}
	else
	{
		gmessage("2", "Unable To Get Artists", "Unable To Get Artists", 0, "Unable To Get Artists");
	}

	// Read Artistlist and Choose Random Song
        inlist.open("/etc/wiimedia/artistsonglist.dat");
        if(inlist.is_open())
	{	
		srand(time(NULL));
      		while( getline( inlist, line ) )
         		if(  ( rand() % ++nlines  ) == 0 )
                	  	chosensong = line ;
		inlist.close();
	}
	else
	{
		gmessage("2", "Unable To Get Song", "Unable To Get Song", 0, "Unable To Get Song");
	}

	// Write Random Song To Data File and Add To Playlist
	list.open("/etc/wiimedia/newsong.dat");
	if(list.is_open())
	{
		mpd_sendAddCommand(conn, chosensong.c_str());
		
		mpd_finishCommand(conn);
        	if(conn->error) 
		{
        		fprintf(stderr,"%s\n",conn->errorStr);
        	        mpd_closeConnection(conn);
        	        return;
        	}
        	list << "New Song Added:\n";	
		list << chosensong;
		list.close();
		gmessage("1", "New Song Added", "New Song Added", 1, "/etc/wiimedia/newsong.dat");
	}
	else
	{
                gmessage("2", "Unable To Get Song", "Unable To Get Song", 0, "Unable To Get Song");
	}

        if(conn->error) 
	{
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

        mpd_finishCommand(conn);
        if(conn->error) 
	{
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

	mpd_closeConnection(conn);
	return;
}

// Browse Specific Artists 
void specificartist(int selection)
{
	ofstream list;
	ifstream inlist;
	string artistlist;
       	string line;
        int nlines=0;
	
        inlist.open("/etc/wiimedia/artistlist.dat");
        
        while( getline( inlist, line ) )
        {
         	if(  nlines == selection )
         	{
                  	artistlist = line;
                }
                nlines++;
        }

	inlist.close();

	currentartist=artistlist;

	if(selection > nlines)
	{
		artistselection=0;
		specificartist(artistselection);
		return;
	}
	else if(selection < 0)
	{
		artistselection=0;
		specificartist(artistselection);
		return;
	}
	
	// Open File And Write Artist
        list.open("/etc/wiimedia/specificartist.dat");
	if(list.is_open())
	{	
		list << artistlist;
		list.close();
		// List Artist
                gmessage("1", "Artist", "Artist", 1, "/etc/wiimedia/specificartist.dat");
	}
	else
	{
                gmessage("1", "Unable To Get Artist", "Unable To Get Artist", 0, "Unable To Get Artist");
	}
	
	return;
}

// Outputs Artist List For Each Letter Of The Alphabet
void alphabet(int alpha)
{
	ofstream list;
        char * artist;
	string artistlist="";

        // Music Player Daemon Connection Information 
        mpd_Connection * conn;
        
	char *hostname = getenv("MPD_HOST");
        char *port = getenv("MPD_PORT");

        // Set to defaults if undefined 
        if(hostname == NULL)
                hostname = "localhost";
        if(port == NULL)
                port = "6600";

        conn = mpd_newConnection(hostname,atoi(port),10);

        if(conn->error)
        {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

        mpd_Status * status;

        mpd_sendCommandListOkBegin(conn);
        mpd_sendStatusCommand(conn);
        mpd_sendCurrentSongCommand(conn);
        mpd_sendCommandListEnd(conn);

        if((status = mpd_getStatus(conn))==NULL)
        {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

        mpd_finishCommand(conn);

	if(conn->error)
        {
                fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

        mpd_sendListCommand(conn,MPD_TABLE_ARTIST,NULL);
        if(conn->error) 
	{
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }
	
	// Lists every artist for current letter, then puts to a single string for output to a text file
        while((artist = mpd_getNextArtist(conn))) 
	{
                if(artist[0]==alphatable[alpha])
		{
			artistlist+=artist;
			artistlist+="\n";
		}
                free(artist);
        }

	// Open File And Write Artists 
        list.open("/etc/wiimedia/artistlist.dat");
	if(list.is_open())
	{	
		list << artistlist;
		list.close();
		artistselection=0;
		// List Artists 
                gmessage("2", "Artists", "Artists", 1, "/etc/wiimedia/artistlist.dat");
	}
	else
	{
                gmessage("2", "Unable To List Artist", "Unable To List Artists", 0, "Unable To List Artists");
	}

        if(conn->error) 
	{
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

        mpd_finishCommand(conn);
        if(conn->error) 
	{
        	fprintf(stderr,"%s\n",conn->errorStr);
                mpd_closeConnection(conn);
                return;
        }

	mpd_closeConnection(conn);
	return;
}

/* Prototype cwiid_callback with cwiid_callback_t, define it with the actual
 * type - this will cause a compile error (rather than some undefined bizarre
 * behavior) if cwiid_callback_t changes */

/* cwiid_mesg_callback_t has undergone a few changes lately, hopefully this
 * will be the last.  Some programs need to know which messages were received
 * simultaneously (e.g. for correlating accelerometer and IR data), and the
 * sequence number mechanism used previously proved cumbersome, so we just
 * pass an array of messages, all of which were received at the same time.
 * The id is to distinguish between multiple wiimotes using the same callback.
 * */
void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count, union cwiid_mesg mesg[], struct timespec *timestamp)
{
	int i, j;
	int valid_source;

	timestamp=timestamp;

	for (i=0; i < mesg_count; i++)
	{
		switch (mesg[i].type) 
		{
			case CWIID_MESG_STATUS:
				printf("Status Report: battery=%d extension=",
			       mesg[i].status_mesg.battery);
				switch (mesg[i].status_mesg.ext_type) 
				{
					case CWIID_EXT_NONE:
						printf("none");
						break;
				case CWIID_EXT_NUNCHUK:
					printf("Nunchuk");
					break;
				case CWIID_EXT_CLASSIC:
					printf("Classic Controller");
					break;
				default:
					printf("Unknown Extension");
					break;
				}
				printf("\n");
				break;
			case CWIID_MESG_BTN:
				printf("Button Report: %.4X\n", mesg[i].btn_mesg.buttons);
				break;
			case CWIID_MESG_ACC:
				printf("Acc Report: x=%d, y=%d, z=%d\n",
                	   		mesg[i].acc_mesg.acc[CWIID_X],
				       	mesg[i].acc_mesg.acc[CWIID_Y],
				       	mesg[i].acc_mesg.acc[CWIID_Z]);
				break;
			case CWIID_MESG_IR:
				printf("IR Report: ");
				valid_source = 0;
				for (j = 0; j < CWIID_IR_SRC_COUNT; j++) 
				{
					if (mesg[i].ir_mesg.src[j].valid) 
					{
						valid_source = 1;
						printf("(%d,%d) ", mesg[i].ir_mesg.src[j].pos[CWIID_X],
					                   mesg[i].ir_mesg.src[j].pos[CWIID_Y]);
					}
				}
				if (!valid_source) 
				{
					printf("no sources detected");
				}
				printf("\n");
				break;
			case CWIID_MESG_NUNCHUK:
				printf("Nunchuk Report: btns=%.2X stick=(%d,%d) acc.x=%d acc.y=%d "
				       "acc.z=%d\n", mesg[i].nunchuk_mesg.buttons,
				       mesg[i].nunchuk_mesg.stick[CWIID_X],
				       mesg[i].nunchuk_mesg.stick[CWIID_Y],
				       mesg[i].nunchuk_mesg.acc[CWIID_X],
				       mesg[i].nunchuk_mesg.acc[CWIID_Y],
				       mesg[i].nunchuk_mesg.acc[CWIID_Z]);
				break;
			case CWIID_MESG_CLASSIC:
				printf("Classic Report: btns=%.4X l_stick=(%d,%d) r_stick=(%d,%d) "
				       "l=%d r=%d\n", mesg[i].classic_mesg.buttons,
				       mesg[i].classic_mesg.l_stick[CWIID_X],
				       mesg[i].classic_mesg.l_stick[CWIID_Y],
				       mesg[i].classic_mesg.r_stick[CWIID_X],
				       mesg[i].classic_mesg.r_stick[CWIID_Y],
				       mesg[i].classic_mesg.l, mesg[i].classic_mesg.r);
				break;
			case CWIID_MESG_ERROR:
				if (cwiid_close(wiimote)) 
				{
					fprintf(stderr, "Error on wiimote disconnect\n");
					exit(-1);
				}
				exit(0);
				break;
			default:
				printf("Unknown Report");
				break;
		}
	}
}

// Function to push battery level to LEDs 
void led_battery_state(cwiid_wiimote_t *wiimote, struct cwiid_state *state)
{
        unsigned char led_state = 0;
        int batlev = (100 * state->battery / CWIID_BATTERY_MAX);

	if(batlev > 75)	{
 		if (state->led & CWIID_LED1_ON)	{ } else {
			toggle_bit(led_state, CWIID_LED1_ON);
                        set_led_state(wiimote, led_state);
		}
        	if (state->led & CWIID_LED2_ON)	{ } else {
                        toggle_bit(led_state, CWIID_LED2_ON);
                        set_led_state(wiimote, led_state);

		}
        	if (state->led & CWIID_LED3_ON)	{ } else {
                        toggle_bit(led_state, CWIID_LED3_ON);
                        set_led_state(wiimote, led_state);
		}
        	if (state->led & CWIID_LED4_ON)	{ } else {
                        toggle_bit(led_state, CWIID_LED4_ON);
                        set_led_state(wiimote, led_state);
		}
	}
        if(batlev > 50 && batlev <= 75) {
                if (state->led & CWIID_LED1_ON) { } else {
                        toggle_bit(led_state, CWIID_LED1_ON);
                        set_led_state(wiimote, led_state);
                }
                if (state->led & CWIID_LED2_ON) { } else {
                        toggle_bit(led_state, CWIID_LED2_ON);
                        set_led_state(wiimote, led_state);

                }
                if (state->led & CWIID_LED3_ON) { } else {
                        toggle_bit(led_state, CWIID_LED3_ON);
                        set_led_state(wiimote, led_state);
                }
                if (state->led & CWIID_LED4_ON) {
                        toggle_bit(led_state, CWIID_LED4_ON);
                        set_led_state(wiimote, led_state);
                } else { }
        }
        if(batlev > 25 && batlev <= 50) {
                if (state->led & CWIID_LED1_ON) { } else {
                        toggle_bit(led_state, CWIID_LED1_ON);
                        set_led_state(wiimote, led_state);
                }
                if (state->led & CWIID_LED2_ON) { } else {
                        toggle_bit(led_state, CWIID_LED2_ON);
                        set_led_state(wiimote, led_state);

                }
                if (state->led & CWIID_LED3_ON) {
                        toggle_bit(led_state, CWIID_LED3_ON);
                        set_led_state(wiimote, led_state);
                } else { }
                if (state->led & CWIID_LED4_ON)	{
                        toggle_bit(led_state, CWIID_LED4_ON);
                        set_led_state(wiimote, led_state);
                } else { }
        }
        if(batlev > 0 && batlev <= 25) {
                if (state->led & CWIID_LED1_ON) { } else {
                        toggle_bit(led_state, CWIID_LED1_ON);
                        set_led_state(wiimote, led_state);
                }
                if (state->led & CWIID_LED2_ON) {
                        toggle_bit(led_state, CWIID_LED2_ON);
                        set_led_state(wiimote, led_state);

                } else { }
                if (state->led & CWIID_LED3_ON) {
                        toggle_bit(led_state, CWIID_LED3_ON);
                        set_led_state(wiimote, led_state);
                } else { }
                if (state->led & CWIID_LED4_ON) {
                        toggle_bit(led_state, CWIID_LED4_ON);
                        set_led_state(wiimote, led_state);
                } else { }
        }
        if(batlev <= 0) {
                if (state->led & CWIID_LED1_ON) {
                        toggle_bit(led_state, CWIID_LED1_ON);
                        set_led_state(wiimote, led_state);
                } else { }
                if (state->led & CWIID_LED2_ON) {
                        toggle_bit(led_state, CWIID_LED2_ON);
                        set_led_state(wiimote, led_state);
                } else { }
                if (state->led & CWIID_LED3_ON) {
                        toggle_bit(led_state, CWIID_LED3_ON);
                        set_led_state(wiimote, led_state);
                } else { }
                if (state->led & CWIID_LED4_ON) {
                        toggle_bit(led_state, CWIID_LED4_ON);
                        set_led_state(wiimote, led_state);
                } else { }
        }
}

// Popup Window With Inputted Information
void gmessage(char * timeout, char * name, char * title, int file, char * output)
{
	string message="gmessage ";
	
	message+=" -timeout ";
	message+=timeout;
	message+=" -center -bg black -fg white";
	message+=" -name \"";
	message+=name;
	message+="\" ";
	message+=" -title \"";
        message+=title;  
        message+="\"";
	message+=" -borderless";
	if(file==1)
		message+=" -file";
	message+=" \"";
	message+=output;
	message+="\"";

	system(message.c_str());
	return;
}

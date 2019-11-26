/****************************************************************************
 ************                                                    ************
 ************                 M 3 6 _ R E A D                    ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ds
 *
 *  Description: Universal tool for read/write M27 channels
 *
 *     Required: libraries: mdis_api, usr_oss, usr_utl
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Copyright 1998-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/m27_drv.h>

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define CH_NUMBER	16		/* nr of device channels */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void usage(void);
static void PrintError(char *info);

/********************************* usage ************************************
 *
 *  Description: Print program usage
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
void usage(void)
{
	printf("Usage:    m27_rw [<opts>] <device> [<opts>]\n");
	printf("Function: Read/write M27 channels 0..15\n");
	printf("Options:\n");
	printf("  device         device name                             [none]\n");
	printf("  -g=<chan>      read     : get state of channel         [none]\n");
	printf("  -s=<chan>      write    : set channel                  [none]\n");			
	printf("  -r=<chan>      write    : reset channel                [none]\n");			
	printf("  -G=<chan>      getblock : get state of channel 0..chan [none]\n");			
	printf("  -S=<string>    setblock : set/reset channels           [none]\n");			
	printf("                            eg. -S=rss -> reset ch0\n");
	printf("                                       -> set   ch1\n");
	printf("                                       -> set   ch2\n");
	printf("  -t (requires option -S) : toggle specified channels    [none]\n");
	printf("  Note: If you specify only the device name, the path will be held open.\n");
	printf("\n");
	printf("Copyright 1998-2019, MEN Mikro Elektronik GmbH\n%s\n", IdentString);
}

/********************************* main *************************************
 *
 *  Description: Program main function
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc,argv	argument counter, data ..
 *  Output.....: return	    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(argc,argv)
int  argc;
char *argv[];
{
    MDIS_PATH path;
	int32	value, gotsize, n, ch;
	int8	get, set, reset, getblk, setblk, toggle, hold=0;
	u_int8  inbuf[20], outbuf[20];
	char	c, *device, *str, *ptr, *errstr;
	char	buf[40];

	/*--------------------+
    |  check arguments    |
    +--------------------*/
	if ((errstr = UTL_ILLIOPT("g=s=r=G=S=t?", buf))) {	/* check args */
		printf("*** %s\n", errstr);
		return(1);
	}

	if (UTL_TSTOPT("?")) {						/* help requested ? */
		usage();
		return(1);
	}

	/*--------------------+
    |  get arguments      |
    +--------------------*/
	for (device=NULL, n=1; n<argc; n++)
		if (*argv[n] != '-') {
			device = argv[n];
			break;
		}

	if (!device) {
		usage();
		return(1);
	}

	get     = ((str = UTL_TSTOPT("g=")) ? atoi(str) : -1);
	set     = ((str = UTL_TSTOPT("s=")) ? atoi(str) : -1);
	reset   = ((str = UTL_TSTOPT("r=")) ? atoi(str) : -1);
	getblk  = ((str = UTL_TSTOPT("G=")) ? atoi(str) : -1);
	setblk  = -1;
	
	if ( (str = UTL_TSTOPT("S=")) ) {
		ptr = str;
		/* fill outbuf */
		while ( (c = *ptr++) ) {
			setblk++;
			outbuf[setblk] = ( (c == 's') ? 1 : 0);
		}
	}

	/* option conflict ? */
	if ( (toggle = (UTL_TSTOPT("t") ? 1 : 0)) && (setblk == -1) ) {
		printf ("*** option '-t' requires option '-S='\n");
		return(1);
	}

	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

	/*--------------------+
    |  set                |
    +--------------------*/
	if (set >= 0) {
		printf("set channel %d\n\n", set);
		/* set current channel */
		if ((M_setstat(path, M_MK_CH_CURRENT, set)) < 0) {
			PrintError("setstat M_MK_CH_CURRENT");
			goto abort;
		}
		/* set channel */
		if ((M_write(path, 1)) < 0) {
			PrintError("write");
			goto abort;
		}
		hold=1;
	}

	/*--------------------+
    |  reset              |
    +--------------------*/
	if (reset >= 0) {
		printf("reset channel %d\n\n", reset);
		/* set current channel */
		if ((M_setstat(path, M_MK_CH_CURRENT, reset)) < 0) {
			PrintError("setstat M_MK_CH_CURRENT");
			goto abort;
		}
		/* reset channel */
		if ((M_write(path, 0)) < 0) {
			PrintError("write");
			goto abort;
		}
	}

	/*--------------------+
    |  get                |
    +--------------------*/
	if (get >= 0) {
		printf("get state of channel %d\n", get);
		/* set current channel */
		if ((M_setstat(path, M_MK_CH_CURRENT, get)) < 0) {
			PrintError("setstat M_MK_CH_CURRENT");
			goto abort;
		}
		/* read from channel */
		if ((M_read(path,&value)) < 0) {
			PrintError("read");
			goto abort;
		}
		printf("read value: %d (%s)\n\n", (int)value, (value ? "set" : "reset"));
	}

	/*--------------------+
    |  setblock           |
    +--------------------*/
	if (setblk >= 0) {
		printf("set state of channel 0..%d\n", setblk );
		printf("channel:   00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15\n");
		printf("state:     ");
		for (ch=0; ch<CH_NUMBER; ch++) {
			if (ch <= setblk)
				printf("%s", (outbuf[ch] ? " S ":" R "));
			else
				printf(" - ");
		}
		printf("\n\n");

		/* set all channels */
		if ((gotsize = M_setblock(path, outbuf, setblk+1)) < 0) {
			PrintError("setblock");
			goto abort;
		}
		hold=1;
	}

	/*--------------------+
    |  getblock           |
    +--------------------*/
	if (getblk >= 0) {
		printf("get state of channel 0..%d\n", getblk );
		/* read all channels */
		if ((gotsize = M_getblock(path, inbuf, getblk+1)) < 0) {
			PrintError("getblock");
			goto abort;
		}
		printf("channel:   00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15\n");
		printf("state:     ");
		for (ch=0; ch<CH_NUMBER; ch++) {
			if (ch <= getblk)
				printf("%s", (inbuf[ch] ? " S ":" R "));
			else
				printf(" - ");
		}
		printf("\n\n");
	}

	/*--------------------+
    |  toggle             |
    +--------------------*/
	if (toggle) {
		printf("toggle all channels - press any key to abort\n" );

		/* build inverse */
		for (ch=0; ch<CH_NUMBER; ch++)
			inbuf[ch] = ( outbuf[ch] ? 0 : 1 );  

		printf("channel:   00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15\n");

		/* endless loop */
		do {
			printf("state:     ");
			for (ch=0; ch<CH_NUMBER; ch++) {
				if (ch <= setblk)
					printf("%s", (outbuf[ch] ? " S ":" R "));
				else
					printf(" - ");
			}
			printf("\n");

			/* invert all channels */
			if ((gotsize = M_setblock(path, outbuf, setblk+1)) < 0) {
				PrintError("setblock");
				goto abort;
			}

			/* delay 1000ms */
			UOS_Delay(1000);

			printf("state:     ");
			for (ch=0; ch<CH_NUMBER; ch++) {
				if (ch <= setblk)
					printf("%s", (inbuf[ch] ? " S ":" R "));
				else
					printf(" - ");
			}
			printf("\n");

			/* set all channels */
			if ((gotsize = M_setblock(path, inbuf, setblk+1)) < 0) {
				PrintError("setblock");
				goto abort;
			}

			/* delay 1000ms */
			UOS_Delay(1000);

		} while( UOS_KeyPressed() == -1 );

	}

	if ( hold || (argc < 3) ) {
		printf("press any key...\n");
		UOS_KeyWait();
	}

	/*--------------------+
    |  cleanup            |
    +--------------------*/
	abort:
	if (M_close(path) < 0)
		PrintError("close");

	return(0);
}

/********************************* PrintError ********************************
 *
 *  Description: Print MDIS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/

void PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}





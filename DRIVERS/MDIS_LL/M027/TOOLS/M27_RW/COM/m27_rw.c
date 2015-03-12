/****************************************************************************
 ************                                                    ************
 ************                 M 3 6 _ R E A D                    ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ds
 *        $Date: 2010/03/10 14:14:57 $
 *    $Revision: 1.4 $
 *
 *  Description: Universal tool for read/write M27 channels
 *
 *     Required: libraries: mdis_api, usr_oss, usr_utl
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m27_rw.c,v $
 * Revision 1.4  2010/03/10 14:14:57  amorbach
 * R: Porting to MDIS5
 * M: changed according to MDIS Porting Guide 0.8
 *
 * Revision 1.3  2004/05/03 14:37:06  cs
 * Minor changes for MDIS4/2004 conformity
 *   changed function prototypes to static
 *   cosmetics
 *
 * Revision 1.2  1998/12/08 12:16:49  see
 * usage: wrong example fixed
 * missing brackets added
 * inbuf/outbuf must be u_int8
 *
 * Revision 1.1  1998/12/07 16:00:04  Schmidt
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
static const char RCSid[]="$Header: /dd2/CVSR/COM/DRIVERS/MDIS_LL/M027/TOOLS/M27_RW/COM/m27_rw.c,v 1.4 2010/03/10 14:14:57 amorbach Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/m27_drv.h>

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
	printf("(c) 1998 by MEN mikro elektronik GmbH\n\n");
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





/****************************************************************************
 ************                                                    ************
 ************                   M27_SIMP                        ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ds
 *        $Date: 2010/03/10 14:15:00 $
 *    $Revision: 1.4 $
 *
 *  Description: Simple example program for the M27 driver 
 *                      
 *     Required: libraries: mdis_api, usr_oss
 *     Switches: NO_MAIN_FUNC	(for systems with one namespace)
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 1998-2019, MEN Mikro Elektronik GmbH
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
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#include <MEN/usr_oss.h>
#include <MEN/m27_drv.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define CH_NUMBER	16		/* nr of device channels */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int32 _m27_simp(char *device);
static void PrintError(char *info);

/********************************* main *************************************
 *
 *  Description: MAIN entry (not used in systems with one namespace)
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc, argv		command line arguments/counter
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
#ifndef NO_MAIN_FUNC
int main(int argc, char *argv[])
{
	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m27_simp <device> <chan>\n");
		printf("Function: M27 example for read/write\n");
		printf("Options:\n");
		printf("    device       device name\n");
		printf("\n");
		return(1);
	}

	_m27_simp(argv[1]);
	return(0);
}
#endif


/******************************* _m27_simp **********************************
 *
 *  Description: Example (directly called in systems with one namespace)
 *               - open path
 *               - toggle each channel
 *               - set all channels
 *               - read all channels
 *               - close path
 *
 *---------------------------------------------------------------------------
 *  Input......: device    device name
 *               chan      channel number
 *  Output.....: return    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int32 _m27_simp(char *device)
{
	MDIS_PATH path=0;
	u_int8	blkdata[CH_NUMBER] = {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
	u_int8	buf[CH_NUMBER];
	u_int32 ch;
	int32 gotsize;


	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

	/*--------------------+
    |  toggle channels    |
    +--------------------*/
	printf("toggle channels\n");
	printf("channel:   00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15\n");
	printf("Set/Reset: ");
	for (ch=0; ch<CH_NUMBER; ch++) {
		/* set channel number */
		if ((M_setstat(path, M_MK_CH_CURRENT, ch)) < 0) {
			PrintError("setstat M_MK_CH_CURRENT");
			goto abort;
		}
		printf("S"); fflush(stdout);
		/* set channel */
		if ((M_write(path, 1)) < 0) {
			PrintError("write");
			goto abort;
		}
		/* delay 1000ms */
		UOS_Delay(1000);
		printf("R "); fflush(stdout);
		/* reset channel */
		if ((M_write(path, 0)) < 0) {
			PrintError("write");
			goto abort;
		}
	}
	printf("\n\n");

	/*--------------------+
    |  set all channels   |
    +--------------------*/
	printf("set all channels alternately (1,0,1,..)\n\n");
	/* set all channels */
	if ((gotsize = M_setblock(path, blkdata, CH_NUMBER)) < 0) {
		PrintError("setblock");
		goto abort;
	}

	/*--------------------+
    |  read all channels  |
    +--------------------*/
	printf("read all channels\n");
	/* read all channels */
	if ((gotsize = M_getblock(path, buf, CH_NUMBER)) < 0) {
		PrintError("getblock");
	}
	printf("channel:   00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15\n");
	printf("read data: ");
	for (ch=0; ch<CH_NUMBER; ch++)
		printf("%s", (buf[ch] ? " S ":" R "));
	printf("\n\npress any key...\n");
	UOS_KeyWait();

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



/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m27_drv.h
 *
 *       Author: ds
 *
 *  Description: Header file for M27 driver
 *               - M27 specific status codes
 *               - M27 function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
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

#ifndef _M27_DRV_H
#define _M27_DRV_H

#ifdef __cplusplus
      extern "C" {
#endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* none */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* M27 specific status codes (STD) */        /* S,G: S=setstat, G=getstat */
/*#define M27_XXX           M_DEV_OF+0x00 */     /* G,S: xxx */

/* M27 specific status codes (BLK) */          /* S,G: S=setstat, G=getstat */
/*#define M27_BLK_XXX       M_DEV_BLK_OF+0x00 */  /* G,S: xxx */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_

#ifdef _ONE_NAMESPACE_PER_DRIVER_
#	define M27_GetEntry LL_GetEntry
#else
#	ifdef ID_SW
#		define M27_GetEntry M27_SW_GetEntry
#	endif
	extern void M27_GetEntry(LL_ENTRY* drvP);
#endif

#endif /* _LL_DRV_ */


/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
    /* we have an MDIS4 men_types.h and mdis_api.h included */
    /* only 32bit compatibility needed!                     */
    #define INT32_OR_64  int32
        #define U_INT32_OR_64 u_int32
    typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */

#ifdef __cplusplus
      }
#endif

#endif /* _M27_DRV_H */


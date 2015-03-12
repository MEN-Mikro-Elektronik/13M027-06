/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m27_drv.h
 *
 *       Author: ds
 *        $Date: 2010/03/10 14:15:09 $
 *    $Revision: 2.4 $
 *
 *  Description: Header file for M27 driver
 *               - M27 specific status codes
 *               - M27 function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m27_drv.h,v $
 * Revision 2.4  2010/03/10 14:15:09  amorbach
 * R: driver ported to MDIS5, new MDIS_API and men_typs
 * M: for backward compatibility to MDIS4 optionally define new types
 *
 * Revision 2.3  2004/05/03 14:37:25  cs
 * Minor changes for MDIS4/2004 conformity
 * added swapped access support
 * moved function prototypes to m27_drv.c (static)
 *
 * Revision 2.2  1999/07/22 18:03:26  Franke
 * cosmetics
 *
 * Revision 2.1  1998/12/07 16:00:31  Schmidt
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

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


/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m27_drv.c
 *      Project: M27 module driver (MDIS V4.x)
 *
 *       Author: dieter.pfeuffer@men.de
 *        $Date: 2010/03/10 14:14:55 $
 *    $Revision: 1.3 $
 *
 *  Description: Low-level driver for M27, M28 and M81 M-Modules
 *
 *               The M27/M28/M81 module is a 16-bit binary output module.
 *               The module does not support interrupts.
 *
 *               The driver handles the output ports as 16 channels.
 *               The state of the channels can be read.
 *   
 *               The driver does not support buffers.
 *               
 *     Required: ---
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
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

#define _NO_LL_HANDLE		/* ll_defs.h: don't define LL_HANDLE struct */

#include <MEN/men_typs.h>   /* system dependend definitions   */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/modcom.h>     /* id prom functions              */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low level driver definitions   */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general */
#define CH_NUMBER			16			/* nr of device channels */
#define USE_IRQ				FALSE		/* interrupt required  */
#define ADDRSPACE_COUNT		1			/* nr of required address spaces */
#define ADDRSPACE_SIZE		256			/* size of address space */
#define MOD_ID_MAGIC		0x5346      /* id prom magic word */
#define MOD_ID_SIZE			128			/* id prom size [bytes] */
#define MOD_ID_1			27			/* id prom module id */
#define MOD_ID_2			28			/* id prom module id */
#define MOD_ID_3			81			/* id prom module id */

/* debug settings */
#define DBG_MYLEVEL			llHdl->dbgLevel
#define DBH					llHdl->dbgHdl

/* register offsets */
#define OUTPUT_REG			0x00		/* output register */

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* ll handle */
typedef struct {
	/* general */
    int32           memAlloc;		/* size allocated for the handle */
    OSS_HANDLE      *osHdl;         /* oss handle */
    OSS_IRQ_HANDLE  *irqHdl;        /* irq handle */
    DESC_HANDLE     *descHdl;       /* desc handle */
    MACCESS         ma;             /* hw access handle */
	MDIS_IDENT_FUNCT_TBL idFuncTbl;	/* id function table */
	/* debug */
    u_int32         dbgLevel;		/* debug level */
	DBG_HANDLE      *dbgHdl;        /* debug handle */
	/* misc */
    u_int32         idCheck;		/* id check enabled */
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>   /* low level driver jumptable  */
#include <MEN/m27_drv.h>	/* M27 driver header file */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static char* Ident( void );
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);

static int32 M27_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
                      MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
                      OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 M27_Exit(LL_HANDLE **llHdlP );
static int32 M27_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 M27_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 M27_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 M27_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64P);
static int32 M27_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
                           int32 *nbrRdBytesP);
static int32 M27_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
                            int32 *nbrWrBytesP);
static int32 M27_Irq(LL_HANDLE *llHdl );
static int32 M27_Info(int32 infoType, ... );

/**************************** M27_GetEntry *********************************
 *
 *  Description:  Initialize drivers jump table
 *
 *---------------------------------------------------------------------------
 *  Input......:  ---
 *  Output.....:  drvP  pointer to the initialized jump table structure
 *  Globals....:  ---
 ****************************************************************************/
#ifdef _ONE_NAMESPACE_PER_DRIVER_
    void LL_GetEntry( LL_ENTRY* drvP )
#else
    void M27_GetEntry( LL_ENTRY* drvP )
#endif
{
    drvP->init        = M27_Init;
    drvP->exit        = M27_Exit;
    drvP->read        = M27_Read;
    drvP->write       = M27_Write;
    drvP->blockRead   = M27_BlockRead;
    drvP->blockWrite  = M27_BlockWrite;
    drvP->setStat     = M27_SetStat;
    drvP->getStat     = M27_GetStat;
    drvP->irq         = M27_Irq;
    drvP->info        = M27_Info;
}

/******************************** M27_Init ***********************************
 *
 *  Description:  Allocate and return ll handle, initialize hardware
 * 
 *                The function resets all channels.
 *
 *                The following descriptor keys are used:
 *
 *                Deskriptor key        Default          Range
 *                --------------------  ---------------  -------------
 *                DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  see dbg.h
 *                DEBUG_LEVEL           OSS_DBG_DEFAULT  see dbg.h
 *                ID_CHECK              1                0..1 
 *
 *---------------------------------------------------------------------------
 *  Input......:  descSpec   pointer to descriptor data
 *                osHdl      oss handle
 *                ma         hw access handle
 *                devSemHdl  device semaphore handle
 *                irqHdl     irq handle
 *  Output.....:  llHdlP     ptr to low level driver handle
 *                return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
    MACCESS         *ma,
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **llHdlP
)
{
    LL_HANDLE *llHdl = NULL;
    u_int32 gotsize;
    int32 error;
    u_int32 value;

    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
	/* alloc */
    if ((*llHdlP = llHdl = (LL_HANDLE*)
		 OSS_MemGet(osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

	/* clear */
    OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

	/* init */
    llHdl->memAlloc   = gotsize;
    llHdl->osHdl      = osHdl;
    llHdl->irqHdl     = irqHdl;
    llHdl->ma		  = *ma;

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
	/* drivers ident function */
	llHdl->idFuncTbl.idCall[0].identCall = Ident;
	/* libraries ident functions */
	llHdl->idFuncTbl.idCall[1].identCall = DESC_Ident;
	llHdl->idFuncTbl.idCall[2].identCall = OSS_Ident;
	/* terminator */
	llHdl->idFuncTbl.idCall[3].identCall = NULL;

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
	DBG_MYLEVEL = OSS_DBG_DEFAULT;	/* set OS specific debug level */
	DBGINIT((NULL,&DBH));

    DBGWRT_1((DBH, "LL - M27_Init\n"));

    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
	/* prepare access */
    if ((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
		return( Cleanup(llHdl,error) );

    /* DEBUG_LEVEL_DESC */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT, 
								&value, "DEBUG_LEVEL_DESC")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	DESC_DbgLevelSet(llHdl->descHdl, value);	/* set level */

    /* DEBUG_LEVEL */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT, 
								&llHdl->dbgLevel, "DEBUG_LEVEL")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    /* ID_CHECK */
    if ((error = DESC_GetUInt32(llHdl->descHdl, TRUE, 
								&llHdl->idCheck, "ID_CHECK")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    /*------------------------------+
    |  check module id              |
    +------------------------------*/
	if (llHdl->idCheck) {
		int modIdMagic = m_read((U_INT32_OR_64)llHdl->ma, 0);
		int modId      = m_read((U_INT32_OR_64)llHdl->ma, 1);

		if (modIdMagic != MOD_ID_MAGIC) {
			DBGWRT_ERR((DBH," *** M27_Init: illegal magic=0x%04x\n",modIdMagic));
			error = ERR_LL_ILL_ID;
			return( Cleanup(llHdl,error) );
		}
		if ( (modId != MOD_ID_1) &&
			 (modId != MOD_ID_2) &&
			 (modId != MOD_ID_3) ) {
			DBGWRT_ERR((DBH," *** M27_Init: illegal id=%d\n",modId));
			error = ERR_LL_ILL_ID;
			return( Cleanup(llHdl,error) );
		}
	}

    /*------------------------------+
    |  init hardware                |
    +------------------------------*/
	/* reset all channels */
	MWRITE_D16( llHdl->ma, OUTPUT_REG, 0x00 );

	return(ERR_SUCCESS);
}

/****************************** M27_Exit *************************************
 *
 *  Description:  De-initialize hardware and cleanup memory
 *
 *                The function resets all channels.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdlP  	ptr to low level driver handle
 *  Output.....:  return    success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_Exit(
   LL_HANDLE    **llHdlP
)
{
    LL_HANDLE *llHdl = *llHdlP;
	int32 error = 0;

    DBGWRT_1((DBH, "LL - M27_Exit\n"));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/
	/* reset all channels */
	MWRITE_D16( llHdl->ma, OUTPUT_REG, 0x00 );

    /*------------------------------+
    |  cleanup memory               |
    +------------------------------*/
	error = Cleanup(llHdl,error);

	return(error);
}

/****************************** M27_Read *************************************
 *
 *  Description:  Reads value from device
 *
 *                The function reads the state of the current channel.
 *                Bit 0 of '*valueP' specify the state (0=reset, 1=set).
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    ll handle
 *                ch       current channel
 *  Output.....:  valueP   read value
 *                return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_Read(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 *valueP
)
{
    DBGWRT_1((DBH, "LL - M27_Read: ch=%d\n",ch));

	/* read channel state */
	*valueP = (MREAD_D16(llHdl->ma, OUTPUT_REG) >> ch) & 0x01;
	
	return(ERR_SUCCESS);
}

/****************************** M27_Write ************************************
 *
 *  Description:  Write value to device
 *
 *                The function sets the current channel, if value<>0.
 *                The function resets the current channel, if value=0.
 *                
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    ll handle
 *                ch       current channel
 *                value    value to write 
 *  Output.....:  return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_Write(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 value
)
{
    DBGWRT_1((DBH, "LL - M27_Write: ch=%d\n",ch));

	/* set channel */
	if (value)
		MSETMASK_D16( llHdl->ma, OUTPUT_REG, 0x01 << ch );
	/* reset channel */
	else
		MCLRMASK_D16( llHdl->ma, OUTPUT_REG, 0x01 << ch );
	
	return(ERR_SUCCESS);
}

/****************************** M27_SetStat **********************************
 *
 *  Description:  Set driver status
 *
 *                Following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level         see oss.h
 *                M_LL_CH_DIR          direction of curr chan     M_CH_INOUT
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl      ll handle
 *                code       status code
 *                ch         current channel
 *                value      data or
 *                           ptr to block data struct (M_SG_BLOCK)  (*)
 *                (*) = for block status codes
 *  Output.....:  return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_SetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 value32_or_64
)
{
	int32 error = ERR_SUCCESS;
    int32 value = (int32)value32_or_64;	    /* 32bit value */
    /* INT32_OR_64 valueP = value32_or_64;     stores 32/64bit pointer */

    DBGWRT_1((DBH, "LL - M27_SetStat: ch=%d code=0x%04x value=0x%x\n",
			  ch,code,value));

    switch(code) {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            llHdl->dbgLevel = value;
            break;
        /*--------------------------+
        |  enable interrupts        |
        +--------------------------*/
        case M_MK_IRQ_ENABLE:
			error = ERR_LL_UNK_CODE;	/* say: not supported */
            break;
        /*--------------------------+
        |  set irq counter          |
        +--------------------------*/
        case M_MK_IRQ_COUNT:
            error = ERR_LL_UNK_CODE;	/* say: not supported */
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
			switch(value) {
				case M_CH_INOUT:
					/* do nothing */
					break;
				default:
					error = ERR_LL_ILL_DIR;
			}

            break;
        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/****************************** M27_GetStat **********************************
 *
 *  Description:  Get driver status
 *
 *                Following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level         see oss.h
 *                M_LL_CH_NUMBER       number of channels         16
 *                M_LL_CH_DIR          direction of curr chan     M_CH_INOUT
 *                M_LL_CH_LEN          length of curr chan [bits] 1
 *                M_LL_CH_TYP          description of curr chan   M_CH_BINARY
 *                M_LL_ID_CHECK        eeprom is checked          0..1
 *                M_LL_ID_SIZE         eeprom size [bytes]        128
 *                M_LL_BLK_ID_DATA     eeprom raw data            -
 *                M_MK_BLK_REV_ID      ident function table ptr   -
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl      ll handle
 *                code       status code
 *                ch         current channel
 *                valueP     ptr to block data struct (M_SG_BLOCK)  (*) 
 *                (*) = for block status codes
 *  Output.....:  valueP     data ptr or
 *                           ptr to block data struct (M_SG_BLOCK)  (*) 
 *                return     success (0) or error code
 *                (*) = for block status codes
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_GetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 *value32_or_64P
)
{
    int32 *valueP = (int32*)value32_or_64P;	            /* pointer to 32bit value  */
    INT32_OR_64	*value64P = value32_or_64P;		 		/* stores 32/64bit pointer  */
    M_SG_BLOCK *blk = (M_SG_BLOCK*)value32_or_64P; 	    /* stores block struct pointer */

	int32 error = ERR_SUCCESS;

    DBGWRT_1((DBH, "LL - M27_GetStat: ch=%d code=0x%04x\n",
			  ch,code));

    switch(code)
    {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            *valueP = llHdl->dbgLevel;
            break;
        /*--------------------------+
        |  nr of channels           |
        +--------------------------*/
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            *valueP = M_CH_INOUT;
            break;
        /*--------------------------+
        |  channel length [bits]    |
        +--------------------------*/
        case M_LL_CH_LEN:
            *valueP = 1;
            break;
        /*--------------------------+
        |  channel type info        |
        +--------------------------*/
        case M_LL_CH_TYP:
            *valueP = M_CH_BINARY;
            break;
        /*--------------------------+
        |  irq counter              |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            error = ERR_LL_UNK_CODE;	/* say: not supported */
            break;
        /*--------------------------+
        |  id prom check enabled    |
        +--------------------------*/
        case M_LL_ID_CHECK:
            *valueP = llHdl->idCheck;
            break;
        /*--------------------------+
        |   id prom size            |
        +--------------------------*/
        case M_LL_ID_SIZE:
            *valueP = MOD_ID_SIZE;
            break;
        /*--------------------------+
        |   id prom data            |
        +--------------------------*/
        case M_LL_BLK_ID_DATA:
		{
			u_int8 n;
			u_int16 *dataP = (u_int16*)blk->data;

			if (blk->size < MOD_ID_SIZE)		/* check buf size */
				return(ERR_LL_USERBUF);

			for (n=0; n<MOD_ID_SIZE/2; n++)		/* read MOD_ID_SIZE/2 words */
				*dataP++ = (u_int16)m_read((U_INT32_OR_64)llHdl->ma, n);

			break;
		}
        /*--------------------------+
        |   ident table pointer     |
        |   (treat as non-block!)   |
        +--------------------------*/
        case M_MK_BLK_REV_ID:
           *value64P = (INT32_OR_64)&llHdl->idFuncTbl;
           break;
        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
        default:
            error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/******************************* M27_BlockRead *******************************
 *
 *  Description:  Read data block from device
 *
 *                Read channels 0..('size'-1) into 'buf'.
 *                Channels will be read in rising order (0..size-1).
 *
 *                Buffer modes:
 *                   The function always reads from the hardware.
 *
 *                Buffer structure:
 *                   The data buffer uses one byte per channel:
 *
 *                   +---------+
 *                   |  byte 0 |  channel 0
 *                   +---------+
 *                   |  byte 1 |  channel 1 
 *                   +---------+
 *                   |   ...   |   
 *                   +---------+
 *                   |  byte n |  channel 'size'-1  
 *                   +---------+
 *
 *                Byte layout:
 *                   Bit 0 shows the state of the channel: 
 *
 *                   Bit    7 6 5 4 3 2 1 0
 *                   Value  0 0 0 0 0 0 0 x
 *                                        |
 *                                        x=1 : channel set
 *                                        x=0 : channel reseted
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        ll handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size  (0..16)
 *  Output.....:  nbrRdBytesP  number of read bytes (0..16)
 *                return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_BlockRead(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrRdBytesP
)
{
	u_int16 value, i;

    DBGWRT_1((DBH, "LL - M27_BlockRead: ch=%d, size=%d\n",ch,size));

	/* set nr of read bytes */
	*nbrRdBytesP = 0;

	/* check for size 0..16 */
	if( (size < 0) || (size > CH_NUMBER) )
		return ERR_LL_ILL_PARAM;

	/* read all channels */
	value = MREAD_D16( llHdl->ma, OUTPUT_REG );

	/* expand 'size' bits -> 'size' bytes */
	for (i=0; i<size; i++) {
		*((int8*)buf+i) = value & 0x1;
		value = value >> 1;
	}

	/* update nr of read bytes */
	*nbrRdBytesP = size;

	return(ERR_SUCCESS);
}

/****************************** M27_BlockWrite *******************************
 *
 *  Description:  Write data block to device
 *
 *                Write channels 0..('size'-1) into 'buf'.
 *                Channels will be written in rising order (0..size-1).
 *
 *                Buffer modes:
 *                   The function always writes from the hardware.
 *
 *                Buffer structure:
 *                   The data buffer uses one byte per channel:
 *
 *                   +---------+
 *                   |  byte 0 |  channel 0
 *                   +---------+
 *                   |  byte 1 |  channel 1 
 *                   +---------+
 *                   |   ...   |   
 *                   +---------+
 *                   |  byte n |  channel 'size'-1  
 *                   +---------+
 *
 *                Byte layout:
 *                   If any bit is set, than the channel will be set.
 *                   If all bits are 0, than the channel will be reseted.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        ll handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size (0..16)
 *  Output.....:  nbrWrBytesP  number of written bytes (0..16)
 *                return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_BlockWrite(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
	u_int16 value=0, oldValue, i;

    DBGWRT_1((DBH, "LL - M27_BlockWrite: ch=%d, size=%d\n",ch,size));

	/* set nr of written bytes */
	*nbrWrBytesP = 0;

	/* check for size 0..16 */
	if( (size < 0) || (size > CH_NUMBER) )
		return ERR_LL_ILL_PARAM;

	/* read all channels */
	oldValue = MREAD_D16( llHdl->ma, OUTPUT_REG );

	/* compress 'size' bytes -> 'size' bits */
	for (i=0; i<size; i++)
		value |= ((*((int8*)buf+i) ? 1 : 0) << i);

	value |= (oldValue & (0xffff << size));
	
	/* write all channels */
	MWRITE_D16( llHdl->ma, OUTPUT_REG, value );

	/* update nr of written bytes */
	*nbrWrBytesP = size;

	return(ERR_SUCCESS);
}


/****************************** M27_Irq *************************************
 *
 *  Description:  Interrupt service routine
 *
 *                Unused - the module supports no interrupt.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    ll handle
 *  Output.....:  return   LL_IRQ_DEVICE	irq caused from device
 *                         LL_IRQ_DEV_NOT   irq not caused from device
 *                         LL_IRQ_UNKNOWN   unknown
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_Irq(
   LL_HANDLE *llHdl
)
{
    IDBGWRT_1((DBH, ">>> M27_Irq:\n"));

	/* not my interrupt */
	return(LL_IRQ_DEV_NOT);
}

/****************************** M27_Info ************************************
 *
 *  Description:  Get information about hardware and driver requirements.
 *
 *                Following info codes are supported:
 *
 *                Code                      Description
 *                ------------------------  -----------------------------
 *                LL_INFO_HW_CHARACTER      hardware characteristics
 *                LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *                LL_INFO_ADDRSPACE         address space information
 *                LL_INFO_IRQ               interrupt required
 *                LL_INFO_LOCKMODE          process lock mode required
 *
 *                The LL_INFO_HW_CHARACTER code returns all address and 
 *                data modes (OR'ed), which are supported from the
 *                hardware (MDIS_MAxx, MDIS_MDxx).
 *
 *                The LL_INFO_ADDRSPACE_COUNT code returns the number
 *                of address spaces used from the driver.
 *
 *                The LL_INFO_ADDRSPACE code returns information about one
 *                specific address space (MDIS_MAxx, MDIS_MDxx). The returned 
 *                data mode represents the widest hardware access used from 
 *                the driver.
 *
 *                The LL_INFO_IRQ code returns, if the driver supports an
 *                interrupt routine (TRUE or FALSE).
 *
 *                The LL_INFO_LOCKMODE code returns, which process locking
 *                mode is required from the driver (LL_LOCK_xxx).
 *
 *---------------------------------------------------------------------------
 *  Input......:  infoType	   info code
 *                ...          argument(s)
 *  Output.....:  return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
int32 M27_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
		/*-------------------------------+
        |  hardware characteristics      |
        |  (all addr/data modes OR'ed)   |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
		{
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);

			*addrModeP = MDIS_MA08;
			*dataModeP = MDIS_MD08 | MDIS_MD16;
			break;
	    }
		/*-------------------------------+
        |  nr of required address spaces |
        |  (total spaces used)           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
		{
			u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

			*nbrOfAddrSpaceP = ADDRSPACE_COUNT;
			break;
	    }
		/*-------------------------------+
        |  address space type            |
        |  (widest used data mode)       |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
		{
			u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
			u_int32 *addrSizeP = va_arg(argptr, u_int32*);

			if (addrSpaceIndex >= ADDRSPACE_COUNT)
				error = ERR_LL_ILL_PARAM;
			else {
				*addrModeP = MDIS_MA08;
				*dataModeP = MDIS_MD16;
				*addrSizeP = ADDRSPACE_SIZE;
			}

			break;
	    }
		/*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
		{
			u_int32 *useIrqP = va_arg(argptr, u_int32*);

			*useIrqP = USE_IRQ;
			break;
	    }
		/*-------------------------------+
        |   process lock mode            |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
		{
			u_int32 *lockModeP = va_arg(argptr, u_int32*);

			*lockModeP = LL_LOCK_CALL;
			break;
	    }
		/*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
          error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ************************************
 *
 *  Description:  Return ident string
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return  ptr to ident string
 *  Globals....:  -
 ****************************************************************************/
static char* Ident( void )
{
    return( "M27 - M27 low level driver: $Id: m27_drv.c,v 1.3 2010/03/10 14:14:55 amorbach Exp $" );
}

/********************************* Cleanup **********************************
 *
 *  Description: Close all handles, free memory and return error code
 *		         NOTE: The ll handle is invalid after calling this function
 *			   
 *---------------------------------------------------------------------------
 *  Input......: llHdl		ll handle
 *               retCode    return value
 *  Output.....: return	    retCode
 *  Globals....: -
 ****************************************************************************/
static int32 Cleanup(
   LL_HANDLE    *llHdl,
   int32        retCode
)
{
    /*------------------------------+
    |  close handles                |
    +------------------------------*/
	/* clean up desc */
	if (llHdl->descHdl)
		DESC_Exit(&llHdl->descHdl);

	/* cleanup debug */
	DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free my handle */
    OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
	return(retCode);
}



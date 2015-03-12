#***************************  M a k e f i l e  *******************************
#
#         Author: ds
#          $Date: 2004/05/03 14:37:09 $
#      $Revision: 1.2 $
#
#    Description: Makefile definitions for the M27 example program
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2004/05/03 14:37:09  cs
#   removed switch MAK_OPTIM
#
#   Revision 1.1  1998/12/07 16:00:08  Schmidt
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m27_simp

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)    \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)     \

MAK_INCL=$(MEN_INC_DIR)/m27_drv.h     \
         $(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/mdis_err.h    \
         $(MEN_INC_DIR)/usr_oss.h     \

MAK_INP1=m27_simp$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

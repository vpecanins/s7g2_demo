//--------------------------------------------------------------------------
// Project: D/AVE
// File:    dave_base_nios.h (%version: 4 %)
//          created Mon Aug 22 12:50:45 2005 by hh04027
//
// Description:
//  %date_modified: Wed Apr 18 15:42:41 2007 %  (%derived_by:  hh74040 %)
//
// Changes:
//  2005-04-12 CSe  fully implemented missing d1 driver functionality
//
//--------------------------------------------------------------------------

#ifndef _dave_base_s7_h
#define _dave_base_s7_h

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

#define DAVE2D_0_BASE (R_G2D_BASE)
/* TODO: Determine if using DLIST_INDIRECT has potential heap fragmentation issues */
#define S7_USE_DLIST_INDIRECT  0

//--------------------------------------------------------------------------
//
#define WRITE_REG( BASE, OFFSET, DATA ) \
        (((unsigned long *)(BASE))[OFFSET] = DATA )

#define READ_REG( BASE, OFFSET ) \
        (((unsigned long *)(BASE))[OFFSET] )

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

typedef struct _d1_device_s7
{
  volatile long *dlist_start;         /* dlist start addresses */
  int dlist_indirect;
} d1_device_s7;

//---------------------------------------------------------------------------

extern int d1_initirq_intern( d1_device_s7 *handle );
extern int d1_shutdownirq_intern( d1_device_s7 *handle );

//---------------------------------------------------------------------------

#define D2_DLISTSTART   50

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif

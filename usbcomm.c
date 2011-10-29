#include "ibuddy.h"

static unsigned int usb_timeout = 0;

static int usb_send( struct usb_device* udev, void* data, int len );
/* ---------------------------------------------------------------- */
void ibuddy_usb_setup( unsigned int timeout )
{
  usb_timeout = timeout;
}

/* ---------------------------------------------------------------- */
static unsigned char* reset_data  = "\x22\x09\x00\x02\x01\x00\x00\x00";
static void ibuddy_setup( struct ibuddy_dev* dev ) 
{
  int retval = usb_send( dev->udev, reset_data, 8 );
  if( retval )
    ERROR("usb_send in reset failed (%d)\n", retval);
}

/* ---------------------------------------------------------------- */
static unsigned char* cmd_data = "\x55\x53\x42\x43\x00\x40\x02\xFF";
void ibuddy_cmd( struct ibuddy_dev* dev, uint8_t cmd )
{
  unsigned char* data = kmemdup( cmd_data, 8, GFP_KERNEL );
  if( !data ) {
    ERROR( "ibuddy_cmd out of memory\n" );
    return;
  }
  data[7] = cmd;

  ibuddy_setup( dev );
  if (usb_send( dev->udev, data, 8 ))
    ERROR("usb_send in ibuddy_cmd failed\n");

  kfree( data );
}

/* ---------------------------------------------------------------- */
static int usb_send( struct usb_device* udev, void* data, int len )
{
  int retval;

  /* move the data to DMA capable area */
  void* msg = kmemdup( data, len, GFP_KERNEL | __GFP_DMA );
  if( !msg ) {
    ERROR( "out of memory\n" );
    return -ENOMEM;
  }

  /* synchronous control message (wait for completion) */
  retval  = usb_control_msg( udev,
			     usb_sndctrlpipe( udev, 0 ) /* 0 endpoint */,

		      /* http://www.beyondlogic.org/usbnutshell/usb6.shtml */
			     0x09, /* request == SET_CONFIGURATION */
			     0x21, /* req type */
			     /* 00100001 
				D0..4 = req type  = interface
				D6..5 = type      = class
				D7    = direction = host to device
			     */

			     0x02,  /* value */
			     0x01,  /* index */
			     msg,  /* data */
			     len,   /* size */
			     usb_timeout   /* timeout in ms, 0 = forever */ );
  kfree( msg );
  return (retval<0) ? retval : 0;
} /* usb_send */

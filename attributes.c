#include "ibuddy.h"

/* ---------------------------------------------------------------- */
/* use xor to "toggle" bits on and off, 
   the protocol bits are from: http://imakethin.gs/blog/?p=17
*/
static void toggle( struct ibuddy_dev* dev, int startval, int shift )
{
  int mask = startval << shift;
  dev->raw ^= mask;  // use xor to flip bit(s)
  ibuddy_cmd( dev, dev->raw );  // send new data to the i-buddy
}

/* ---------------------------------------------------------------- */
/* _every_ attribute will return the raw bits in hex when read
*/
static ssize_t show_attr
(struct device *dev, struct device_attribute *attr,  char *buf)   
{ 
  struct usb_interface *intf = to_usb_interface(dev);	     
  struct ibuddy_dev *buddy = usb_get_intfdata(intf);
  return sprintf(buf, "%X\n", buddy->raw );
}

/* reset-attribute will set a baseline value (all off) */
static ssize_t set_reset
(struct device *dev, struct device_attribute *attr, 
 const char *buf, size_t count)	
{
  struct usb_interface *intf = to_usb_interface(dev);	    
  struct ibuddy_dev *buddy = usb_get_intfdata(intf);	   

  /* set initial value */
  buddy->raw = IBUDDY_INITIAL_VALUE;
  ibuddy_cmd( buddy, buddy->raw );

  return count;                                        
} 
static DEVICE_ATTR(reset, S_IWUSR | S_IRUGO, show_attr, set_reset );

/* helper macro to make individual set-routines for attributes */
#define ibuddy_toggle_attr( name, startval, bitpos )	\
static ssize_t set_##name \
(struct device *dev, struct device_attribute *attr, \
 const char *buf, size_t count)		            \
{ /* any actual data written from userspace is ignored */	     \
        struct usb_interface *intf = to_usb_interface(dev);  \
        struct ibuddy_dev *buddy = usb_get_intfdata(intf);   \
        toggle( buddy, startval, bitpos );		     \
        return count;                                        \
} \
static DEVICE_ATTR(name, S_IWUSR | S_IRUGO, show_attr, set_##name );

/* control attributes for the i-buddy. 
   1 is one bit value,  3 is for 2 bits
   last arg is bit position in the control byte */
ibuddy_toggle_attr( heart, 1, 7 );
ibuddy_toggle_attr( blue,  1, 6 );
ibuddy_toggle_attr( green, 1, 5 );
ibuddy_toggle_attr( red,   1, 4 );
ibuddy_toggle_attr( wings, 3, 2 );
ibuddy_toggle_attr( twist, 3, 0 );

/* ---------------------------------------------------------------- */
/* macro to register an attribute and howl an error if needed */
#define ibuddy_register_attr( name ) \
if( device_create_file(&interface->dev, &dev_attr_##name) != 0 ) { \
    ERROR("attr create foo\n"); \
}

/* create attributes at device init (usb probe calls this) */
void ibuddy_attr_init( struct usb_interface* interface )
{
  ibuddy_register_attr( heart );
  ibuddy_register_attr( blue );
  ibuddy_register_attr( green );
  ibuddy_register_attr( red );
  ibuddy_register_attr( wings );
  ibuddy_register_attr( twist );
  ibuddy_register_attr( reset );
}

/* ---------------------------------------------------------------- */
/* in symmetry remove attributes in device disconnect */
#define ibuddy_deregister_attr( name ) \
  device_remove_file(&interface->dev, &dev_attr_heart);

void ibuddy_attr_remove( struct usb_interface* interface )
{
  ibuddy_deregister_attr( heart );
  ibuddy_deregister_attr( blue );
  ibuddy_deregister_attr( green );
  ibuddy_deregister_attr( red );
  ibuddy_deregister_attr( wings );
  ibuddy_deregister_attr( twist );
  ibuddy_deregister_attr( reset );
}


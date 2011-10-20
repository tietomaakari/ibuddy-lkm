#ifndef IBUDDY_H
#define IBUDDY_H

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>

#define DEBUG_IBUDDY 1

#define DRIVER_AUTHOR "Jyke Tapani Jokinen, tietomaakari@gmail.com"
#define DRIVER_NAME "ibuddy"
#define DRIVER_DESC "iBuddy USB driver"
#define IBUDDY_INITIAL_VALUE 0xF5

/* --- kernel printing routines --- */
#define PRINT(fmt,arg...) printk(KERN_INFO DRIVER_NAME ": " fmt,##arg)
#ifdef DEBUG_IBUDDY
#define ERROR(fmt,arg...) do { \
  printk(KERN_ALERT DRIVER_NAME ":%s: Error! - " fmt, __func__, ##arg); \
  } while(0) 
#else
#define ERROR(fmt,arg...) do {} while(0)
#endif

/* --- per i-buddy information ---- */
struct ibuddy_dev {
  struct usb_device* udev;      // usb device
  unsigned char      raw;       // data sent to ibuddy
  unsigned short     type;      // type from probe
  unsigned char*     procname;  // /proc/driver/ibuddy/X name
  unsigned char      num;       // enumeration of this device (0-9)
  struct kobject*    kobj;
};

void ibuddy_cmd( struct ibuddy_dev* dev, uint8_t cmd );

void ibuddy_attr_init( struct usb_interface* interface );
void ibuddy_attr_remove( struct usb_interface* interface );

#endif /* IBUDDY_H */

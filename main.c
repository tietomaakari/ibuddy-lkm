#include "ibuddy.h"

#ifndef CONFIG_SYSFS
#error "sysfs needed in kernel"
#endif

/* table of devices that work with this driver */
static const struct usb_device_id id_table[] = {
  { USB_DEVICE(0x1130, 0x0001),
    .driver_info = 1
  },
  { },
};
MODULE_DEVICE_TABLE (usb, id_table);

/* table of devices [ 0, 1, ... ]; size 10 */
static unsigned char* ibuddy_enumeration;

/* ---------------------------------------------------------------- */
static int ibuddy_probe(
  struct usb_interface*        interface, 
  const struct usb_device_id*  id )
{
  struct usb_device* udev = interface_to_usbdev(interface);
  struct ibuddy_dev* mydev = NULL;
  int pos;
  int retval = -ENOMEM;

  PRINT("probe\n");
  /* we only attach to 1st interface */
  if( interface->cur_altsetting->desc.bInterfaceNumber != 1 ) {
    ERROR("ignored interface\n");
    return -ENXIO; 
  }

  /* find out our enumeration 0-9 */
  for( pos = 0; pos < 10; pos++ ) {
    if( ibuddy_enumeration[pos] == 0 ) {
      ibuddy_enumeration[pos] = 1;
      break;
    }
  }
  if( pos == 10 ) {
    ERROR( "more than 10 devices not supported\n" );
    return -ENOSPC;
  }

  mydev = kzalloc(sizeof(struct ibuddy_dev), GFP_KERNEL);
  if (mydev == NULL) {
    ERROR( "out of memory\n" );
    return retval;
  }
  mydev->num = pos;

  /* usb_get_dev - increments the reference count of the usb device structure*/
  mydev->udev = usb_get_dev( udev );
  /* extra info from usb core probe */
  mydev->type = id->driver_info;
  /* save the kobj location */
  mydev->kobj = &interface->dev.kobj;

  /* save our data to interface */
  usb_set_intfdata(interface, mydev);

  mydev->procname = NULL;

#ifdef CONFIG_PROC_FS
  { 
    /* create symlink from /proc/driver to sysfs entry
       e.g. /proc/driver/ibuddy/0 -> /sys/... */
    struct kobject* k = mydev->kobj;
    char** path_parts = NULL;
    char* path = NULL;
    int idx = 0;
    char* from;

                     /* FIXME: magic constant */
    path_parts = kzalloc( 40*sizeof(char*), GFP_KERNEL );
    if( path_parts == NULL ) {
      ERROR( "out of memory\n" );
      return retval;
    }
    while( k->parent != NULL ) {
      path_parts[idx++] = (char*)k->name;  // const char* -> char*
      // PRINT( "%s\n", k->name );  // DEBUG
      k = k->parent;
      if( idx >= 39 ) {
	ERROR( "kobj name nesting too deep\n" );
	break;
      }
    }

    path = kzalloc( 40*16, GFP_KERNEL ); /* FIXME: magic constants */
    if( path == NULL ) {
      ERROR( "out of memory\n" );
      return retval;
    }
    strcat( path, "/sys/devices/" );
    while( idx > 0 ) {
      strcat( path, path_parts[idx-1] );
      strcat( path, "/" );
      idx--;
    }
    kfree( path_parts );  /* when path is ready the parts no longer needed */


    from = kzalloc( 20*sizeof(char), GFP_KERNEL );
    if( from == NULL ) {
      ERROR( "out of memory\n" );
      return retval;
    }
    strcat( from, "driver/ibuddy/X" );
    from[14] = '0' + mydev->num;
    mydev->procname = from;

    if( proc_symlink( from, NULL, path ) ) {
      PRINT( "created /proc/%s -> %s\n", from, path );
    }
    kfree( path );
  }
#endif

  /* create attributes for the device in the sysfs */
  ibuddy_attr_init( interface );

  /* set initial values */
  mydev->raw = IBUDDY_INITIAL_VALUE;
  ibuddy_cmd( mydev, mydev->raw );

  dev_info( &interface->dev, DRIVER_NAME " USB driver attached\n" );

  /* increment module use count */
  if( !try_module_get(THIS_MODULE) )
    ERROR( "try_module_get failed!\n" );

  return 0;
} /* ibuddy_probe() */

/* ---------------------------------------------------------------- */
static void ibuddy_disconnect(struct usb_interface* interface )
{
  struct ibuddy_dev* mydev = usb_get_intfdata( interface );
  PRINT("disconnect called\n");

  if( mydev == NULL ) return;  /* not registered for this */


  PRINT("disconnecting ibuddy %d\n", mydev->num );

  ibuddy_attr_remove( interface );

#ifdef CONFIG_PROC_FS
  if( mydev->procname != NULL ) {
    PRINT("removing entry /proc/%s\n", mydev->procname );
    remove_proc_entry( mydev->procname, NULL );
    kfree( mydev->procname );
  }
#endif

  usb_set_intfdata( interface, NULL );
  usb_put_dev( mydev->udev );
  ibuddy_enumeration[ mydev->num ] = 0;

  kfree( mydev );
  module_put(THIS_MODULE);

  PRINT("disconnect done.\n");
} /* ibuddy_disconnect() */


/* ---------------------------------------------------------------- */
static struct usb_driver ibuddy_driver = {
  .name =         DRIVER_NAME,
  .probe =        ibuddy_probe,
  .disconnect =   ibuddy_disconnect,
  .id_table =     id_table,
};

/* -------------------------------------------------------------------- */
/* module parameters */
static unsigned int timeout = 0;
module_param( timeout, uint, S_IRUGO );
MODULE_PARM_DESC( timeout, 
		  "timeout for usb_control_msg(). default 0 (no timeout)" );

/* -------------------------------------------------------------------- */
static int __init ibuddy_init(void)
{
  int retval = -ENOMEM;

  PRINT("init\n");

  ibuddy_usb_setup( timeout ); /* module param, default = 0 */

  ibuddy_enumeration = kzalloc( 8*sizeof(unsigned char), GFP_KERNEL );
  if( ibuddy_enumeration == NULL ) {
    ERROR("init out of memory\n");
    return retval;
  }

#ifdef CONFIG_PROC_FS
  /* don't care if it already exists: */
  (void)proc_mkdir( "driver/ibuddy", NULL );
#endif

  retval = usb_register(&ibuddy_driver);
  if (retval)
    err("usb_register failed. Error number %d", retval);
  return retval;
}

/* -------------------------------------------------------------------- */
static void __exit ibuddy_exit(void)
{
  usb_deregister(&ibuddy_driver);
  kfree( ibuddy_enumeration );

#ifdef CONFIG_PROC_FS
  /* don't care if it already exists: */
  (void)remove_proc_entry( "driver/ibuddy", NULL );
#endif
  PRINT("exit\n");
}

module_init (ibuddy_init);
module_exit (ibuddy_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");



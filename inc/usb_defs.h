#ifndef _USB_DEFS_H
#define _USB_DEFS_H

/* USB types */
#define USB_TYPE_STANDARD   (0x00 << 5)
#define USB_TYPE_CLASS      (0x01 << 5)
#define USB_TYPE_VENDOR     (0x02 << 5)
#define USB_TYPE_RESERVED   (0x03 << 5)

/* USB recipients */
#define USB_RECIP_DEVICE      0x00
#define USB_RECIP_INTERFACE   0x01
#define USB_RECIP_ENDPOINT    0x02
#define USB_RECIP_OTHER       0x03

/* USB directions */
#define USB_DIR_OUT           0
#define USB_DIR_IN            0x80

/* USB device speeds */
#define USB_SPEED_FULL      0x0 /* 12Mbps */
#define USB_SPEED_LOW       0x1 /* 1.5Mbps */
#define USB_SPEED_HIGH      0x2 /* 480Mbps */
#define USB_SPEED_RESERVED  0x3

/* Descriptor types */
#define USB_DT_DEVICE        0x01
#define USB_DT_CONFIG        0x02
#define USB_DT_STRING        0x03
#define USB_DT_INTERFACE     0x04
#define USB_DT_ENDPOINT      0x05

/* Endpoints */
#define USB_ENDPOINT_NUMBER_MASK  0x0f  /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK     0x80

#define USB_ENDPOINT_XFERTYPE_MASK 0x03 /* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL  0
#define USB_ENDPOINT_XFER_ISOC     1
#define USB_ENDPOINT_XFER_BULK     2
#define USB_ENDPOINT_XFER_INT      3


#endif	/* _USB_DEFS_H */

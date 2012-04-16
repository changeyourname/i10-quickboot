#ifndef _USB_CH9_H
#define _USB_CH9_H

/*
 * Device State (c.f USB Spec 2.0 Figure 9-1)
 *
 * What state the usb device is in.
 *
 * Note the state does not change if the device is suspended, we simply set a
 * flag to show that it is suspended.
 *
 */
typedef enum usb_device_state {
    STATE_INIT,     /* just initialized */
    STATE_CREATED,      /* just created */
    STATE_ATTACHED,     /* we are attached */
    STATE_POWERED,      /* we have seen power indication (electrical bus signal) */
    STATE_DEFAULT,      /* we been reset */
    STATE_ADDRESSED,    /* we have been addressed (in default configuration) */
    STATE_CONFIGURED,   /* we have seen a set configuration device command */
    STATE_UNKNOWN,      /* destroyed */
} usb_device_state_t;

/*
 * Device Requests  (c.f Table 9-2)
 */

struct usb_device_request {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__ ((packed));

/* 
 * bmRequestType 
 */
#define USB_REQ_DIRECTION_MASK      0x80
#define USB_REQ_TYPE_MASK       	0x60
#define USB_REQ_RECIPIENT_MASK      0x1f
    
#define USB_REQ_DEVICE2HOST     	0x80
#define USB_REQ_HOST2DEVICE     	0x00
    
#define USB_REQ_TYPE_STANDARD       0x00
#define USB_REQ_TYPE_CLASS      	0x20
#define USB_REQ_TYPE_VENDOR     	0x40
    
#define USB_REQ_RECIPIENT_DEVICE    0x00
#define USB_REQ_RECIPIENT_INTERFACE 0x01
#define USB_REQ_RECIPIENT_ENDPOINT  0x02
#define USB_REQ_RECIPIENT_OTHER     0x03

/*
 * Standard requests (bRequest, see table 9-4)
 */
#define USB_REQ_GET_STATUS			0x00
#define USB_REQ_CLEAR_FEATURE       0x01
#define USB_REQ_SET_FEATURE			0x03
#define USB_REQ_SET_ADDRESS			0x05
#define USB_REQ_GET_DESCRIPTOR      0x06
#define USB_REQ_SET_DESCRIPTOR      0x07
#define USB_REQ_GET_CONFIGURATION   0x08
#define USB_REQ_SET_CONFIGURATION   0x09
#define USB_REQ_GET_INTERFACE       0x0A
#define USB_REQ_SET_INTERFACE       0x0B
#define USB_REQ_SYNCH_FRAME			0x0C

/*
 * descriptor types (see table 9-5)
 */
#define USB_DESCRIPTOR_TYPE_DEVICE				0x01
#define USB_DESCRIPTOR_TYPE_CONFIGURATION       0x02
#define USB_DESCRIPTOR_TYPE_STRING				0x03
#define USB_DESCRIPTOR_TYPE_INTERFACE           0x04
#define USB_DESCRIPTOR_TYPE_ENDPOINT            0x05
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER	0x06
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION	0x07
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER     0x08

/*
 * standard usb descriptor structures
 */ 
struct usb_device_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType; 		/* 0x01 */
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__ ((packed));


struct usb_configuration_descriptor {
	uint8_t bLength;       
	uint8_t bDescriptorType; /* 0x2 */
	uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__ ((packed));



struct usb_interface_descriptor {
    u8 bLength;
    u8 bDescriptorType; /* 0x04 */
    u8 bInterfaceNumber;
    u8 bAlternateSetting;
    u8 bNumEndpoints; 
    u8 bInterfaceClass;
    u8 bInterfaceSubClass;
    u8 bInterfaceProtocol;
    u8 iInterface;
} __attribute__ ((packed));

struct usb_endpoint_descriptor {
    u8 bLength;
    u8 bDescriptorType; /* 0x5 */
    u8 bEndpointAddress;
    u8 bmAttributes;
    u16 wMaxPacketSize;
    u8 bInterval;
} __attribute__ ((packed));

/* Endpoints */
#define USB_ENDPOINT_NUMBER_MASK  0x0f  /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK     0x80
                    
#define USB_ENDPOINT_XFERTYPE_MASK 0x03 /* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL  0
#define USB_ENDPOINT_XFER_ISOC     1
#define USB_ENDPOINT_XFER_BULK     2
#define USB_ENDPOINT_XFER_INT      3

struct usb_string_descriptor {
	uint8_t bLength;
    uint8_t bDescriptorType; /* 0x03 */
//    uint16_t wData[0];
} __attribute__ ((packed));


/*
 * Device and/or Interface Class codes
 * as found in bDeviceClass or bInterfaceClass
 * and defined by www.usb.org documents
 */
#define USB_CLASS_VENDOR_SPEC       0xff

#define USB_SUBCLASS_VENDOR_SPEC    0xff

/*
 * standard feature selectors (Table 9-6)
 */             
#define USB_ENDPOINT_HALT			0x00		/* recipient: endpoint */
#define USB_DEVICE_REMOTE_WAKEUP    0x01		/* recipient: device */
#define USB_TEST_MODE				0x02		/* recipient: device */

/*      
 * get status bits
 */ 
        
#define USB_STATUS_SELFPOWERED      0x01
#define USB_STATUS_REMOTEWAKEUP     0x02
    
#define USB_STATUS_HALT         0x01



/* configuration modifiers
 */
#define BMATTRIBUTE_RESERVED        0x80
#define BMATTRIBUTE_SELF_POWERED    0x40


#endif	/* _USB_CH9_H */

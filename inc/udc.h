#ifndef _UDC_H
#define _UDC_H
#include <usb_defs.h>
#include <usb_ch9.h>

#define usb_err		PRINT
#define usb_dbg		PRINT
#define usb_info	//PRINT


typedef enum usb_device_event {

    DEVICE_UNKNOWN,     /* bi - unknown event */
    DEVICE_INIT,        /* bi  - initialize */
    DEVICE_CREATE,      /* bi  - */
    DEVICE_HUB_CONFIGURED,  /* bi  - bus has been plugged int */
    DEVICE_RESET,       /* bi  - hub has powered our port */

    DEVICE_ADDRESS_ASSIGNED,    /* ep0 - set address setup received */
    DEVICE_CONFIGURED,  /* ep0 - set configure setup received */
    DEVICE_SET_INTERFACE,   /* ep0 - set interface setup received */

    DEVICE_SET_FEATURE, /* ep0 - set feature setup received */
    DEVICE_CLEAR_FEATURE,   /* ep0 - clear feature setup received */
    
    DEVICE_DE_CONFIGURED,   /* ep0 - set configure setup received for ?? */
    
    DEVICE_BUS_INACTIVE,    /* bi  - bus in inactive (no SOF packets) */
    DEVICE_BUS_ACTIVITY,    /* bi  - bus is active again */

    DEVICE_POWER_INTERRUPTION,  /* bi  - hub has depowered our port */
    DEVICE_HUB_RESET,   /* bi  - bus has been unplugged */
    DEVICE_DESTROY,     /* bi  - device instance should be destroyed */

    DEVICE_HOTPLUG,     /* bi  - a hotplug event has occured */

    DEVICE_FUNCTION_PRIVATE,    /* function - private */

} usb_device_event_t;


typedef enum urb_send_status {
    SEND_IN_PROGRESS,
    SEND_FINISHED_OK,
    SEND_FINISHED_ERROR,
    RECV_READY,
    RECV_OK,
    RECV_ERROR
} urb_send_status_t;

typedef enum {
	URB_NOUSE = 0,
	URB_SUBMIT,
//	URB_IN_PROGRESS,
	URB_COMPLETE,
}urb_stage_t;


struct urb {
    
    struct usb_endpoint_instance *endpoint;
    struct usb_device_instance *device;
    
    struct usb_device_request device_request;   /* contents of received SETUP packet */
    
    
    uint8_t* buffer; 
    unsigned int buffer_length;
    unsigned int actual_length;
    
    urb_stage_t stage;
    int		result;
    unsigned 	zero:1;
    
    void (*complete)(void *, int);
    void *complete_arg;

};

struct usb_endpoint_instance {
	int endpoint_address;
	
    /* control */
//    int status;     /* halted */
    int state;      /* available for use by bus interface driver */
        
    /* receive side */
    struct urb *rcv_urb;    /* active urb */
    uint8_t rcv_attributes; /* copy of bmAttributes from endpoint descriptor */
    uint16_t rcv_packetSize; /* maximum packet size from endpoint descriptor */
//    int rcv_transferSize;   /* maximum transfer size from function driver */
//    int rcv_queue;

    /* transmit side */
    struct urb *tx_urb; /* active urb */
    uint8_t tx_attributes;  /* copy of bmAttributes from endpoint descriptor */
    uint16_t tx_packetSize;  /* maximum packet size from endpoint descriptor */
//    int tx_transferSize;    /* maximum transfer size from function driver */
//    int tx_queue;	
    
    int sent;       /* data already sent */
    int last;       /* data sent in last packet XXX do we need this */
};

struct usb_alternate_instance {
    struct usb_interface_descriptor *interface_descriptor;

    int endpoints;
//    int *endpoint_transfersize_array;
    struct usb_endpoint_descriptor **endpoints_descriptor_array;
};

struct usb_interface_instance {
    int alternates;
    struct usb_alternate_instance *alternates_instance_array;
};

struct usb_configuration_instance {
	int interfaces;
	struct usb_configuration_descriptor *configuration_descriptor;
	struct usb_interface_instance *interface_instance_array;
};

struct usb_device_instance {
	struct usb_device_descriptor *device_descriptor;
	
	void (*event) (struct usb_device_instance *device, usb_device_event_t event, int data);
	
	/* configuration descriptors */
	int configurations;
	struct usb_configuration_instance *configuration_instance_array;
	
	/* device state */
	usb_device_state_t	device_state;

	
	uint8_t		address;
	uint8_t		configuration;
	uint8_t		interface;
	uint8_t		alternate;
	
	int	speed;
//    usb_device_status_t status; /* device status */
};




int udc_init(void);
void udc_enable(struct usb_device_instance *device);
void udc_disable(void);
void udc_connect(void);
void udc_disconnect(void);
void udc_startup_events(struct usb_device_instance *device);
void udc_setup_ep(struct usb_device_instance *device, unsigned int id, struct usb_endpoint_instance *endpoint);
void udc_irq(void);
int udc_endpoint_write(struct usb_endpoint_instance *endpoint);

struct usb_string_descriptor *usbd_get_string (uint8_t index);
struct usb_interface_instance *usbd_device_interface_instance (
		struct usb_device_instance *device, 
		int configuration, 
		int interface);
struct usb_alternate_instance *usbd_device_alternate_instance (
		struct usb_device_instance *device, 
		int configuration, 
		int interface,
		int alternate);
struct usb_device_descriptor *usbd_device_device_descriptor (struct usb_device_instance *device);
struct usb_configuration_descriptor *usbd_device_configuration_descriptor (
		struct usb_device_instance *device,
		int configuration);
struct usb_interface_descriptor *usbd_device_interface_descriptor (
		struct usb_device_instance *device, 
		int configuration, 
		int interface, 
		int alternate);
struct usb_endpoint_descriptor *usbd_device_endpoint_descriptor_index (
		struct usb_device_instance *device, 
		int configuration, 
		int interface, 
		int alternate, 
		int index);
struct usb_endpoint_descriptor *usbd_device_endpoint_descriptor (
		struct usb_device_instance *device, 
		int configuration, 
		int interface, 
		int alternate, 
		int endpoint);
struct urb *usbd_alloc_urb (struct usb_device_instance *device,
                struct usb_endpoint_instance *endpoint);
void usbd_dealloc_urb (struct urb *urb);

void usbd_device_event_irq(struct usb_device_instance *device, usb_device_event_t event, int data);

#endif	/* _UDC_H */

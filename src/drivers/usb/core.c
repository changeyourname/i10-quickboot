#include <types.h>
#include <common.h>
#include <udc.h>

/* Global variables ************************************************************************** */
struct usb_string_descriptor **usb_strings;

struct usb_string_descriptor *usbd_get_string (uint8_t index)
{
    return usb_strings[index];
}

static struct usb_configuration_instance *
usbd_device_configuration_instance (struct usb_device_instance *device,
									unsigned int configuration)
{
    if (configuration >= device->configurations)
        return NULL;

    return device->configuration_instance_array + configuration;
}

struct usb_interface_instance *
usbd_device_interface_instance (struct usb_device_instance *device, 
								int configuration, 
								int interface)
{
    struct usb_configuration_instance *configuration_instance;
    
    configuration_instance = usbd_device_configuration_instance (device, configuration);
    if (configuration_instance == NULL) {
        return NULL;
    }
    if (interface >= configuration_instance->interfaces) {
        return NULL;
    }
    
    return configuration_instance->interface_instance_array + interface;
}

struct usb_alternate_instance *
usbd_device_alternate_instance (struct usb_device_instance *device, 
								int configuration, 
								int interface,
								int alternate)
{
    struct usb_interface_instance *interface_instance;

    interface_instance = usbd_device_interface_instance (device, configuration, interface);
    if (interface_instance == NULL) {
        return NULL;
    }

    if (alternate >= interface_instance->alternates) {
        return NULL;
    }

    return interface_instance->alternates_instance_array + alternate;
}

struct usb_device_descriptor *
usbd_device_device_descriptor (struct usb_device_instance *device)
{
    return (device->device_descriptor);
}

struct usb_configuration_descriptor *
usbd_device_configuration_descriptor (struct usb_device_instance *device,
										int configuration)
{
    struct usb_configuration_instance *configuration_instance;
    
    configuration_instance = usbd_device_configuration_instance (device, configuration);
    if (!(configuration_instance)) {
        return NULL;
    }

    return (configuration_instance->configuration_descriptor);
}

struct usb_interface_descriptor *
usbd_device_interface_descriptor (struct usb_device_instance *device, 
									int configuration, 
									int interface, 
									int alternate)
{
    struct usb_interface_instance *interface_instance;
    
    interface_instance = usbd_device_interface_instance (device, configuration, interface);
    if (!(interface_instance)) {
        return NULL;
    }
    
    if ((alternate < 0) || (alternate >= interface_instance->alternates)) {
        return NULL;
    }
    return (interface_instance->alternates_instance_array[alternate].interface_descriptor);
}

struct usb_endpoint_descriptor *
usbd_device_endpoint_descriptor_index (struct usb_device_instance *device, 
										int configuration, 
										int interface, 
										int alternate, 
										int index)
{
    struct usb_alternate_instance *alternate_instance;

    alternate_instance = usbd_device_alternate_instance (device, configuration, interface, alternate);
    if (!(alternate_instance)) {
        return NULL;
    }
    if (index >= alternate_instance->endpoints) {
        return NULL;
    }
    return *(alternate_instance->endpoints_descriptor_array + index);
}

struct usb_endpoint_descriptor *
usbd_device_endpoint_descriptor (struct usb_device_instance *device, 
									int configuration, 
									int interface, 
									int alternate, 
									int endpoint)
{
    struct usb_endpoint_descriptor *endpoint_descriptor;
    int i;

    for (i = 0; 
    	!(endpoint_descriptor = usbd_device_endpoint_descriptor_index (device, configuration, interface, alternate, i)); 
    	i++) {
        if (endpoint_descriptor->bEndpointAddress == endpoint) {
            return endpoint_descriptor;
        }
    }
    
    return NULL;
}

void usbd_rcv_complete(struct usb_endpoint_instance *endpoint, int len, int urb_bad)
{               
    if (endpoint) {
        struct urb *rcv_urb;
                                   
        /* if we had an urb then update actual_length, dispatch if neccessary */
        if ((rcv_urb = endpoint->rcv_urb)) {
                               
            /* check the urb is ok, are we adding data less than the packetsize */
            if (!urb_bad && (len <= endpoint->rcv_packetSize)) {
        
                /* increment the received data size */
                rcv_urb->actual_length += len;
                if (rcv_urb->actual_length >= rcv_urb->buffer_length)
                	rcv_urb->stage = URB_COMPLETE;
    
            } else {
                rcv_urb->actual_length = 0;
//                rcv_urb->status = RECV_ERROR;
            }
        } 
    } 

}

void usbd_tx_complete (struct usb_endpoint_instance *endpoint)
{
	if (endpoint) {
		struct urb *tx_urb = endpoint->tx_urb;

		usb_info("%s \n", __FUNCTION__);
		/* if we have a tx_urb advance or reset, finish if complete */
		if (tx_urb) {
			int sent = endpoint->last;
			endpoint->sent += sent;
			endpoint->last -= sent;

			if( (endpoint->tx_urb->actual_length - endpoint->sent) <= 0 ) {
				usb_info("%s: all send\n", __FUNCTION__);
				tx_urb->stage = URB_COMPLETE;
				tx_urb->actual_length = 0;
				endpoint->sent = 0;
				endpoint->last = 0;

				/* Remove from active, save for re-use */
//				urb_detach(tx_urb);
//				urb_append(&endpoint->done, tx_urb);

//				endpoint->tx_urb = first_urb_detached(&endpoint->tx);
//				if( endpoint->tx_urb ) {
//					endpoint->tx_queue--;
//					usbdbg("got urb from tx list");
//				}
//				if( !endpoint->tx_urb ) {
//					/*usbdbg("taking urb from done list"); */
//					endpoint->tx_urb = first_urb_detached(&endpoint->done);
//				}
//				if( !endpoint->tx_urb ) {
//					usbdbg("allocating new urb for tx_urb");
//					endpoint->tx_urb = usbd_alloc_urb(tx_urb->device, endpoint);
//				}
			}
		}
	}
}

void usbd_device_event_irq(struct usb_device_instance *device,
		usb_device_event_t event, int data)
{
	usb_device_state_t	state;
	
	state = device->device_state;
	
	switch (event) {
	case DEVICE_UNKNOWN:
		break;
	case DEVICE_INIT:
		device->device_state = STATE_INIT;
		break;
	case DEVICE_CREATE:
		device->device_state = STATE_ATTACHED;
		break;
	case DEVICE_HUB_CONFIGURED:
		device->device_state = STATE_POWERED;
		break;
	case DEVICE_RESET:
		device->device_state = STATE_DEFAULT;
		device->address = 0;
		break;
	case DEVICE_ADDRESS_ASSIGNED:
		device->device_state = STATE_ADDRESSED;
		break;
	case DEVICE_CONFIGURED:		
		device->device_state = STATE_CONFIGURED;
		break;
	default:
		break;
	}
	
	if (device->event) {		
		device->event (device, event, data);
	}
}


struct urb *usbd_alloc_urb (struct usb_device_instance *device,
                struct usb_endpoint_instance *endpoint)
{ 
	//TODO
	return (struct urb *)NULL;
}

void usbd_dealloc_urb (struct urb *urb)
{
	//TODO
}

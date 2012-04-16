#include <types.h>
#include <common.h>
#include <byteorder.h>
#include <udc.h>


int usbd_endpoint_halted (struct usb_device_instance *device, int endpoint)
{       
//    return (device->status == USB_STATUS_HALT);
} 

static int ep0_get_status (struct usb_device_instance *device, struct urb *urb, int index, int requesttype)
{
    char *cp;

    urb->actual_length = 2;
    cp = (char*)urb->buffer;
    cp[0] = cp[1] = 0;

    switch (requesttype) {
    case USB_REQ_RECIPIENT_DEVICE:
        cp[0] = USB_STATUS_SELFPOWERED;
        break;
    case USB_REQ_RECIPIENT_INTERFACE:
        break;
    case USB_REQ_RECIPIENT_ENDPOINT:
//        cp[0] = usbd_endpoint_halted (device, index);
        break;
    case USB_REQ_RECIPIENT_OTHER:
        urb->actual_length = 0;
    default:
        break;
    }

    return 0;
}

static int ep0_get_one (struct usb_device_instance *device, struct urb *urb, uint8_t result)
{
    urb->actual_length = 1;
    ((char *) urb->buffer)[0] = result;
    
    return 0;
}

static void copy_config (struct urb *urb, void *data, int max_length, int max_buf)
{
    int available;
    int length;

    length = max_length;
    available = /*urb->buffer_length */ max_buf - urb->actual_length;
    
    if (available <= 0)
        return;

    if (length > available) {
        length = available;
    }
    usb_info("%s : length =%d", __FUNCTION__, length);
    memcpy (urb->buffer + urb->actual_length, data, length);
    urb->actual_length += length;
}

static int ep0_get_descriptor (struct usb_device_instance *device,
                   				struct urb *urb, 
                   				int max, 
                   				int descriptor_type,
                   				int index)
{
	char *cp;
	
	urb->actual_length = 0;
	cp = (char*)urb->buffer;
	
	switch (descriptor_type) {
	case USB_DESCRIPTOR_TYPE_DEVICE:
		{
		struct usb_device_descriptor *device_descriptor;
		if (!(device_descriptor = usbd_device_device_descriptor(device)))
			return -1;
		
		copy_config(urb, device_descriptor, sizeof(struct usb_device_descriptor), max);
		}
		break;
	
	case USB_DESCRIPTOR_TYPE_CONFIGURATION:
		{
        struct usb_configuration_descriptor *configuration_descriptor;
        struct usb_device_descriptor *device_descriptor;
        
        usb_info("get configuration, index = %d\n", index);
        if (!(device_descriptor = usbd_device_device_descriptor (device)))
            return -1;

        if (index >= device_descriptor->bNumConfigurations) {
            return -1;
        }

        if (!(configuration_descriptor = usbd_device_configuration_descriptor (device, index)))
            return -1;
        copy_config (urb, configuration_descriptor,
                cpu_to_le16(configuration_descriptor->wTotalLength), max);
		}
		break;
		
	case USB_DESCRIPTOR_TYPE_STRING:
    	{
            		
        struct usb_string_descriptor *string_descriptor;
        usb_info("get string, index = %d, max = %d\n", index, max);
        string_descriptor = usbd_get_string (index);
        
//        if (!(string_descriptor))
//            return -1;
        

        copy_config (urb, string_descriptor, string_descriptor->bLength, max);
    	}
    break;
    
	case USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER:
		//TODO
	case USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION:
		//TODO
	case USB_DESCRIPTOR_TYPE_INTERFACE:	//should not see this
	case USB_DESCRIPTOR_TYPE_ENDPOINT:	//should not see this
	default:
		return -1;
	}
	return 0;
 }          

/** called to indicate URB has been received
 * @param[in] urb: pointer to struct urb
 *  
 * Check if this is a setup packet, process the device request, put results
 * back into the urb and return zero or non-zero to indicate success (DATA)
 * or failure (STALL).
 *  
 */ 
int ep0_recv_setup(struct urb* urb)
{
	struct usb_device_request *request;
	struct usb_device_instance *device;
	int address;
	
	//sanity check
	if (!urb || !urb->device)
		return -1;
	
	request = &urb->device_request;
	device = urb->device;
	
	if ((request->bmRequestType & USB_REQ_TYPE_MASK) != USB_REQ_TYPE_STANDARD){
		
		return -1;
	}
	
	/*
	 *  handle standard request
	 */
	
	// do some filter
	switch (device->device_state) {
	case STATE_CREATED:
	case STATE_ATTACHED:
	case STATE_POWERED:
		break;
	
	case STATE_INIT:
	case STATE_DEFAULT:
		switch (request->bRequest) {
		case USB_REQ_GET_STATUS:
		case USB_REQ_GET_INTERFACE:
		case USB_REQ_SYNCH_FRAME:
		case USB_REQ_CLEAR_FEATURE:
		case USB_REQ_SET_FEATURE:
		case USB_REQ_SET_DESCRIPTOR:
		case USB_REQ_SET_INTERFACE:
			// these request not allowed in DEFAULT state
			return -1;
		
		case USB_REQ_SET_CONFIGURATION:
		case USB_REQ_SET_ADDRESS:
		case USB_REQ_GET_DESCRIPTOR:
		case USB_REQ_GET_CONFIGURATION:
			break;
		}
	case STATE_ADDRESSED:
	case STATE_CONFIGURED:
		break;
	case STATE_UNKNOWN:
		return -1;
	}
	
    /* handle all requests that return data (direction bit set on bmRequestType) */
	if ((request->bmRequestType & USB_REQ_DIRECTION_MASK) == USB_REQ_DEVICE2HOST) {
		// device to host
		switch (request->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			usb_info("USB_REQ_GET_DESCRIPTOR\n");
			return ep0_get_descriptor(device, urb,
					le16_to_cpu(request->wLength),
					le16_to_cpu(request->wValue) >> 8,
					le16_to_cpu(request->wValue) & 0xff);
			
		case USB_REQ_GET_CONFIGURATION:
			usb_info("USB_REQ_GET_CONFIGURATION\n");
			return ep0_get_one(device, urb, device->configuration);
		
		case USB_REQ_GET_INTERFACE:
			usb_info("USB_REQ_GET_INTERFACE\n");
			return ep0_get_one(device, urb, device->alternate);
			
		case USB_REQ_GET_STATUS:
			usb_info("USB_REQ_GET_STATUS\n");
			return ep0_get_status(device, urb, request->wIndex,
					request->bmRequestType & USB_REQ_RECIPIENT_MASK);
			
        case USB_REQ_SYNCH_FRAME:
            return -1;
            
        default:
			return -1;
		}
	} else {
		/* handle the requests that do not return data */
		switch (request->bRequest) {
		case USB_REQ_CLEAR_FEATURE:
			usb_info("USB_REQ_CLEAR_FEATURE\n");
		case USB_REQ_SET_FEATURE:
			usb_info("USB_REQ_SET_FEATURE\n");
			switch (request->bmRequestType & USB_REQ_RECIPIENT_MASK) {
			case USB_REQ_RECIPIENT_DEVICE:
				/* DEVICE_REMOTE_WAKEUP or TEST_MODE would be added here 
				 * but we currently not support 
				 */
			case USB_REQ_RECIPIENT_INTERFACE:
			case USB_REQ_RECIPIENT_OTHER:
			default:
				return -1;
			
			case USB_REQ_RECIPIENT_ENDPOINT:
				if (le16_to_cpu (request->wValue) == USB_ENDPOINT_HALT) {
					// implement by udc
				} else {
					return -1;
				}
			}
			return 0;
		
		case USB_REQ_SET_ADDRESS:
			usb_info("USB_REQ_SET_ADDRESS\n");
			if (device->device_state != STATE_DEFAULT)
				return -1;
			address = le16_to_cpu(request->wValue);
			if ((address & 0x7f) != address)
				return -1;
			device->address = address;
			return 0;
			
		case USB_REQ_SET_CONFIGURATION:
			usb_info("USB_REQ_SET_CONFIGURATION\n");
			device->configuration = le16_to_cpu(request->wValue) & 0xff;
			
			device->interface = device->alternate = 0;
			
			return 0;
		
		case USB_REQ_SET_INTERFACE:
			usb_info("USB_REQ_SET_INTERFACE\n");
			device->interface = le16_to_cpu(request->wIndex);
			device->alternate = le16_to_cpu(request->wValue);
			
			return 0;
		
		case USB_REQ_SET_DESCRIPTOR:
			// not supported
			return -1;
			
		default:
			return -1;
		
		}
		
	}
	return -1;
}

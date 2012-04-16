/**
 * @file
 * @brief	MUSB USB Device Controller Driver.
 * @date	2010/07/05
 * 
 * Copyright (c) 2010 Innofidei Inc. All rights reserved.
 */
#include <types.h>
#include <common.h>
#include <byteorder.h>
#include <udc.h>
#include "musb.h"


extern void __iomem* musb_base;


//#define MAX_ENDPOINT	15
//static struct musb_epinfo epinfo[MAX_ENDPOINT * 2];
static struct musb_epinfo epinfo[] = {
	{1, 0, 512},	/* EP1 OUT */
	{1, 1, 512},	/* EP1 IN */
	{2, 0, 512},	/* EP2 OUT */
	{2, 1, 512},	/* EP2 IN */
};

struct musb_epinstance {
    uint8_t  epnum;  	/* endpoint number  */
    uint8_t  epdir;  	/* endpoint direction   */
    struct usb_endpoint_instance *endpoint; /* endpoint instance  */
}; 

static struct musb_epinstance epinstance[] = {
		{1, 0, NULL},	/* EP1 OUT */
		{1, 1, NULL},	/* EP1 IN */
		{2, 0, NULL},	/* EP2 OUT */
		{2, 1, NULL},	/* EP2 IN */	
};


static enum ep0_state_enum {
    IDLE = 0,
    TX,
    RX,
    SET_ADDRESS
} ep0_state = IDLE;

#define SET_EP0_STATE(s)                        \
do {                                    \
    if ((0 <= (s)) && (SET_ADDRESS >= (s))) {           \
        if ((s) != ep0_state) {                 \
            ep0_state = s;                  \
        }                           \
    }								\
} while (0)

static struct urb _ep0_urb;
static struct urb *ep0_urb;
struct usb_endpoint_instance *ep0_endpoint;
static struct usb_device_instance *udc_device;
static int enabled;

static uint8_t ep0_buffer[512];

static void musb_pullup(int is_on)
{
    uint8_t power;

    power = musb_readb(musb_base, MUSB_POWER);
  
    if (is_on) {
        power |= MUSB_POWER_SOFTCONN;
//        power &= ~MUSB_POWER_HSENAB;
        power |= MUSB_POWER_HSENAB;
    } else {
        power &= ~MUSB_POWER_SOFTCONN;
    }
    /* FIXME if on, HdrcStart; if off, HdrcStop */
//    usb_info("gadget  D+ pullup %s\n", is_on ? "on" : "off");
    musb_writeb(musb_base, MUSB_POWER, power);
}

static void musb_generic_disable(void)
{   
    uint16_t temp;
    
    /* disable interrupts */
    musb_writeb(musb_base, MUSB_INTRUSBE, 0);
    musb_writew(musb_base, MUSB_INTRTXE, 0);
    musb_writew(musb_base, MUSB_INTRRXE, 0);

    /* off */
    musb_writeb(musb_base, MUSB_DEVCTL, 0);

    /*  flush pending interrupts */
    temp = musb_readb(musb_base, MUSB_INTRUSB);
    temp = musb_readw(musb_base, MUSB_INTRTX);
    temp = musb_readw(musb_base, MUSB_INTRRX);

}

static void musb_peri_softconnect(void)
{
	uint8_t	power, devctl;
	uint8_t intusb;
	uint16_t intrx, inttx;
	
	/* power off musb */
	musb_pullup(0);
	
	/* read int to clear */
	intusb = musb_readb(musb_base, MUSB_INTRUSB);
	intrx = musb_readw(musb_base, MUSB_INTRRX);
	inttx = musb_readw(musb_base, MUSB_INTRTX);
	
	udelay(1000 * 1000);		/* 1 sec */
	
	/* power on musb */
	musb_pullup(1);

	/* check if device is in b-peripheral mode */
	devctl = musb_readb(musb_base, MUSB_DEVCTL);
	if (!(devctl & MUSB_DEVCTL_BDEVICE) || (devctl & MUSB_DEVCTL_HM) ){
		usb_err("check that mini-B USB cable is attached to the device\n");
	}
	
}

static void musb_peri_resume(void)
{
	/* noop */
}

static void musb_peri_reset(void)
{
	if (ep0_endpoint)
		ep0_endpoint->endpoint_address = 0xff;
	
	musb_writeb(musb_base, MUSB_FADDR, udc_device->address);
	
	SET_EP0_STATE(IDLE);
}

static void musb_peri_rxep_set_halted(int ep_num, int value)
{
	
}

static void musb_peri_txep_set_halted(int ep_num, int value)
{
	
}

static void musb_peri_ep0_stall(void)
{
	uint16_t csr0;
	
	usb_info("%s\n", __FUNCTION__);
	csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
	csr0 |= MUSB_CSR0_P_SENDSTALL;
	musb_writew(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0), csr0);
}

static void musb_peri_ep0_ack_req(void)
{
	uint16_t csr0;
	
	csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
	csr0 |= MUSB_CSR0_P_SVDRXPKTRDY;
	musb_writew(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0), csr0);
}

static void musb_ep0_tx_ready(void)
{
	uint16_t csr0;

	csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
	csr0 |= MUSB_CSR0_TXPKTRDY;
	musb_writew(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0), csr0);
}

static void musb_ep0_tx_ready_and_last(void)
{
	uint16_t csr0;
	
	csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
	csr0 |= (MUSB_CSR0_TXPKTRDY | MUSB_CSR0_P_DATAEND);
	musb_writew(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0), csr0);
}

static void musb_peri_ep0_last(void)
{
	uint16_t csr0;

	csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
	csr0 |= MUSB_CSR0_P_DATAEND;
	musb_writew(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0), csr0);
	
	/*
	 * jimmy.li 2010-08-02 
	 * 
	 * wait the CSR0 DataEnd bit cleared automatically, which fix the bug:
	 * 	work under full speed, when turn the serial trace on, the enumeration will be ok, 
	 * but once turn the serial trace off, the enumeration failed.
	 * 
	 */
	do {
		/* Wait a bit */
		int p, pm = 10;
		for (p = 0; p < pm; p++) {
			csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
			if (!(csr0 & MUSB_CSR0_P_DATAEND))
				break;

			/* Double the delay. */
			udelay(1 << pm);
		}
	} while (0);
}

static void musb_peri_ep0_set_address(void)
{
	uint8_t faddr;
    musb_writeb(musb_base, MUSB_FADDR, udc_device->address);

    /* Verify */
    faddr = musb_readb(musb_base, MUSB_FADDR);
    if (udc_device->address == faddr) {
        SET_EP0_STATE(IDLE);
        usbd_device_event_irq(udc_device, DEVICE_ADDRESS_ASSIGNED, 0);
    }
}

static void musb_peri_ep0_zero_data_request(int err)
{
	musb_peri_ep0_ack_req();

	if (err) {
		musb_peri_ep0_stall();
		SET_EP0_STATE(IDLE);
		
	} else {
		musb_peri_ep0_last();

		/* USBD state */
		switch (ep0_urb->device_request.bRequest) {
		case USB_REQ_SET_CONFIGURATION:
			usb_info("%s, SET_CONFIGURATION\n", __FUNCTION__);
			usbd_device_event_irq(udc_device, DEVICE_CONFIGURED, 0);
			break;
			
		case USB_REQ_CLEAR_FEATURE:
		case USB_REQ_SET_FEATURE:
			switch (ep0_urb->device_request.bmRequestType & USB_REQ_RECIPIENT_MASK) {
			case USB_REQ_RECIPIENT_ENDPOINT:
			{
				uint8_t ep_num = le16_to_cpu(ep0_urb->device_request.wIndex)  & 0x0f;
				int tx = le16_to_cpu(ep0_urb->device_request.wIndex) & USB_DIR_IN;
				if (tx)
					musb_peri_txep_set_halted(ep_num, ep0_urb->device_request.bRequest==USB_REQ_SET_FEATURE);
				else
					musb_peri_rxep_set_halted(ep_num, ep0_urb->device_request.bRequest==USB_REQ_SET_FEATURE);
				break;
			}
			default:
				break;
			}
			break;
		}
		
		/* EP0 state */
		if (USB_REQ_SET_ADDRESS == ep0_urb->device_request.bRequest) {
			usb_info("%s : set_address!\n", __FUNCTION__);
			SET_EP0_STATE(SET_ADDRESS);
		} else {
			SET_EP0_STATE(IDLE);
		}
		
	}	
}

static void musb_peri_ep0_tx_data_request(int err)
{
	if (err) {
		musb_peri_ep0_stall();
		SET_EP0_STATE(IDLE);
	} else {
        musb_peri_ep0_ack_req();

        ep0_endpoint->tx_urb = ep0_urb;
        ep0_endpoint->sent = 0;
        SET_EP0_STATE(TX);
	}
}

static void musb_peri_ep0_rx_data_request(void)
{
	musb_peri_ep0_ack_req();
	
	ep0_endpoint->rcv_urb = ep0_urb;
	ep0_urb->actual_length = 0;
	SET_EP0_STATE(RX);
}

static void musb_peri_ep0_idle(void)
{
	uint16_t	count0;
	int err;
	uint16_t	csr0;
	

	musb_ep_select(musb_base, 0);
	
	csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
	
	if (!(csr0 & MUSB_CSR0_RXPKTRDY))
		goto end;
	
	usb_info("udc ep0 idle: rx pkt rdy\n");
	count0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_COUNT0));
	if (count0 == 0)
		goto end;
	
	if (count0 != 8) {
		musb_peri_ep0_stall();
		goto end;
	}
	
	read_fifo(0, count0, &ep0_urb->device_request);
//	{
//		int i;
//		for(i=0; i<count0; i++)
//			PRINT("%02x ", ((uint8_t*)(&ep0_urb->device_request))[i]);
//		PRINT("\n");
//	}
	if (ep0_urb->device_request.wLength == 0) {
		usb_info("ZERO DATA\n");
		err = ep0_recv_setup(ep0_urb);
		
		/* zero data request */
		musb_peri_ep0_zero_data_request(err);
		
	} else {
		uint8_t reqType = ep0_urb->device_request.bmRequestType;
		
		if (USB_REQ_DEVICE2HOST == (reqType & USB_REQ_DIRECTION_MASK)) {
			usb_info("DEVICE2HOST\n");
			err = ep0_recv_setup(ep0_urb);
			
			musb_peri_ep0_tx_data_request(err);
			
		} else {
			usb_info("HOST2DEVICE\n");
			musb_peri_ep0_rx_data_request();
		}
	}
	
end:
	return;
}

static void musb_peri_ep0_tx(void)
{
	uint16_t csr0;
	int transfer_size = 0;
	unsigned int p, pm;
	
	csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
	usb_info("peri_ep0_tx: csr0 = %x\n", csr0);
	
	/* Check for pending tx */
	if (csr0 & MUSB_CSR0_TXPKTRDY)
		goto end;
	
	/* Check if this is the last packet sent */
	if (ep0_endpoint->sent >= ep0_urb->actual_length) {
		usb_info("peri_ep0_tx: last packet\n");
		SET_EP0_STATE(IDLE);
		goto end;
	}
	
	transfer_size = ep0_urb->actual_length - ep0_endpoint->sent;
	
	if (transfer_size > ep0_endpoint->tx_packetSize)
		transfer_size = ep0_endpoint->tx_packetSize;
	
	usb_info("peri_ep0_tx: transfer_size = %d\n", transfer_size);
	write_fifo(0, transfer_size, &ep0_urb->buffer[ep0_endpoint->sent]);
//	{
//		int i=0;
//		for(i=0; i< transfer_size; i++) {
//			PRINT("%02x ", ep0_urb->buffer[ep0_endpoint->sent + i]);
//			if ((i+1)%16==0)
//				PRINT("\n");
//		}
//	}
	ep0_endpoint->sent += transfer_size;
//	usb_info("%s: sent%d, actual%d\n", __FUNCTION__, ep0_endpoint->sent, ep0_urb->actual_length);
	if (ep0_endpoint->sent >= ep0_urb->actual_length){
		musb_ep0_tx_ready_and_last();
	}else{
		musb_ep0_tx_ready();
	}
	/* Wait a bit */
	pm = 10;
	for (p = 0; p < pm; p++) {
		csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
		if (!(csr0 & MUSB_CSR0_TXPKTRDY))
			break;

		/* Double the delay. */
		udelay(1 << pm);
	}

	if ((ep0_endpoint->sent >= ep0_urb->actual_length) && (p < pm))
		SET_EP0_STATE(IDLE);


end:
	return;
}

static void musb_peri_ep0_rx(void)
{
	uint16_t	csr0;
	uint16_t	count0;
	
	if (ep0_urb->device_request.wLength == ep0_urb->actual_length) {
		musb_peri_ep0_last();
		SET_EP0_STATE(IDLE);
		ep0_recv_setup(ep0_urb);
		return;
	}
	
	csr0 = musb_readw(musb_base, MUSB_CSR0);
	if (!(csr0 & MUSB_CSR0_RXPKTRDY))
		return;
	
	count0 = musb_readw(musb_base, MUSB_CSR0);
	if (count0) {
		struct usb_endpoint_instance *endpoint;
		uint32_t length;
		uint8_t* data;
		
		endpoint = ep0_endpoint;
		if (endpoint && endpoint->rcv_urb) {
			struct urb* urb = endpoint->rcv_urb;
			unsigned int remaining_space = urb->buffer_length - urb->actual_length;
			
			if (remaining_space) {
				int urb_bad = 0;
				
				if (count0 > remaining_space)
					length = remaining_space;
				else
					length = count0;
				
                data = (uint8_t *) urb->buffer;
                data += urb->actual_length;

                /* The common musb fifo reader */
                read_fifo(0, length, data);

                musb_peri_ep0_ack_req();		
                /*
                 * urb's actual_length is updated in
                 * usbd_rcv_complete
                 */
                usbd_rcv_complete(endpoint, length, urb_bad);
                
              
			}
		}
	}
}

static void musb_peri_ep0(void)
{
	uint16_t	csr0;
	
	if (SET_ADDRESS == ep0_state)
		return;
	
	musb_ep_select(musb_base, 0);
	csr0 = musb_readw(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0));
	
	
	if (csr0 & MUSB_CSR0_P_SENTSTALL) {
		csr0 &= ~MUSB_CSR0_P_SENTSTALL;
		musb_writew(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0), csr0);
		SET_EP0_STATE(IDLE);
	}
	
	if (csr0 & MUSB_CSR0_P_SETUPEND) {
		csr0 |= MUSB_CSR0_P_SVDSETUPEND;
		musb_writew(musb_base, MUSB_EP_OFFSET(0, MUSB_CSR0), csr0);
		SET_EP0_STATE(IDLE);
	}
	
	if (IDLE == ep0_state)
		musb_peri_ep0_idle();
	
	if (TX == ep0_state)
		musb_peri_ep0_tx();

	if (RX == ep0_state)
		musb_peri_ep0_rx();
}

static void musb_peri_rx_ack(unsigned int ep)
{
	uint16_t peri_rxcsr;

	peri_rxcsr = musb_readw(musb_base, MUSB_EP_OFFSET(ep, MUSB_RXCSR));
	peri_rxcsr &= ~MUSB_RXCSR_RXPKTRDY;
	musb_writew(musb_base, MUSB_EP_OFFSET(ep, MUSB_RXCSR), peri_rxcsr);
}

static void musb_peri_tx_ready(unsigned int ep)
{
	uint16_t peri_txcsr;

	peri_txcsr = musb_readw(musb_base, MUSB_EP_OFFSET(ep, MUSB_TXCSR));
	peri_txcsr |= MUSB_TXCSR_TXPKTRDY;
	musb_writew(musb_base, MUSB_EP_OFFSET(ep, MUSB_TXCSR), peri_txcsr);
}

static __INLINE__ struct usb_endpoint_instance* GET_RXENDPOINT(unsigned int ep)
{
	return epinstance[((ep-1)<<1) + 1].endpoint;
	
}

static void musb_peri_rx_ep(unsigned int ep)
{
	uint16_t peri_rxcount;

//	usb_info("%s (%d).\n", __FUNCTION__, ep);
	musb_ep_select(musb_base, ep);
	peri_rxcount =	musb_readw(musb_base, MUSB_EP_OFFSET(ep, MUSB_RXCOUNT));
	
//	usb_info("rxcount = %d", peri_rxcount);
	if (peri_rxcount) {
		struct usb_endpoint_instance *endpoint;
		uint32_t length;
		uint8_t *data;

		endpoint = GET_RXENDPOINT(ep);
		if (endpoint && endpoint->rcv_urb && endpoint->rcv_urb->stage==URB_SUBMIT) {
			struct urb *urb = endpoint->rcv_urb;
			unsigned int remaining_space = urb->buffer_length - urb->actual_length;

			if (remaining_space) {
//				int urb_bad = 0; /* urb is good */

				if (peri_rxcount > remaining_space)
					length = remaining_space;
				else
					length = peri_rxcount;

				data = (uint8_t *) urb->buffer;
				data += urb->actual_length;

//				usb_info("read %d\n", length);
				/* The common musb fifo reader */
				read_fifo(ep, length, data);

				musb_peri_rx_ack(ep);

//				/*
//				 * urb's actual_length is updated in
//				 * usbd_rcv_complete
//				 */
//				usbd_rcv_complete(endpoint, length, urb_bad);
				 
				urb->actual_length += length;
				if (urb->actual_length >= urb->buffer_length || peri_rxcount < endpoint->rcv_packetSize) {
						urb->stage = URB_COMPLETE;
						if (urb->complete)
								urb->complete(urb->complete_arg, urb->actual_length);
				}
			} 
		} 

	} 
}

static void musb_peri_rx(uint16_t intr)
{
    unsigned int ep;

    /* Check for EP0 */
    if (0x01 & intr)
        musb_peri_ep0();

    for (ep = 1; ep < 16; ep++) {
        if ((1 << ep) & intr)
            musb_peri_rx_ep(ep);
    }
}

static void musb_peri_tx(uint16_t intr)
{
    /* Check for EP0 */
    if (0x01 & intr) 
        musb_peri_ep0_tx();

}


void udc_irq(void)
{
	if (enabled) {
		uint8_t	intusb;
		
		intusb = musb_readb(musb_base, MUSB_INTRUSB);
		
		if (intusb & MUSB_INTR_RESUME) {
			usb_info("udc resume\n");
			usbd_device_event_irq(udc_device, DEVICE_BUS_ACTIVITY, 0);
			musb_peri_resume();
		}
		
		musb_peri_ep0();
		
		if (intusb & MUSB_INTR_RESET) {
			uint8_t power = musb_readb(musb_base, MUSB_POWER);
			udc_device->speed = (power & MUSB_POWER_HSMODE) ? USB_SPEED_HIGH : USB_SPEED_FULL;
			usb_info("udc reset, speed:%d\n", udc_device->speed);
			usbd_device_event_irq(udc_device, DEVICE_RESET, 0);
			musb_peri_reset();
		}
		
		if (intusb & MUSB_INTR_DISCONNECT) {
			usb_info("udc disconnect\n");
			usbd_device_event_irq(udc_device, DEVICE_RESET, 0);
			musb_peri_reset();
			usbd_device_event_irq(udc_device, DEVICE_HUB_RESET, 0);
		}
		
//		if (intusb & MUSB_INTR_SOF) {
//			usb_info("udc SOF\n");
//			usbd_device_event_irq(udc_device, DEVICE_BUS_ACTIVITY, 0);
//			musb_peri_resume();
//		}
		
		if (intusb & MUSB_INTR_SUSPEND) {
			usb_info("udc suspend\n");
			usbd_device_event_irq(udc_device, DEVICE_BUS_INACTIVE, 0);
		}
		
		if (ep0_state != SET_ADDRESS) {
			uint16_t intrx, inttx;
			
			intrx = musb_readw(musb_base, MUSB_INTRRX);
			inttx = musb_readw(musb_base, MUSB_INTRTX);
			
			if (intrx)
				musb_peri_rx(intrx);
			
			if (inttx)
				musb_peri_tx(inttx);
		} else {
			if (intusb & MUSB_INTR_SOF) {
				uint8_t faddr;
				faddr = musb_readb(musb_base, MUSB_FADDR);
				
				if (udc_device->address != faddr)
					musb_peri_ep0_set_address();
			}
		}
	}
}

void udc_set_nak(int ep_num)
{
	
}

void udc_unset_nak(int ep_num)
{
	
}

int udc_endpoint_write(struct usb_endpoint_instance *endpoint)
{
	int ret = 0;

	/* Transmit only if the hardware is available */
	if (endpoint->tx_urb && endpoint->state == 0 && endpoint->tx_urb->stage==URB_SUBMIT) {
		unsigned int ep = endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK;
		uint16_t peri_txcsr;
		
		musb_ep_select(musb_base, ep);
		peri_txcsr = musb_readw(musb_base, MUSB_EP_OFFSET(ep, MUSB_TXCSR));

		/* Error conditions */
		if (peri_txcsr & MUSB_TXCSR_P_UNDERRUN) {
			usb_info("%s: under run", __FUNCTION__);
			peri_txcsr &= ~MUSB_TXCSR_P_UNDERRUN;
			musb_writew(musb_base, MUSB_EP_OFFSET(ep, MUSB_TXCSR), peri_txcsr);
		}

		/* Check if a packet is waiting to be sent */
		if (!(peri_txcsr & MUSB_TXCSR_TXPKTRDY)) {
			uint32_t length;
			uint8_t *data;
			struct urb *urb = endpoint->tx_urb;
			unsigned int remaining_packet = urb->actual_length - endpoint->sent;

			if (endpoint->tx_packetSize < remaining_packet)
				length = endpoint->tx_packetSize;
			else
				length = remaining_packet;

			usb_info("%s : length = %d\n", __FUNCTION__, length);
			data = (uint8_t *) urb->buffer;
			data += endpoint->sent;

			/* common musb fifo function */
			write_fifo(ep, length, data);

			musb_peri_tx_ready(ep);

			endpoint->last = length;
//			/* usbd_tx_complete will take care of updating 'sent' */
//			usbd_tx_complete(endpoint);
			endpoint->sent += length;
			endpoint->last -= length;
			if (urb->actual_length - endpoint->sent <=0){
				if (urb->zero && length && (length % endpoint->tx_packetSize)==0) {
					usb_info("zero packet send");
					musb_peri_tx_ready(ep);
				}
				urb->stage = URB_COMPLETE;
				urb->actual_length = 0;
				if (urb->complete)
					urb->complete(urb->complete_arg, endpoint->sent);
				//				endpoint->sent = 0;
				//				endpoint->last = 0;				
			}

		}
	} 

	return ret;
}


void udc_setup_ep(struct usb_device_instance *device, unsigned int id,
          struct usb_endpoint_instance *endpoint)
{
	if ( 0 == id) {
		/* EP0 */
		ep0_endpoint = endpoint;
		ep0_endpoint->endpoint_address = 0xff;
		
		ep0_urb = &_ep0_urb;
		memset(ep0_urb, 0, sizeof(struct urb));
		ep0_urb->device = device;
		ep0_urb->endpoint = endpoint;
		ep0_urb->buffer = ep0_buffer;
		ep0_urb->buffer_length = 512;
	} else {
        int ep_addr;
        int  epnum;
        int  epdir; 

        /* Check the direction */
        ep_addr = endpoint->endpoint_address;
        epnum = ep_addr & USB_ENDPOINT_NUMBER_MASK;
        epdir = !(ep_addr & USB_ENDPOINT_DIR_MASK);
    	epinstance[((epnum-1)<<1) + epdir].endpoint = endpoint;
    	
//        if (USB_DIR_IN == (ep_addr & USB_ENDPOINT_DIR_MASK)) {
//            /* IN */
//            epinfo[(id * 2) + 1].epsize = endpoint->tx_packetSize;
//        } else {
//            /* OUT */
//            epinfo[id * 2].epsize = endpoint->rcv_packetSize;
//        }

//        musb_configure_ep(&epinfo[0],
//                  sizeof(epinfo) / sizeof(struct musb_epinfo));
	}
}

void udc_connect(void)
{
	
}

void udc_disconnect(void)
{
	musb_pullup(0);
	musb_generic_disable();
}

void udc_enable(struct usb_device_instance *device)
{
	udc_device = device;
	
	enabled = 1;
}

void udc_disable(void)
{
	enabled = 1;
}

void udc_startup_events(struct usb_device_instance *device)
{
	usbd_device_event_irq(device, DEVICE_INIT, 0);
	
	usbd_device_event_irq(device, DEVICE_CREATE, 0);
	
	usbd_device_event_irq(device, DEVICE_RESET, 0);
	
	udc_enable(device);
}

int udc_init(void)
{
	int ep_loop;
	
//	for (ep_loop = 0; ep_loop < MAX_ENDPOINT * 2; ep_loop++) {
//		epinfo[ep_loop].epnum = (ep_loop / 2) + 1;
//		epinfo[ep_loop].epdir = ep_loop % 2;
//		epinfo[ep_loop].epsize = 0;
//	}
    musb_configure_ep(&epinfo[0],
              sizeof(epinfo) / sizeof(struct musb_epinfo));
    
	musb_peri_softconnect();
	return 0;
}

int udc_start_tx(struct usb_endpoint_instance *endpoint, void* buf, unsigned int size, 
			void (*complete)(void *, int), void *complete_arg, int send_zero)
{
	struct urb * urb = endpoint->tx_urb;
	
	urb->buffer = buf;
	urb->buffer_length = 	urb->actual_length = size;
	urb->stage = URB_SUBMIT;
	urb->complete = complete;
	urb->complete_arg = complete_arg;
	urb->zero = send_zero;
	endpoint->sent = 0;
	udc_endpoint_write(endpoint);

	return 0;
}

int udc_start_rx(struct usb_endpoint_instance *endpoint, void *buf, unsigned int size, 
		void (*complete)(void *, int), void *complete_arg)
{
	struct urb * urb = endpoint->rcv_urb;
	
	urb->buffer = buf;
	urb->buffer_length = size;
	urb->actual_length = 0;
	urb->stage = URB_SUBMIT;
	urb->complete = complete;
	urb->complete_arg = complete_arg;
	
	musb_peri_rx_ep(endpoint->endpoint_address & USB_ENDPOINT_NUMBER_MASK);
	
	return 0;
}

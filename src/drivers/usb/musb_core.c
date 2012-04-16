#include <types.h>
#include <common.h>
#include <bitops.h>
#include "musb.h"
#include <hardware.h>

void __iomem *musb_base = (void __iomem *)USBD_BASE_ADDR;

void musb_configure_ep(struct musb_epinfo *epinfo, uint8_t cnt)
{
    uint16_t csr;
    uint16_t fifoaddr = 64; /* First 64 bytes of FIFO reserved for EP0 */
    uint32_t fifosize;
    uint8_t  idx;
    uint8_t	epnum;
    
    while (cnt--) {
        /* prepare fifosize to write to register */
        fifosize = epinfo->epsize >> 3;
        idx = ffs(fifosize) - 1;
        
        epnum = epinfo->epnum;
        musb_writeb(musb_base, MUSB_INDEX, epnum);
        if (epinfo->epdir) {
			/* Configure fifo size and fifo base address */
			musb_writeb(musb_base, MUSB_TXFIFOSZ, idx);
			musb_writew(musb_base, MUSB_TXFIFOADDR, fifoaddr >> 3);
        
            csr = musb_readw(musb_base, MUSB_EP_OFFSET(epnum, MUSB_TXCSR));

            /* Flush fifo if required */
            if (csr & MUSB_TXCSR_TXPKTRDY)
            	musb_writew(musb_base, MUSB_TXCSR, csr | MUSB_TXCSR_FLUSHFIFO);
        } else {
			/* Configure fifo size and fifo base address */
			musb_writeb(musb_base, MUSB_RXFIFOSZ, idx);
			musb_writew(musb_base, MUSB_RXFIFOADDR, fifoaddr >> 3);

            csr = musb_readw(musb_base, MUSB_EP_OFFSET(epnum, MUSB_RXCSR));

            /* Flush fifo if required */
            if (csr & MUSB_RXCSR_RXPKTRDY)
            	musb_writew(musb_base, MUSB_EP_OFFSET(epnum, MUSB_RXCSR), csr | MUSB_RXCSR_FLUSHFIFO);
        }
        fifoaddr += epinfo->epsize;
        epinfo++;
    }
}

void write_fifo(uint8_t ep, uint32_t length, void *fifo_data)
{
	uint8_t  *data = (uint8_t *)fifo_data;

	/* select the endpoint index */
	musb_writeb(musb_base, MUSB_INDEX, ep);
	
	/* write the data to the fifo */
	while (length--)
		musb_writeb(musb_base, MUSB_FIFO_OFFSET(ep), *data++);
}

void read_fifo(uint8_t ep, uint32_t length, void *fifo_data)
{       
	uint8_t  *data = (uint8_t *)fifo_data;
        
    /* select the endpoint index */
    musb_writeb(musb_base, MUSB_INDEX, ep);
            
    /* read the data to the fifo */
    while (length--)
    	*data++ = musb_readb(musb_base, MUSB_FIFO_OFFSET(ep));
}


#include <types.h>
#include <common.h>
#include <io.h>
#include <hardware.h>
#include <iomux.h>

typedef struct {
	union {
		unsigned long mpv;
		struct {
			u8 fn;
			u8 shift;
			u8 offset;		/* register offset in SYSC */
			u8 reserved;
		};
	};
} mux_pin_s;


void i10_iomux_config(mux_pin_t pins[], int num)
{
	unsigned long sysc_base = SYSC_BASE_ADDR; 
	unsigned long val;
	u8 cached_offset = 0xff;
	int cnt = 0;
	int i;


	for (i=0; i<num; i++) {
		mux_pin_s *p = (mux_pin_s*)(&pins[i]);

		if (p->offset < 0x50 || p->offset > 0xa0)
			continue;

		if (cached_offset != p->offset) {
			if (cnt)
				SET_REG(sysc_base + cached_offset, val);

			cached_offset = p->offset;
			val = GET_REG(sysc_base + cached_offset);
			cnt = 0;
		}

		val &= ~(0xf << p->shift);
		val |= (p->fn << p->shift);
		cnt += 1;
	}

	if (cnt)
		SET_REG(sysc_base + cached_offset, val);
}

#include <kern/time.h>
#include <kern/cpu.h>
#include <inc/assert.h>

static unsigned int ticks;

void
time_init(void)
{
	ticks = 0;
}

// This should be called once per timer interrupt.  A timer interrupt
// fires every 10 ms.
void
time_tick(void)
{
	static unsigned char curr_ticks = 0;

	if (++curr_ticks == ncpu) {
		curr_ticks = 0;
		ticks++;
	}

	if (ticks * 10 < ticks)
		panic("time_tick: time overflowed");
}

unsigned int
time_msec(void)
{
	return ticks * 10;
}

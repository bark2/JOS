#include "../inc/env.h"
#include "ns.h"

extern union Nsipc nsipcbuf;
static struct jif_pkt *pkt = (struct jif_pkt *)REQVA;

/* #define RX_QUEUE_SZ 32 */
/* static volatile uint8_t packets[RX_QUEUE_SZ][2048] __attribute__((aligned(PGSIZE))); */

static void
hexdump(const char *prefix, const void *data, int len)
{
	int i;
	char buf[80];
	char *end = buf + sizeof(buf);
	char *out = NULL;
	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			out = buf + snprintf(buf, end - buf, "%s%04x   ", prefix, i);
		out += snprintf(out, end - out, "%02x", ((uint8_t *)data)[i]);
		if (i % 16 == 15 || i == len - 1)
			cprintf("%.*s\n", out - buf, buf);
		if (i % 2 == 1)
			*(out++) = ' ';
		if (i % 16 == 7)
			*(out++) = ' ';
	}
}

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	uint32_t i, req, whom;
	int r;

	while (1) {
		sys_packet_receive(&nsipcbuf);
		/* hexdump("input: ", nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len); */
		/* cprintf("input: len %x\n\n", nsipcbuf.pkt.jp_len); */
		ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_U | PTE_W | PTE_P);
	}
}

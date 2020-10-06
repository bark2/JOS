#include "ns.h"

#define TX_QUEUE_SZ 32

extern union Nsipc nsipcbuf;
static struct jif_pkt *pkt = (struct jif_pkt *)REQVA;
static volatile uint32_t tx_packets[TX_QUEUE_SZ][PGSIZE] __attribute__((aligned(PGSIZE)));

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
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver

	uint32_t req, whom;
	int perm, r;
	int offset = 0;

	while (1) {
		req = ipc_recv((int32_t *)&whom, (void *)&tx_packets[offset], 0);
		if (req == NSREQ_OUTPUT) {
			r = sys_packet_transmit(((union Nsipc *)(tx_packets[offset]))->pkt.jp_data,
						((union Nsipc *)(tx_packets[offset]))->pkt.jp_len);
			offset = (offset + 1) % TX_QUEUE_SZ;
		} else {
			cprintf("Invalid request code %d from %08x\n", req, whom);
			r = -E_INVAL;
		}
	}
}

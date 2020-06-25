#include <kern/e1000.h>
#include <kern/pci.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <inc/env.h>
#include <inc/stdio.h>
#include <inc/string.h>

static volatile uint8_t *bar0;
#define bar0_reg32(reg) (*(volatile uint32_t *)(bar0 + reg))

/* static volatile uint8_t tx_packets[TX_QUEUE_SZ][MAX_PACKET_LEN]; */
static struct PageInfo *rx_packets[RX_QUEUE_SZ];

#define PACKET_FIFO_SZ 256
static struct PageInfo *packet_fifo[PACKET_FIFO_SZ], **packet_fifo_head, **packet_fifo_tail;
static uint32_t packet_fifo_count = 0;

static struct tx_desc {
	uint64_t addr;
	uint16_t length;
	uint8_t csum;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
} volatile tx_queue[TX_QUEUE_SZ] __attribute__((aligned(128)));

static struct rx_desc {
	uint64_t addr;	 /* Address of the descriptor's data buffer */
	uint16_t length; /* Length of data DMAed into data buffer */
	uint16_t csum;	 /* Packet checksum */
	uint8_t status;	 /* Descriptor status */
	uint8_t errors;	 /* Descriptor Errors */
	uint16_t special;
} volatile rx_queue[RX_QUEUE_SZ] __attribute__((aligned(128)));

static void
eeprom_read_mac_address(uint16_t buf[3])
{
	int i;
	for (i = 0; i < 3; i++) {
		bar0_reg32(E1000_EERD) = (i << E1000_EEPROM_RW_ADDR_SHIFT) | E1000_EEPROM_RW_REG_START;
		while (!(bar0_reg32(E1000_EERD) & E1000_EEPROM_RW_REG_DONE))
			;
		buf[i] = bar0_reg32(E1000_EERD) >> E1000_EEPROM_RW_REG_DATA;
	}
}

int
e1000_init(struct pci_func *pcif)
{
	pci_func_enable(pcif);
	bar0 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);

	// init tx queue
	bar0_reg32(E1000_TDBAL) = PADDR((void *)tx_queue);
	bar0_reg32(E1000_TDLEN) = sizeof(tx_queue);
	bar0_reg32(E1000_TDH) = 0;
	bar0_reg32(E1000_TDT) = 0;

	bar0_reg32(E1000_TCTL) |= E1000_TCTL_EN; // enable tx queue
	bar0_reg32(E1000_TCTL) |= 0x40000;	 // collision distance default
	bar0_reg32(E1000_TIPG) = 0x10;		 // default packet gap

	volatile struct tx_desc *tx_tail = &tx_queue[0];
	for (; tx_tail != &tx_queue[TX_QUEUE_SZ]; tx_tail++)
		tx_tail->status |= E1000_TXD_STAT_DD; // unset descriptor done

	// init rx queue
	// configure the recieve address as the device mac address
	/* uint8_t mac[8] = { */
	/* [7] = 0x00, [6] = 0x00, [5] = 0x56, [4] = 0x34, [3] = 0x12, [2] = 0x00, [1] = 0x54, [0] = 0x52, */
	/* }; */
	uint8_t mac[8] = {};
	eeprom_read_mac_address((uint16_t *)mac);
	memcpy((void *)&bar0_reg32(E1000_RAL), &mac[0], sizeof(uint32_t));
	memcpy((void *)&bar0_reg32(E1000_RAH), &mac[4], sizeof(uint32_t));
	bar0_reg32(E1000_RAH) |= E1000_RAH_AV;
	bar0_reg32(E1000_MTA) = 0;

	bar0_reg32(E1000_RDBAL) = PADDR((void *)rx_queue);
	bar0_reg32(E1000_RDLEN) = sizeof(rx_queue);
	bar0_reg32(E1000_RDH) = 0;
	bar0_reg32(E1000_RDT) = RX_QUEUE_SZ - 1;

	volatile struct rx_desc *rx_tail = &rx_queue[0];
	for (; rx_tail != &rx_queue[RX_QUEUE_SZ]; rx_tail++) {
		rx_tail->status = 0;
		/* rx_tail->addr = PADDR((void *)&rx_packets[rx_tail - rx_queue]); */
		struct PageInfo *page = page_alloc(ALLOC_ZERO);
		if (!page)
			panic("page_alloc");
		rx_packets[rx_tail - rx_queue] = page;
		rx_tail->addr = page2pa(page) + sizeof(int);
	}

	// timer interupt
	bar0_reg32(E1000_IMC) = 0xff;
	bar0_reg32(E1000_IMS) |= E1000_IMS_RXT0;
	bar0_reg32(E1000_IMS) |= E1000_IMS_RXDMT0;
	bar0_reg32(E1000_IMS) |= E1000_IMS_RXO;

	bar0_reg32(E1000_RADV) = 0x1ff;
	bar0_reg32(E1000_RDTR) = 0x200;

	//
	bar0_reg32(E1000_RCTL) = E1000_RCTL_EN; // enable tx queue
	bar0_reg32(E1000_RCTL) |= E1000_RCTL_RDMTS_HALF;
	bar0_reg32(E1000_RCTL) |= E1000_RCTL_SZ_2048;
	bar0_reg32(E1000_RCTL) |= E1000_RCTL_SECRC;

	packet_fifo_tail = packet_fifo;
	packet_fifo_head = packet_fifo;
	packet_fifo_count = 0;

	return 0;
}

int
e1000_packet_transmit(const uint8_t packet[], int len)
{
	assert(len < MAX_PACKET_LEN);

	volatile struct tx_desc *tail = &tx_queue[bar0[E1000_TDT]];
	while (!(tail->status & E1000_TXD_STAT_DD)) // TODO
		;

	tail->cmd &= ~E1000_TXD_CMD_DEXT;   // legacy descriptor
	tail->cmd |= E1000_TXD_CMD_RS;	    // report status
	tail->cmd |= E1000_TXD_CMD_EOP;	    // end of packet
	tail->status &= ~E1000_TXD_STAT_DD; // unset descriptor done

	// copy data

	/* int tail_off = tail - tx_queue; */
	/* struct PageInfo *pg = page_lookup(curenv->env_pgdir, (void *)packet, 0); */
	/* tail->addr = PADDR((void *)tx_packets[tail_off]); */
	/* tail->length = len; */
	/* memcpy((void *)tx_packets[tail_off], packet, len); */

	// Zero-copy challenge!
	// TODO: make the user page entry as copy on write until the page is transmitted
	struct PageInfo *pg = page_lookup(curenv->env_pgdir, (void *)packet, 0);
	tail->addr = page2pa(pg) + PGOFF(packet);
	tail->length = len;

	// commit by update tx tail
	bar0_reg32(E1000_TDT) = (bar0_reg32(E1000_TDT) + 1) % TX_QUEUE_SZ;

	return 0;
}

int
e1000_packet_receive()
{
	struct PageInfo *new_page;
	uint32_t icr = bar0_reg32(E1000_ICR);
	uint32_t ims = bar0_reg32(E1000_IMS);
	volatile uint32_t *rdh = &bar0_reg32(E1000_RDH);

	int tail_next_off = (bar0[E1000_RDT] + 1) % RX_QUEUE_SZ;
	volatile struct rx_desc *tail_next = &rx_queue[tail_next_off];

	if (!(tail_next->status & E1000_TXD_STAT_DD) || (tail_next_off == *rdh)) {
		if (icr & ims) 
			return E1000_RECV_QUEUE_EMPTY;
		return E1000_RECV_UNREQUESTED_INTERRUPT;
	}

	do {
		assert(packet_fifo_count != PACKET_FIFO_SZ);

		assert(bar0_reg32(E1000_RDT) + 1 != *rdh);
		bar0_reg32(E1000_RDT) = tail_next_off;	      // free current desc
		*(int *)page2kva(rx_packets[tail_next_off]) = // update length, now ready to ship to user
			tail_next->length;

		*packet_fifo_head = rx_packets[tail_next_off]; // add to packet list
		packet_fifo_head =
			packet_fifo_head == &packet_fifo[PACKET_FIFO_SZ] ? packet_fifo : packet_fifo_head + 1;
		packet_fifo_count++;

		// alloc new page for descriptor (the user now ownes it)
		if (!(new_page = page_alloc(ALLOC_ZERO)))
			panic("page_alloc");

		rx_packets[tail_next_off] = new_page;
		tail_next->addr = page2pa(new_page) + sizeof(int);
		tail_next->status = 0;

		tail_next_off = (tail_next_off + 1) % RX_QUEUE_SZ;
		tail_next = &rx_queue[tail_next_off];
	} while ((tail_next->status & E1000_TXD_STAT_DD) && (tail_next_off != *rdh));

	return E1000_RECV_SUCCESS;
}

int
packet_receive(pde_t *pgdir, void *packet)
{
	// copy data
	if (packet_fifo_count == 0)
		return E1000_RECV_QUEUE_EMPTY;
	if (page_insert(pgdir, *packet_fifo_tail, packet, PTE_U | PTE_W) < 0)
		panic("page_insert");

	packet_fifo_count--;
	packet_fifo_tail = packet_fifo_tail == &packet_fifo[PACKET_FIFO_SZ] ? packet_fifo : packet_fifo_tail + 1;

	return E1000_RECV_SUCCESS;
}

#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>
#include <inc/env.h>

#define E1000_DEV_ID_82540EM 0x100E

#define E1000_TDBAL 0x03800 /* TX Descriptor Base Address Low - RW */
#define E1000_TDH 0x03810   /* TX Descriptor Head - RW */
#define E1000_TDT 0x03818   /* TX Descripotr Tail - RW */
#define E1000_TDLEN 0x03808 /* TX Descriptor Length - RW */

#define E1000_TCTL 0x00400	   /* TX Control - RW */
#define E1000_TCTL_EN 0x00000002   /* enable tx */
#define E1000_TCTL_COLD 0x003ff000 /* collision distance */
#define E1000_TIPG 0x00410	   /* TX Inter-packet gap -RW */

#define E1000_TXDCTL 0x03828 /* TX Descriptor Control - RW */

#define E1000_TXD_CMD_DEXT (1 << 5)  /* Descriptor extension (0 = legacy) */
#define E1000_TXD_CMD_RS (1 << 3)    /* Report Status */
#define E1000_TXD_CMD_EOP (1 << 0)   /* End of Packet */
#define E1000_TXD_STAT_DD 0x00000001 /* Descriptor Done */

//

#define E1000_RDBAL 0x02800	/* RX Descriptor Base Address Low - RW */
#define E1000_RDLEN 0x02808	/* RX Descriptor Length - RW */
#define E1000_RDH 0x02810	/* RX Descriptor Head - RW */
#define E1000_RDT 0x02818	/* RX Descriptor Tail - RW */
#define E1000_RCTL 0x00100	/* RX Control - RW */
#define E1000_RXDCTL 0x02828	/* RX Descriptor Control queue 0 - RW */
#define E1000_MTA 0x05200	/* Multicast Table Array - RW Array */
#define E1000_RAH_AV 0x80000000 /* Receive descriptor valid */

#define E1000_RCTL 0x00100	       /* RX Control - RW */
#define E1000_RCTL_EN 0x00000002       /* enable */
#define E1000_RCTL_MPE 0x00000010      /* multicast promiscuous enab */
#define E1000_RCTL_LPE 0x00000020      /* long packet enable */
#define E1000_RCTL_LBM_NO 0x00000000   /* no loopback mode */
#define E1000_RCTL_LBM_MAC 0x00000040  /* MAC loopback mode */
#define E1000_RCTL_LBM_SLP 0x00000080  /* serial link loopback mode */
#define E1000_RCTL_LBM_TCVR 0x000000C0 /* tcvr loopback mode */
#define E1000_RCTL_SECRC 0x04000000    /* Strip Ethernet CRC */
#define E1000_RCTL_SZ_2048 0x00000000  /* rx buffer size 2048 */
#define E1000_RCTL_BAM 0x00008000      /* broadcast enable */
#define E1000_RCTL_SBP 0x00000004      /* store bad packet */
#define E1000_RCTL_LPE 0x00000020      /* long packet enable */

#define E1000_RXD_STAT_DD 0x01 /* Descriptor Done */

// interrupts
#define E1000_ICR 0x000C0	  /* Interrupt Cause Read - R/clr */
#define E1000_ICR_RXT0 0x00000080 /* rx timer intr (ring 0) */
#define E1000_ITR 0x000C4	  /* Interrupt Throttling Rate - RW */

#define E1000_IMS 0x000D0	      /* Interrupt Mask Set - RW */
#define E1000_IMS_RXT0 E1000_ICR_RXT0 /* rx timer intr */
#define E1000_IMC 0x000D8	      /* Interrupt Mask Clear - WO */
#define E1000_IMC_RXT0 E1000_ICR_RXT0 /* rx timer intr */

#define E1000_RDTR 0x02820 /* RX Delay Timer - RW */
#define E1000_RADV 0x0282C /* RX Interrupt Absolute Delay Timer - RW */

#define E1000_ICR_RXDMT0 0x00000010	  /* rx desc min. threshold (0) */
#define E1000_IMS_RXDMT0 E1000_ICR_RXDMT0 /* rx desc min. threshold */
#define E1000_RCTL_RDMTS_HALF 0x00000000  /* rx desc min threshold size */
#define E1000_RCTL_RDMTS_QUAT 0x00000100  /* rx desc min threshold size */
#define E1000_RCTL_RDMTS_EIGTH 0x00000200 /* rx desc min threshold size */

#define E1000_ICR_RXO 0x00000040    /* rx overrun */
#define E1000_ICS_RXO E1000_ICR_RXO /* rx overrun */
#define E1000_IMS_RXO E1000_ICR_RXO /* rx overrun */
#define E1000_IMC_RXO E1000_ICR_RXO /* rx overrun */
//

#define E1000_TPR 0x040D0 /* Total Packets RX - R/clr */

#define E1000_CTRL 0x00000	  /* Device Control - RW */
#define E1000_CTRL_SLU 0x00000040 /* Set link up (Force Link) */

//

#define E1000_EERD 0x00014 /* EEPROM Read - RW */
#define E1000_EEPROM_RW_REG_DATA   16   /* Offset to data in EEPROM read/write registers */
#define E1000_EEPROM_RW_REG_DONE   0x10 /* Offset to READ/WRITE done bit */
#define E1000_EEPROM_RW_REG_START  1    /* First bit for telling part to start operation */
#define E1000_EEPROM_RW_ADDR_SHIFT 8    /* Shift to the address bits */
#define E1000_EEPROM_POLL_WRITE    1    /* Flag for polling for write complete */
#define E1000_EEPROM_POLL_READ     0    /* Flag for polling for read complete */

//

#define E1000_RAL 0x5400
#define E1000_RAH 0x5404
#define TX_QUEUE_SZ 32
#define RX_QUEUE_SZ 150
#define MAX_PACKET_LEN 1518

enum { E1000_RECV_BAD_INT = -1,
       E1000_RECV_SUCCESS,
       E1000_RECV_QUEUE_EMPTY,
       E1000_RECV_UNREQUESTED_INTERRUPT
};

int e1000_init(struct pci_func *pcif);
int e1000_packet_transmit(const uint8_t packet[], int len);

struct e1000_packet_receive_res {
	uint8_t packets_start;
	uint8_t npackets;
	int packet_lengths[0];
};
typedef uint8_t (*rx_packets_t)[MAX_PACKET_LEN];

int e1000_packet_receive();
int packet_receive(pde_t *pgdir, void *packet);

#endif // JOS_KERN_E1000_H

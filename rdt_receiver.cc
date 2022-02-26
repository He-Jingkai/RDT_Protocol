/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or
 *       reordering.  You will need to enhance it to deal with all these
 *       situations.  In this implementation, the packet format is laid out as
 *       the following:
 *
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */

#include "rdt_receiver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crc.h"
#include "rdt_struct.h"

void Receiver_ToLowerLayer(struct packet *pkt);

u_int16_t Receiver_nextseqnum = 1;

void ACK(u_int16_t seqnum) {
  fprintf(stdout, "[receiver]send ack of %d\n", seqnum);
  struct packet *pkt = new packet;
  *(u_int16_t *)pkt->data = seqnum;
  pkt->data[2] = crc::getCRC(pkt->data, 2);
  Receiver_ToLowerLayer(pkt);
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init() {
  fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some
   memory you allocated in Receiver_init(). */
void Receiver_Final() {
  fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the
   receiver */
/* Packet Structure:
  |size(1 byte)|sequence_num(1 byte)|checksum(1 byte)
 */
void Receiver_FromLowerLayer(struct packet *pkt) {
  /* 1-byte header indicating the size of the payload */
  int header_size = 4;

  /* construct a message and deliver to the upper layer */
  struct message *msg = (struct message *)malloc(sizeof(struct message));
  ASSERT(msg != NULL);

  msg->size = (u_int8_t)pkt->data[0];
  u_int16_t sequence_num = *(u_int16_t *)(pkt->data + 1);
  u_int8_t pkt_checksum = (u_int8_t)pkt->data[3];
  u_int8_t caculated_checksum = crc::getCRC((pkt->data + 4), (int)pkt->data[0]);
  fprintf(stdout,
          "[receiver]receive packet of %d, pkt_checksum = %d, "
          "caculated_checksum = %d\n",
          sequence_num, pkt_checksum, caculated_checksum);

  /* sanity check in case the packet is corrupted */
  if (msg->size < 0 || msg->size > RDT_PKTSIZE - header_size ||
      pkt_checksum != caculated_checksum ||
      sequence_num != Receiver_nextseqnum) {
    ACK(Receiver_nextseqnum - 1);
    return;
  }

  msg->data = (char *)malloc(msg->size);
  ASSERT(msg->data != NULL);
  memcpy(msg->data, pkt->data + header_size, msg->size);
  Receiver_ToUpperLayer(msg);
  ACK(Receiver_nextseqnum);
  Receiver_nextseqnum++;

  /* don't forget to free the space */
  if (msg->data != NULL) free(msg->data);
  if (msg != NULL) free(msg);
}

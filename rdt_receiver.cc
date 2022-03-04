/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 */

#include "rdt_receiver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include "crc.h"
#include "rdt_struct.h"

extern int HEADER_SIZE;
std::map<uint16_t, message *> Receiver_packet_buffer;

void Receiver_ToLowerLayer(struct packet *pkt);

uint16_t Receiver_nextseqnum = 1;

void Receiver_ToUpperLayer_rewrite(struct message *msg) {
  Receiver_ToUpperLayer(msg);
  if (msg->data != NULL) free(msg->data);
  if (msg != NULL) free(msg);
}
void ACK(uint16_t seqnum) {
#ifdef DEBUG
  fprintf(stdout, "[receiver]send ack of %d\n", seqnum);
#endif
  struct packet *pkt = new packet;
  *(uint16_t *)pkt->data = seqnum;
  pkt->data[2] = crc::crc4_itu(pkt->data, 2);
  pkt->data[3] = crc::crc8_rohc(pkt->data, 3);
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
void Receiver_FromLowerLayer(struct packet *pkt) {
  if (crc::crc4_itu(pkt->data, RDT_PKTSIZE) ||
      crc::crc8_rohc(pkt->data, RDT_PKTSIZE - 1) ||
      crc::crc6_itu(pkt->data, RDT_PKTSIZE - 2) || (uint8_t)pkt->data[0] < 1 ||
      (uint8_t)pkt->data[0] > RDT_PKTSIZE - HEADER_SIZE) {
    ACK(Receiver_nextseqnum - 1);
    return;
  }

  /* construct a message and deliver to the upper layer */
  struct message *msg = (struct message *)malloc(sizeof(struct message));
  msg->size = (uint8_t)pkt->data[0];
  uint16_t sequence_num = *(uint16_t *)(pkt->data + 1);

#ifdef DEBUG
  fprintf(stdout, "[receiver]receive packet of %d\n", sequence_num);
#endif
  if (sequence_num < Receiver_nextseqnum) {
    ACK(Receiver_nextseqnum - 1);
    return;
  }
  if (sequence_num > Receiver_nextseqnum) {
    ACK(Receiver_nextseqnum - 1);
    msg->data = (char *)malloc(msg->size);
    memcpy(msg->data, pkt->data + HEADER_SIZE, msg->size);
    Receiver_packet_buffer[sequence_num] = msg;
    return;
  }

  msg->data = (char *)malloc(msg->size);
  memcpy(msg->data, pkt->data + HEADER_SIZE, msg->size);
  Receiver_ToUpperLayer_rewrite(msg);

  ACK(Receiver_nextseqnum);
  Receiver_nextseqnum++;
  while (Receiver_packet_buffer.find(Receiver_nextseqnum) !=
         Receiver_packet_buffer.end()) {
    msg = Receiver_packet_buffer[Receiver_nextseqnum];
    Receiver_ToUpperLayer_rewrite(msg);
    ACK(Receiver_nextseqnum);
    Receiver_nextseqnum++;
  }
}

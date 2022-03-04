/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 */

#include "rdt_sender.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <map>

#include "crc.h"
#include "rdt_struct.h"

#define WINDOW_SIZE 10
#define CHECKSUM_SIZE 3
#define TIMEOUT 0.3
int HEADER_SIZE = 3;
uint16_t Sender_nextseqnum = 1;
uint16_t send_base = 1;
uint16_t total_packet_num = 0;
std::map<uint16_t, packet *> packet_buffer;

void Sender_StartTimer(double timeout);
void Sender_StopTimer();

/*move the window, called after @nextsequence or @send_base changes*/
void Sender_moveWindow() {
  while (Sender_nextseqnum < send_base + WINDOW_SIZE &&
         Sender_nextseqnum <= total_packet_num) {
#ifdef DEBUG
    fprintf(stdout, "[sender]sender send packet %d\n", Sender_nextseqnum);
#endif
    Sender_ToLowerLayer(packet_buffer[Sender_nextseqnum]);
    Sender_nextseqnum++;
  }
}

void addChecksum(packet *pkt) {
  (pkt->data)[RDT_PKTSIZE - 3] = crc::crc6_itu(pkt->data, RDT_PKTSIZE - 3);
  (pkt->data)[RDT_PKTSIZE - 2] = crc::crc8_rohc(pkt->data, RDT_PKTSIZE - 2);
  (pkt->data)[RDT_PKTSIZE - 1] = crc::crc4_itu(pkt->data, RDT_PKTSIZE - 1);
}

/* change @send_base and delete packets before @sequenceNum(included) */
void Sender_ACKPacket(int sequenceNum) {
  send_base = sequenceNum + 1;
  // for (int i = 0; i <= sequenceNum; i++)
  //   if (packet_buffer.find(i) != packet_buffer.end()) {
  //     delete packet_buffer[i];
  //     packet_buffer.erase(i);
  //   }
}

/* sender initialization, called once at the very beginning */
void Sender_Init() {
  fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some
   memory you allocated in Sender_init(). */
void Sender_Final() {
  fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the
   sender */
void Sender_FromUpperLayer(struct message *msg) {
  /* maximum payload size */
  uint8_t maxpayload_size = RDT_PKTSIZE - HEADER_SIZE - CHECKSUM_SIZE;

  /* the cursor always points to the first unsent byte in the message */
  int cursor = 0;
  while (msg->size - cursor > maxpayload_size) {
    packet *pkt = new packet;
    uint16_t sequence_num = total_packet_num + 1;
    (pkt->data)[0] = maxpayload_size;
    *(uint16_t *)(pkt->data + 1) = sequence_num;
    memcpy(pkt->data + HEADER_SIZE, msg->data + cursor, maxpayload_size);
    addChecksum(pkt);
    packet_buffer[sequence_num] = pkt;
    cursor += maxpayload_size;
    total_packet_num++;
  }

  if (msg->size > cursor) {
    packet *pkt = new packet;
    uint16_t sequence_num = total_packet_num + 1;
    pkt->data[0] = msg->size - cursor;
    *(uint16_t *)(pkt->data + 1) = sequence_num;
    memcpy(pkt->data + HEADER_SIZE, msg->data + cursor, pkt->data[0]);
    addChecksum(pkt);
    packet_buffer[sequence_num] = pkt;
    total_packet_num++;
  }
#ifdef DEBUG
  fprintf(stdout, "[sender]total packets num %d\n", total_packet_num);
#endif
  Sender_moveWindow();
  Sender_StartTimer(TIMEOUT);
}

/* event handler, called when a packet is passed from the lower layer at the
   sender */
void Sender_FromLowerLayer(struct packet *pkt) {
  /*Check Corruption*/
  if (crc::crc8_rohc(pkt->data, 4) || crc::crc4_itu(pkt->data, 3)) return;
  uint16_t ack_sequence_num = *(uint16_t *)(pkt->data);
#ifdef DEBUG
  fprintf(stdout, "[sender]receive ack of %d\n", ack_sequence_num);
#endif
  Sender_ACKPacket(ack_sequence_num);
  Sender_moveWindow();
  if (send_base == Sender_nextseqnum)
    Sender_StopTimer();
  else
    Sender_StartTimer(TIMEOUT);
}

/* event handler, called when the timer expires */
void Sender_Timeout() {
  Sender_StartTimer(TIMEOUT);
  // send_base(included) to nextseqnum-1(included)
  for (uint16_t i = send_base; i <= Sender_nextseqnum - 1; i++)
    Sender_ToLowerLayer(packet_buffer[i]);
}
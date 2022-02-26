/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
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

#include "rdt_sender.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <map>

#include "crc.h"
#include "rdt_struct.h"

#define WINDOW_SIZE 10
#define TIMEOUT 0.3
u_int16_t Sender_nextseqnum = 1;
u_int16_t send_base = 1;
u_int16_t total_packet_num = 0;
// The first packet in packer_buffer is send_base
std::map<u_int16_t, packet *> packet_buffer;

void Sender_StartTimer(double timeout);
void Sender_StopTimer();

/*move the window, called after @nextsequence or @send_base changes*/
void Sender_moveWindow() {
  while (Sender_nextseqnum < send_base + WINDOW_SIZE &&
         Sender_nextseqnum <= total_packet_num) {
    fprintf(stdout, "[sender]sender send packet %d\n", Sender_nextseqnum);

    Sender_ToLowerLayer(packet_buffer[Sender_nextseqnum]);
    Sender_nextseqnum++;
  }
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
  /* 1-byte header indicating the size of the payload */
  fprintf(stdout, "[sender]message come\n");
  u_int16_t header_size = 4;

  /* maximum payload size */
  u_int8_t maxpayload_size = RDT_PKTSIZE - header_size;

  /* split the message if it is too big */

  /* the cursor always points to the first unsent byte in the message */
  int cursor = 0;
  while (msg->size - cursor > maxpayload_size) {
    packet *pkt = new packet;
    u_int16_t sequence_num = total_packet_num + 1;
    (pkt->data)[0] = maxpayload_size;
    *(u_int16_t *)(pkt->data + 1) = sequence_num;
    memcpy(pkt->data + header_size, msg->data + cursor, maxpayload_size);
    (pkt->data)[3] = crc::getCRC((pkt->data + 4), (int)pkt->data[0]);
    packet_buffer[sequence_num] = pkt;
    cursor += maxpayload_size;
    total_packet_num++;
  }

  if (msg->size > cursor) {
    packet *pkt = new packet;
    u_int16_t sequence_num = total_packet_num + 1;
    pkt->data[0] = msg->size - cursor;
    *(u_int16_t *)(pkt->data + 1) = sequence_num;
    memcpy(pkt->data + header_size, msg->data + cursor, pkt->data[0]);
    (pkt->data)[3] = crc::getCRC((pkt->data + 4), (int)pkt->data[0]);
    packet_buffer[sequence_num] = pkt;
    total_packet_num++;
  }

  fprintf(stdout, "[sender]total packets num %d\n", total_packet_num);
  if (total_packet_num != packet_buffer.size()) exit(0);

  // for (std::pair<u_int16_t, packet *> packet_in_buffer : packet_buffer)
  //   fprintf(stdout, "[sender]sequence_num %d in buffer\n",
  //           packet_in_buffer.first);

  Sender_moveWindow();
}

/* event handler, called when a packet is passed from the lower layer at the
   sender */
void Sender_FromLowerLayer(struct packet *pkt) {
  u_int16_t ack_sequence_num = *(u_int16_t *)(pkt->data);
  /*Check Corruption*/
  u_int8_t pkt_checksum = (u_int8_t)pkt->data[2];
  u_int8_t caculated_checksum = crc::getCRC(pkt->data, 2);
  if (pkt_checksum != caculated_checksum) return;
  fprintf(stdout, "[sender]receive ack of %d\n", ack_sequence_num);
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
  for (u_int16_t i = send_base; i <= Sender_nextseqnum - 1; i++)
    Sender_ToLowerLayer(packet_buffer[i]);
}
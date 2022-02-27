# RDT_Protocol

## 目录
- [目录](#%E7%9B%AE%E5%BD%95)
- [packet结构](#packet结构)
- [crc循环检验和](#crc循环检验和)

## Packet结构

rdt中表示包的数据结构为````struct packet````, 是一个固定长度的字符数组。 
````
struct packet {
    char data[RDT_PKTSIZE];
};
````
在结构上，Packet分为header段和数据段，header中包括有效内容的长度(````size````)，packet的序列号(````sequence number````)和循环校验和(````checksum````)，其中，````size````用一个字节表示（范围为0-255），````sequence number````用两个字节表示（范围为0-65535），````checksum````用两个字节表示(crc循环检验和为一个字节，但是在测试中发现单个crc循环检验和无法在错误率较高时检测出全部错误的包，因此我们使用两次crc循环检验和来提高发现错误的概率，详见[crc循环检验和](#crc循环检验和))，为了方便crc检验，````checksum````放在packet的末尾。


Packet的结构如下:


````
|size(u_int8_t)|sequence number(u_int16_t)|data(RDT_PKTSIZE-5)|crc_checksum-1(u_int8_t)|crc_checksum-2(u_int8_t)|  
````

## CRC循环检验和
为了方便receiver在收到包后检查包在传输中是否出错，我们添加了crc checksum，

````
u_int8_t crc4_itu(char const message[], int nBytes) {
  u_int8_t i;
  u_int8_t crc = 0xFF;  // Initial value
  while (nBytes--) {
    crc ^= *message++;  // crc ^= *data; data++;
    for (i = 0; i < 8; ++i) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xE0;  // 0xE0 = reverse 0x07
      else
        crc = (crc >> 1);
    }
  }
  return crc;
}
````


````
u_int8_t crc8_rohc(char const message[], int nBytes) {
  u_int8_t i;
  u_int8_t crc = 0;  // Initial value
  while (nBytes--) {
    crc ^= *message++;  // crc ^= *data; data++;
    for (i = 0; i < 8; ++i) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0x0C;  // 0x0C = (reverse 0x03)>>(8-4)
      else
        crc = (crc >> 1);
    }
  }
  return crc;
}
````
## RDT原理概述

## CRC循环检验和

## Sender

## Receiver

## 其他
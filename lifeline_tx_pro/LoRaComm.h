#ifndef LORA_COMM_H
#define LORA_COMM_H

#include "Config.h"
#include "SharedProtocol.h"
#include <SPI.h>
#include <LoRa.h>

extern bool lastAckReceived;
extern String lastAckStatus;
extern String lastAckBaseId;
extern String lastAckMessage;
extern int lastAckRssi;
extern int lastAckSnr;

void initLoRa();
bool transmitAlert();
bool transmitAlertWithAck(int alertIdx = -1);
bool transmitChatMessage(const String& message);
bool transmitSPUTelemetry(const TelemetryPacket& pkt);

#endif // LORA_COMM_H

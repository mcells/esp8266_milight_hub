#include <CasaluxPacketFormatter.h>
#include <MiLightCommands.h>

static const uint8_t CASALUX_PROTOCOL_ID = 0xDD; //0xDD or 0x11

bool CasaluxPacketFormatter::canHandle(const uint8_t *packet, const size_t len) {
  return len == packetLength && packet[2] == CASALUX_PROTOCOL_ID;
}

void CasaluxPacketFormatter::initializePacket(uint8_t* packet) {
  size_t packetPtr = 0;

  // Byte 0: Packet length = 10 bytes

  // Byte 1: Bulb command, filled in later
  packet[packetPtr++] = 0;

  // Byte 2: 00
  packet[packetPtr++] = 0x00;

  // Byte 3: Protocol id
  packet[packetPtr++] = CASALUX_PROTOCOL_ID;

  // Byte 4 and 5: Device ID
  packet[packetPtr++] = deviceId >> 8;
  packet[packetPtr++] = deviceId & 0xFF;

  // Byte 6: Zone
  packet[packetPtr++] = groupToGroupId(groupId);

  // Byte 7: Zero
  packet[packetPtr++] = 0;

  // Byte 8: Packet sequence number 0..255
  packet[packetPtr++] = 2;//++sequenceNum;

  // Byte 9: Checksum over previous bytes
  // The checksum will be calculated when setting the command field      1  2  3  4  5  6  7  8  9 10 11
  packet[packetPtr++] = 0; //Captured example Packet (Group 1 ON): (09) 6F 00 DD 4E 51 D1 00 02 BC 20 F5 (00)
  //packet[packetPtr++] = 0; //Checksum byte 9 is lastByte(sum of bytes 1-7)

}

void CasaluxPacketFormatter::finalizePacket(uint8_t* packet) {
  uint16_t checksum;

  // Calculate checksum over packet length .. sequenceNum
  checksum = 0;
  for (uint8_t i = 0; i < 7; i++) {
    checksum += currentPacket[i];
  }
  // Store the checksum in the 9th byte
  currentPacket[8] = checksum & 0x00FF;
}

void CasaluxPacketFormatter::updateBrightness(uint8_t value) {
  const GroupState* state = this->stateStore->get(deviceId, groupId, MiLightRemoteType::REMOTE_TYPE_CASALUX);
  int8_t knownValue = (state != NULL && state->isSetBrightness()) ? state->getBrightness() / CASALUX_BRIGHTNESS_INTERVALS : -1;

  valueByStepFunction(
    &PacketFormatter::increaseBrightness,
    &PacketFormatter::decreaseBrightness,
    CASALUX_BRIGHTNESS_INTERVALS,
    value / CASALUX_BRIGHTNESS_INTERVALS,
    knownValue
  );
}

void CasaluxPacketFormatter::updateTemperature(uint8_t value) {
  const GroupState* state = this->stateStore->get(deviceId, groupId, MiLightRemoteType::REMOTE_TYPE_CASALUX);
  int8_t knownValue = (state != NULL && state->isSetKelvin()) ? state->getKelvin() / CASALUX_TEMPERATURE_INTERVALS : -1;

  valueByStepFunction(
    &PacketFormatter::increaseTemperature,
    &PacketFormatter::decreaseTemperature,
    CASALUX_TEMPERATURE_INTERVALS,
    value / CASALUX_TEMPERATURE_INTERVALS,
    knownValue
  );
}

void CasaluxPacketFormatter::command(uint8_t command, uint8_t arg) {
  pushPacket();
  currentPacket[CASALUX_COMMAND_INDEX] = command;
}

void CasaluxPacketFormatter::updateStatus(MiLightStatus status, uint8_t groupId) {
  if(status == ON) command(CASALUX_ON,0);
  else command(CASALUX_OFF,0);
}

void CasaluxPacketFormatter::increaseTemperature() {
  command(CASALUX_TEMPERATURE_UP, 0);
}

void CasaluxPacketFormatter::decreaseTemperature() {
  command(CASALUX_TEMPERATURE_DOWN, 0);
}

void CasaluxPacketFormatter::increaseBrightness() {
  command(CASALUX_BRIGHTNESS_UP, 0);
}

void CasaluxPacketFormatter::decreaseBrightness() {
  command(CASALUX_BRIGHTNESS_DOWN, 0);
}

uint8_t CasaluxPacketFormatter::groupToGroupId(uint8_t group){
  switch(group) {
    case 1:
      return CASALUX_GROUP_1;
    case 2:
      return CASALUX_GROUP_2;
    case 3:
      return CASALUX_GROUP_3;
    case 4:
      return CASALUX_GROUP_4;
    default:
      return CASALUX_GROUP_ALL;
  }
}

uint8_t CasaluxPacketFormatter::groupIdToGroup(uint8_t groupId){
  switch(groupId) {
    case CASALUX_GROUP_1:
      return 1;
    case CASALUX_GROUP_2:
      return 2;
    case CASALUX_GROUP_3:
      return 3;
    case CASALUX_GROUP_4:
      return 4;
    default:
      return 255;
  }
}

BulbId CasaluxPacketFormatter::parsePacket(const uint8_t* packet, JsonObject result) {
  uint8_t command = packet[CASALUX_COMMAND_INDEX];

  uint8_t onOffGroup = groupIdToGroup(packet[CASALUX_GROUP_INDEX]);

  BulbId bulbId(
    (packet[3] << 8) | packet[4],
    onOffGroup,
    REMOTE_TYPE_CASALUX
  );

  sequenceNum = packet[7];
  sequenceNum++;
  
  if(onOffGroup < 255) {
    if (command == CASALUX_ON) {
      result[GroupStateFieldNames::STATE] = "ON";
    } else if (command == CASALUX_OFF) {
      result[GroupStateFieldNames::STATE] = "OFF";
    }
  }
  
  if (command == CASALUX_BRIGHTNESS_UP) {
    result[GroupStateFieldNames::COMMAND] = MiLightCommandNames::LEVEL_UP;
  } else if (command == CASALUX_BRIGHTNESS_DOWN) {
    result[GroupStateFieldNames::COMMAND] = MiLightCommandNames::LEVEL_DOWN;
  } else if (command == CASALUX_TEMPERATURE_DOWN) {
    result[GroupStateFieldNames::COMMAND] = MiLightCommandNames::TEMPERATURE_DOWN;
  } else if (command == CASALUX_TEMPERATURE_UP) {
    result[GroupStateFieldNames::COMMAND] = MiLightCommandNames::TEMPERATURE_UP;
  } else {
    result["button_id"] = command;
  }

  return bulbId;
}

void CasaluxPacketFormatter::format(uint8_t const* packet, char* buffer) {
  buffer += sprintf_P(buffer, PSTR("Request type  : %02X\n"), packet[CASALUX_COMMAND_INDEX]) ;
  //buffer += sprintf_P(buffer, PSTR("Byte 3 (DD)   : %02X\n"), packet[2]);
  buffer += sprintf_P(buffer, PSTR("Device ID     : %02X%02X\n"), packet[3], packet[4]);
  buffer += sprintf_P(buffer, PSTR("Group         : %02X\n"), packet[CASALUX_GROUP_INDEX]);
  //buffer += sprintf_P(buffer, PSTR("Byte 5(00)    : %02X\n"), packet[6]);
  buffer += sprintf_P(buffer, PSTR("Sequence Num. : %02X\n"), packet[7]);
  buffer += sprintf_P(buffer, PSTR("Payload Chksum: %02X%02X\n"), packet[8]);
}

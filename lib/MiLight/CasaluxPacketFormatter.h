#include <PacketFormatter.h>

#ifndef _CASALUX_PACKET_FORMATTER_H
#define _CASALUX_PACKET_FORMATTER_H

#define CASALUX_COMMAND_INDEX 0
#define CASALUX_GROUP_INDEX 5
#define CASALUX_BRIGHTNESS_INTERVALS 12
#define CASALUX_TEMPERATURE_INTERVALS 13

enum MiLightCasaluxButton {
  CASALUX_ON                = 0x6F,
  CASALUX_OFF               = 0x60,

  CASALUX_GROUP_1           = 0xD1,
  CASALUX_GROUP_2           = 0xD2,
  CASALUX_GROUP_3           = 0xD4,
  CASALUX_GROUP_4           = 0xD8,
  CASALUX_GROUP_ALL         = 0xDF,

  CASALUX_BRIGHTNESS_DOWN   = 0x65,
  CASALUX_BRIGHTNESS_UP     = 0x66,

  CASALUX_TEMPERATURE_DOWN  = 0x69,
  CASALUX_TEMPERATURE_UP    = 0x6A
};

class CasaluxPacketFormatter : public PacketFormatter {
public:
  CasaluxPacketFormatter()
    : PacketFormatter(REMOTE_TYPE_CASALUX, 9, 20)
  { }

  virtual bool canHandle(const uint8_t* packet, const size_t len);

  virtual void updateStatus(MiLightStatus status, uint8_t groupId);
  virtual void command(uint8_t command, uint8_t arg);

  virtual void updateTemperature(uint8_t value);
  virtual void increaseTemperature();
  virtual void decreaseTemperature();

  virtual void updateBrightness(uint8_t value);
  virtual void increaseBrightness();
  virtual void decreaseBrightness();

  virtual void format(uint8_t const* packet, char* buffer);
  virtual void initializePacket(uint8_t* packet);
  virtual void finalizePacket(uint8_t* packet);
  virtual BulbId parsePacket(const uint8_t* packet, JsonObject result);

  static uint8_t groupToGroupId(uint8_t group);
  static uint8_t groupIdToGroup(uint8_t groupId);
};

#endif

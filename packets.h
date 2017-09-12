#define LEFT 0
#define RIGHT 1
#define POLLRATE 0.1 // 100ms

typedef struct ControlPacket {
  int8_t pwm[2];
} ControlPacket;

typedef struct StatusPacket {
  uint8_t intcounts[2];
  double speed[2]; // cm/sec
} StatusPacket;

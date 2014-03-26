class OBC1 : public Memory {
public:
  void init() NALL_COLD;
  void enable();
  void power() NALL_COLD;
  void reset() NALL_COLD;

  uint8 read(unsigned addr);
  void write(unsigned addr, uint8 data);

  void serialize(serializer&) NALL_COLD;
  OBC1() NALL_COLD;
  ~OBC1() NALL_COLD;

private:
  uint8 ram_read(unsigned addr);
  void ram_write(unsigned addr, uint8 data);

  struct {
    uint16 address;
    uint16 baseptr;
    uint16 shift;
  } status;
};

extern OBC1 obc1;

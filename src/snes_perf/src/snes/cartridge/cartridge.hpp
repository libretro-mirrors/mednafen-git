class Cartridge : property<Cartridge> {
public:
  struct Mode{ enum e{
    Normal,
    BsxSlotted,
    Bsx,
    SufamiTurbo,
    SuperGameBoy,
  } i; };

  struct Region{ enum e{
    NTSC,
    PAL,
  } i; };

  struct SuperGameBoyVersion{ enum e{
    Version1,
    Version2,
  } i; };

  //assigned externally to point to file-system datafiles (msu1 and serial)
  //example: "/path/to/filename.sfc" would set this to "/path/to/filename"
  readwrite<string> basename;

  readonly<bool> loaded;
  readonly<unsigned> crc32;
  readonly<string> sha256;

  Mode mode;
  Region region;
  readonly<unsigned> ram_size;
  readonly<unsigned> spc7110_data_rom_offset;
  SuperGameBoyVersion supergameboy_version;
  readonly<unsigned> supergameboy_ram_size;
  readonly<unsigned> supergameboy_rtc_size;

  readonly<bool> has_bsx_slot;
  readonly<bool> has_superfx;
  readonly<bool> has_sa1;
  readonly<bool> has_upd77c25;
  readonly<bool> has_srtc;
  readonly<bool> has_sdd1;
  readonly<bool> has_spc7110;
  readonly<bool> has_spc7110rtc;
  readonly<bool> has_cx4;
  readonly<bool> has_obc1;
  readonly<bool> has_st0010;
  readonly<bool> has_st0011;
  readonly<bool> has_st0018;
  readonly<bool> has_msu1;
  readonly<bool> has_serial;

  struct Mapping {
    Memory *memory;
    MMIO *mmio;
    Bus::MapMode mode;
    unsigned banklo;
    unsigned bankhi;
    unsigned addrlo;
    unsigned addrhi;
    unsigned offset;
    unsigned size;

    Mapping();
    Mapping(Memory&);
    Mapping(MMIO&);
  };
  array<Mapping> mapping;

  void load(Mode::e, const lstring&) NALL_COLD;
  void unload() NALL_COLD;

  void serialize(serializer&) NALL_COLD;
  Cartridge();
  ~Cartridge();

private:
  void parse_xml(const lstring&) NALL_COLD;
  void parse_xml_cartridge(const char*) NALL_COLD;
  void parse_xml_bsx(const char*) NALL_COLD;
  void parse_xml_sufami_turbo(const char*, bool) NALL_COLD;
  void parse_xml_gameboy(const char*) NALL_COLD;

  void xml_parse_rom(xml_element&) NALL_COLD;
  void xml_parse_ram(xml_element&) NALL_COLD;
  void xml_parse_superfx(xml_element&) NALL_COLD;
  void xml_parse_sa1(xml_element&) NALL_COLD;
  void xml_parse_upd77c25(xml_element&) NALL_COLD;
  void xml_parse_bsx(xml_element&) NALL_COLD;
  void xml_parse_sufamiturbo(xml_element&) NALL_COLD;
  void xml_parse_supergameboy(xml_element&) NALL_COLD;
  void xml_parse_srtc(xml_element&) NALL_COLD;
  void xml_parse_sdd1(xml_element&) NALL_COLD;
  void xml_parse_spc7110(xml_element&) NALL_COLD;
  void xml_parse_cx4(xml_element&) NALL_COLD;
  void xml_parse_necdsp(xml_element&) NALL_COLD;
  void xml_parse_obc1(xml_element&) NALL_COLD;
  void xml_parse_setadsp(xml_element&) NALL_COLD;
  void xml_parse_setarisc(xml_element&) NALL_COLD;
  void xml_parse_msu1(xml_element&) NALL_COLD;
  void xml_parse_serial(xml_element&) NALL_COLD;

  void xml_parse_address(Mapping&, const string&) NALL_COLD;
  void xml_parse_mode(Mapping&, const string&) NALL_COLD;
};

namespace memory {
  extern MappedRAM cartrom, cartram, cartrtc;
  extern MappedRAM bsxflash, bsxram, bsxpram;
  extern MappedRAM stArom, stAram;
  extern MappedRAM stBrom, stBram;
  extern MappedRAM gbrom, gbram, gbrtc;
};

extern Cartridge cartridge;

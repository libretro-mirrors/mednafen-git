#ifndef LIBSNES_HPP
#define LIBSNES_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SNES_PORT_1  0
#define SNES_PORT_2  1

#define SNES_REGION_NTSC 0
#define SNES_REGION_PAL 1

#define SNES_DEVICE_NONE         0
#define SNES_DEVICE_JOYPAD       1
#define SNES_DEVICE_MULTITAP     2
#define SNES_DEVICE_MOUSE        3
#define SNES_DEVICE_SUPER_SCOPE  4
#define SNES_DEVICE_JUSTIFIER    5
#define SNES_DEVICE_JUSTIFIERS   6

#define SNES_DEVICE_ID_JOYPAD_B        0
#define SNES_DEVICE_ID_JOYPAD_Y        1
#define SNES_DEVICE_ID_JOYPAD_SELECT   2
#define SNES_DEVICE_ID_JOYPAD_START    3
#define SNES_DEVICE_ID_JOYPAD_UP       4
#define SNES_DEVICE_ID_JOYPAD_DOWN     5
#define SNES_DEVICE_ID_JOYPAD_LEFT     6
#define SNES_DEVICE_ID_JOYPAD_RIGHT    7
#define SNES_DEVICE_ID_JOYPAD_A        8
#define SNES_DEVICE_ID_JOYPAD_X        9
#define SNES_DEVICE_ID_JOYPAD_L       10
#define SNES_DEVICE_ID_JOYPAD_R       11

#define SNES_DEVICE_ID_MOUSE_X      0
#define SNES_DEVICE_ID_MOUSE_Y      1
#define SNES_DEVICE_ID_MOUSE_LEFT   2
#define SNES_DEVICE_ID_MOUSE_RIGHT  3

#define SNES_DEVICE_ID_SUPER_SCOPE_X        0
#define SNES_DEVICE_ID_SUPER_SCOPE_Y        1
#define SNES_DEVICE_ID_SUPER_SCOPE_TRIGGER  2
#define SNES_DEVICE_ID_SUPER_SCOPE_CURSOR   3
#define SNES_DEVICE_ID_SUPER_SCOPE_TURBO    4
#define SNES_DEVICE_ID_SUPER_SCOPE_PAUSE    5

#define SNES_DEVICE_ID_JUSTIFIER_X        0
#define SNES_DEVICE_ID_JUSTIFIER_Y        1
#define SNES_DEVICE_ID_JUSTIFIER_TRIGGER  2
#define SNES_DEVICE_ID_JUSTIFIER_START    3

#define SNES_MEMORY_CARTRIDGE_RAM       0
#define SNES_MEMORY_CARTRIDGE_RTC       1
#define SNES_MEMORY_BSX_RAM             2
#define SNES_MEMORY_BSX_PRAM            3
#define SNES_MEMORY_SUFAMI_TURBO_A_RAM  4
#define SNES_MEMORY_SUFAMI_TURBO_B_RAM  5
#define SNES_MEMORY_GAME_BOY_RAM        6
#define SNES_MEMORY_GAME_BOY_RTC        7

// Note: Only one instance of the libsnes library is supported in one process.
// Compiling with C, you might have to #include <stdbool.h> since bool is not a recognized keyword in C(99) without this header.


// Typedefs used for callbacks.
typedef void (*snes_video_refresh_t)(const uint16_t *data, unsigned widths[], unsigned height, bool interlaced, bool field);
typedef void (*snes_audio_sample_t)(uint16_t left, uint16_t right);
typedef void (*snes_input_poll_t)(void);
typedef int16_t (*snes_input_state_t)(bool port, unsigned device, unsigned index, unsigned id);

/////////////////////////////////////
// How to implement callbacks:
/////////////////////////////////////

/*    
snes_audio_sample_t: 

   Arguments are seemingly unsigned 16-bit samples, but they are actually signed 16-bit samples.
   This function is called once for every audio frame (one sample from left and right channels). Under ideal conditions, this audio sample rate is 32040Hz (?).
   Arguments left and right are audio data for left and right speakers respectively (just mentioned for completeness. :P)

   It might be possible that you will need to treat this sample rate as slightly lower when working with e.g. vsync. 
   Values from 31900 - 31950 Hz are good bets. 
   You should however allow users of your frontend to alter this value. 
   E.g. if screen refresh rate is 98% of the ideal (~60.2Hz (?)), 
   input samplerate should be slightly lower than 0.98 * 32040 to avoid buffer underruns in the long run. 

   To work with the audio (e.g. resample), you will need to reinterpret the sample value.
   int16_t real_left = *(int16_t*)(&left);
*/


/* 
snes_video_refresh_t: 

   Pixel format is 15-bit RGB: 0RRRRRGGGGGBBBBB. (XBGR1555)
   The pitch of the input frame is generally 1024, with the exception of 512 should the height be equal to 448 or 478.
   The pixel data starts for top-left of the frame.

   Example code:
   void process_frame(const uint16_t* data, int width, int height)
   {
      uint16_t frame[width * height];
      const uint16_t *src;
      uint16_t *dst;

      for (int i = 0; i < height; i++ )
      {
         src = data + i * 1024;  // Not very intuitive, is it? :\
         dst = frame + i * width;

         memcpy(dst, src, width * sizeof(uint16_t));
      }
      do_cool_stuff_with_frame(frame, width, height);
      output_frame(frame, width, height);
   }
*/

/*
snes_input_poll_t:

   Will generally be called before snes_input_state_t. 
   Asks the input driver to update its input states so they can be read from the input state callback.
   This will be called every frame.
*/

/* 
snes_input_state_t:

   Sets the callback that reports the current state of input devices.

   The callback should accept the following parameters:

      "port" is one of the constants PORT_1 or PORT_2, describing which
      controller port is being reported.

      "device" is one of the DEVICE_* constants describing which type of
      device is currently connected to the given port.

      "index" is a number describing which of the devices connected to the
      port is being reported. It's only useful for DEVICE_MULTITAP and
      DEVICE_JUSTIFIERS - for other device types, it's always 0.

      "id" is one of the DEVICE_ID_* constants for the given device,
      describing which button or axis is being reported (for DEVICE_MULTITAP,
      use the DEVICE_ID_JOYPAD_* IDs; for DEVICE_JUSTIFIERS use the
      DEVICE_ID_JUSTIFIER_* IDs.).

   If "id" represents an analogue input (such as DEVICE_ID_MOUSE_X and
   DEVICE_ID_MOUSE_Y), you should return a value between -32768 and 32767. If
   it represents a digital input such as DEVICE_ID_MOUSE_LEFT or
   DEVICE_ID_MOUSE_RIGHT), return 1 if the button is pressed, and 0 otherwise.

   You are responsible for implementing any turbo-fire features, etc.
*/


// Returns library revisions
unsigned snes_library_revision_major(void);
unsigned snes_library_revision_minor(void);

// Sets callbacks that will be called from snes_run()
// All callbacks must be set.
void snes_set_video_refresh(snes_video_refresh_t);
void snes_set_audio_sample(snes_audio_sample_t);
void snes_set_input_poll(snes_input_poll_t);
void snes_set_input_state(snes_input_state_t);

// Connects given device to designated controller port.
/*
   Connecting a device to a port implicitly removes any device previously
   connected to that port. To remove a device without connecting a new one,
   pass DEVICE_NONE as the device parameter. From this point onward, the
   callback passed to set_input_state_cb() will be called with the appropriate
   device, index and id parameters.

   If this function is never called, the default is to have a DEVICE_JOYPAD
   connected to both ports.

   "port" must be either the PORT_1 or PORT_2 constants, describing which port
   the given controller will be connected to. If "port" is set to "PORT_1",
   the "device" parameter must not be any of the devices listed in
   PORT_2_ONLY_DEVICES.

   "device" must be one of the DEVICE_* (but not DEVICE_ID_*) constants,
   describing what kind of device will be connected to the given port. If
   "port" is PORT_1, "device" must not be one of the devices in
   PORT_2_ONLY_DEVICES. The devices are:

      - DEVICE_NONE: No device is connected to this port.
      - DEVICE_JOYPAD: A standard SNES gamepad.
      - DEVICE_MULTITAP: A multitap controller, which acts like
        4 DEVICE_JOYPADs. Your input state callback will be passed "id"
        parameters between 0 and 3.
      - DEVICE_MOUSE: A SNES mouse controller, as shipped with Mario Paint.
      - DEVICE_SUPER_SCOPE: A Nintendo Super Scope light-gun device.
      - DEVICE_JUSTIFIER: A Konami Justifier light-gun device.
      - DEVICE_JUSTIFIERS: Two Konami Justifier light-gun devices,
        daisy-chained together. Your input state callback will be passed "id"
        parameters 0 and 1.

   TODO: Is there any time it's not safe to call this method? For example, is
   it safe to call this method from inside the input state callback?
*/
void snes_set_controller_port_device(bool port, unsigned device);
void snes_set_cartridge_basename(const char *basename);

// Initializes library
void snes_init(void);
// Terminates library
void snes_term(void);
// Turns the emulated console off and on. This requires that a cartridge is loaded.
// TODO: Can snes_term() be called right before snes_init() after a cartridge is loaded to effectively "reset" the library?
void snes_power(void);
// Resets emulation. Emulates pressing the reset button on a SNES. This requires that a cartridge is loaded.
void snes_reset(void);
// Emulates a single frame once everything else is loaded. 
// Will usually call each callback at least once before returning. 
// This function will run as fast as possible.
// It is up to the programmer to make sure that the game runs as fast as intended.
// For optimal A/V sync, make sure that the audio callback never blocks for longer than a frame (approx 16ms.)
// Optimally, it should never block for more than a few ms at a time.
void snes_run(bool frameskip);

//////////////////////////////////////
// Save state support. Data acquired from these are not portable across library versions.
//
// If snes_serialize_size() does not match the size of old savestates, 
// it usually implies you have savestates from a different version of the library.

// Returns size of internal objects that needs to be serialized/unserialized.
unsigned snes_serialize_size(void);
// Serializes internal objects into raw data. Equivalent to a save state. 
// Can later be restored with snes_unserialize().
// Will return boolean true on success.
bool snes_serialize(uint8_t *data, unsigned size);
// Unserializes data. Effectively loads state.
// Will return boolean true on success.
bool snes_unserialize(const uint8_t *data, unsigned size);
//////////////////////////////////////

/////////////////////////////////
// Cheat support TODO: Document more.
void snes_cheat_reset(void);
// Cheats need to contain a valid Game Genie cheat code, or a sequence of them separated with plus signed:
// E.g.: "DD62-3B1F+DD12-FA2C"
void snes_cheat_set(unsigned index, bool enabled, const char *code);
/////////////////////////////////

/////////////////////////////////
// Load cartridges

// Loads normal roms into libsnes.
//
// rom_xml: XML string containing mapping information for the rom. 
// Is not needed for practically any licensed game released. Can be set to NULL.
// rom_data: Data for the ROM.
// rom_size: Size of rom.
bool snes_load_cartridge_normal(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size
);

// Load a BS-X slotted cartridge into the emulated SNES.
//   A "BS-X slotted cartridge" is an ordinary SNES cartridge with a slot in the
//   top that accepts the same memory packs that the BS-X cartridge does.

// rom_data: Must be data containing the uncompressed, de-interleaved, headerless ROM image of the BS-X slotted cartridge.
// *_xml: Mapping information. Can be set to NULL.
bool snes_load_cartridge_bsx_slotted(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size,
  const char *bsx_xml, const uint8_t *bsx_data, unsigned bsx_size
);

bool snes_load_cartridge_bsx(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size,
  const char *bsx_xml, const uint8_t *bsx_data, unsigned bsx_size
);

bool snes_load_cartridge_sufami_turbo(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size,
  const char *sta_xml, const uint8_t *sta_data, unsigned sta_size,
  const char *stb_xml, const uint8_t *stb_data, unsigned stb_size
);

// rom_*: Data for the Super Gameboy rom itself
// dmg_*: Data for the Gameboy rom.
bool snes_load_cartridge_super_game_boy(
  const char *rom_xml, const uint8_t *rom_data, unsigned rom_size,
  const char *dmg_xml, const uint8_t *dmg_data, unsigned dmg_size
);
/////////////////////////////////

// Unloads current cartridge from the system.
void snes_unload_cartridge(void);

// Gets region of ROM. true == PAL (50 fps), false == NTSC/PAL60 (60 fps)
bool snes_get_region(void);

//////////////////////////////////////////////
// Save files
/////////////////
// To work with non-volatile memory (save files), acquire the pointer with snes_get_memory_data(), 
// and read snes_get_memory_size() bytes. 
//
// This memory is not dynamically allocated and must not be free()-d. 
// To load non-volatile memory into libsnes (loading a save file), 
// acquire pointer and size and memcpy() the save file to this location. 
// snes_get_memory_size() must match the size of your save file. 
//
// Using the SNES_MEMORY_CARTRIDGE_* id-s will be sufficient for most games. 
// Standard format for SNES_MEMORY_CARTRIDGE_RAM is .srm and SNES_MEMORY_CARTRIDGE_RTC is .rtc.
// If you use different cartridges, you will have to use the appropriate id-s for those.
uint8_t* snes_get_memory_data(unsigned id);
unsigned snes_get_memory_size(unsigned id);
//////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif

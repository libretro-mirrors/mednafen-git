#ifndef __MDFN_DRIVERS_INPUT_CONFIG_H
#define __MDFN_DRIVERS_INPUT_CONFIG_H

bool DTestButton(const std::vector<ButtConfig>& bc, const uint8* KeyState, const uint32* MouseData, bool AND_Mode);
int32 DTestAnalogButton(const std::vector<ButtConfig>& bc, const uint8* KeyState, const uint32* MouseData);

int DTestButtonCombo(std::vector<ButtConfig> &bc, const uint8* KeyState, const uint32 *MouseData, bool AND_Mode = false);
int DTestButtonCombo(ButtConfig &bc, const uint8* KeyState, const uint32 *MouseData, bool AND_Mode = false);

int32 DTestMouseAxis(ButtConfig &bc, const uint8* KeyState, const uint32* MouseData, const bool axis_hint);

int DTryButtonBegin(ButtConfig *bc, int commandkey);
int DTryButton(void);
int DTryButtonEnd(ButtConfig *bc);

#endif

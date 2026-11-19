#ifndef JOYPAD_SWITCH_H
#define JOYPAD_SWITCH_H

#include "main/input_default.h"
#include "switch_wrapper.h"

#define JOYPADS_MAX 4

class JoypadSwitch {
	InputDefault *input;
	PadState pads[JOYPADS_MAX];
	bool connected_pads[JOYPADS_MAX] = {};
	bool scroll_down[JOYPADS_MAX] = {};
	int connected_count = 0;
	int logical_device(int p_index) const;

public:
	JoypadSwitch(InputDefault *p_input);
	void process();
	int get_controller_count() const { return connected_count; }
	int get_controller_style(int p_index) const;
};

#endif

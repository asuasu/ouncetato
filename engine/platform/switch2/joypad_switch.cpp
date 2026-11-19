#include "joypad_switch.h"
#include "input_latency.h"

#include "core/os/os.h"

struct ButtonMapping {
	u64 source;
	int target;
};

static const u64 pad_ids[JOYPADS_MAX] = {
	(1ULL << HidNpadIdType_No1) | (1ULL << HidNpadIdType_Handheld),
	1ULL << HidNpadIdType_No2,
	1ULL << HidNpadIdType_No3,
	1ULL << HidNpadIdType_No4,
};

static const ButtonMapping standard_buttons[] = {
	{ HidNpadButton_A, 0 },
	{ HidNpadButton_B, 1 },
	{ HidNpadButton_X, 2 },
	{ HidNpadButton_Y, 3 },
	{ HidNpadButton_L, 4 },
	{ HidNpadButton_R, 5 },
	{ HidNpadButton_ZL, 6 },
	{ HidNpadButton_ZR, 7 },
	{ HidNpadButton_StickL, 8 },
	{ HidNpadButton_StickR, 9 },
	{ HidNpadButton_Minus, 10 },
	{ HidNpadButton_Plus, 11 },
	{ HidNpadButton_Up, 12 },
	{ HidNpadButton_Down, 13 },
	{ HidNpadButton_Left, 14 },
	{ HidNpadButton_Right, 15 },
};

static const ButtonMapping left_horizontal_buttons[] = {
	{ HidNpadButton_Down, 0 },
	{ HidNpadButton_Left, 1 },
	{ HidNpadButton_Up, 2 },
	{ HidNpadButton_Right, 3 },
	{ HidNpadButton_Minus, 11 },
};

static const ButtonMapping right_horizontal_buttons[] = {
	{ HidNpadButton_X, 0 },
	{ HidNpadButton_A, 1 },
	{ HidNpadButton_Y, 2 },
	{ HidNpadButton_B, 3 },
	{ HidNpadButton_Plus, 11 },
};

static const ButtonMapping shared_buttons[] = {
	{ HidNpadButton_AnySL, 4 },
	{ HidNpadButton_AnySR, 5 },
	{ HidNpadButton_L, 6 },
	{ HidNpadButton_R, 7 },
};

static int style_for_pad(const PadState &pad) {
	const u32 style = padGetStyleSet(&pad);
	if (style & HidNpadStyleTag_NpadJoyLeft) {
		return 4;
	}
	if (style & HidNpadStyleTag_NpadJoyRight) {
		return 5;
	}
	return 0;
}

static void send_buttons(InputDefault *input, int device, u64 pressed, u64 released, const ButtonMapping *mapping, size_t count) {
	for (size_t i = 0; i < count; i++) {
		if (pressed & mapping[i].source) {
			uint64_t started = InputLatencySwitch::is_enabled() ? OS::get_singleton()->get_ticks_usec() : 0;
			input->joy_button(device, mapping[i].target, true);
			InputLatencySwitch::button(device, mapping[i].target, true, started);
		}
		if (released & mapping[i].source) {
			uint64_t started = InputLatencySwitch::is_enabled() ? OS::get_singleton()->get_ticks_usec() : 0;
			input->joy_button(device, mapping[i].target, false);
			InputLatencySwitch::button(device, mapping[i].target, false, started);
		}
	}
}

JoypadSwitch::JoypadSwitch(InputDefault *p_input) :
		input(p_input) {
	padConfigureInput(JOYPADS_MAX, HidNpadStyleSet_NpadStandard);
	for (int i = 0; i < JOYPADS_MAX; i++) {
		padInitializeWithMask(&pads[i], pad_ids[i]);
	}
}

int JoypadSwitch::logical_device(int p_index) const {
	if (connected_count == 2) {
		const int style = style_for_pad(pads[p_index]);
		if (style == 4) {
			return 0;
		}
		if (style == 5) {
			return 1;
		}
	}
	return p_index;
}

int JoypadSwitch::get_controller_style(int p_index) const {
	if (p_index < 0 || p_index >= JOYPADS_MAX) {
		return 0;
	}
	for (int i = 0; i < JOYPADS_MAX; i++) {
		if (logical_device(i) == p_index) {
			return style_for_pad(pads[i]);
		}
	}
	return 0;
}

void JoypadSwitch::process() {
	connected_count = 0;
	for (int index = 0; index < JOYPADS_MAX; index++) {
		padUpdate(&pads[index]);
		if (padIsConnected(&pads[index])) {
			connected_count++;
		}
	}

	for (int index = 0; index < JOYPADS_MAX; index++) {
		const bool connected = padIsConnected(&pads[index]);
		const int device = logical_device(index);
		const int style = style_for_pad(pads[index]);
		if (connected != connected_pads[index]) {
			connected_pads[index] = connected;
			input->joy_connection_changed(device, connected, "Nintendo Switch Controller", "switch2");
			printf("Controller %d %s, style %d\n", device, connected ? "connected" : "disconnected", style);
		}

		HidAnalogStickState primary = padGetStickPos(&pads[index], 0);
		if (style == 4 || style == 5) {
			const HidAnalogStickState alternate = padGetStickPos(&pads[index], 1);
			const int64_t primary_magnitude = (int64_t)primary.x * primary.x + (int64_t)primary.y * primary.y;
			const int64_t alternate_magnitude = (int64_t)alternate.x * alternate.x + (int64_t)alternate.y * alternate.y;
			if (alternate_magnitude > primary_magnitude) {
				primary = alternate;
			}
		}

		float primary_x = primary.x / 32767.0f;
		float primary_y = -primary.y / 32767.0f;
		if (style == 4) {
			primary_x = -primary.y / 32767.0f;
			primary_y = -primary.x / 32767.0f;
		} else if (style == 5) {
			primary_x = primary.y / 32767.0f;
			primary_y = primary.x / 32767.0f;
		}
		uint64_t primary_started = InputLatencySwitch::is_enabled() ? OS::get_singleton()->get_ticks_usec() : 0;
		input->joy_axis(device, 0, primary_x);
		InputLatencySwitch::axis(device, 0, primary_x, primary_started);
		primary_started = InputLatencySwitch::is_enabled() ? OS::get_singleton()->get_ticks_usec() : 0;
		input->joy_axis(device, 1, primary_y);
		InputLatencySwitch::axis(device, 1, primary_y, primary_started);

		const HidAnalogStickState right = padGetStickPos(&pads[index], 1);
		float right_x = right.x / 32767.0f;
		float right_y = -right.y / 32767.0f;
		if (style == 4 || style == 5) {
			right_x = 0.0f;
			right_y = 0.0f;
		}
		input->joy_axis(device, 2, right_x);
		input->joy_axis(device, 3, right_y);

		const u64 pressed = padGetButtonsDown(&pads[index]);
		const u64 released = padGetButtonsUp(&pads[index]);
		if (style == 4) {
			send_buttons(input, device, pressed, released, left_horizontal_buttons, sizeof(left_horizontal_buttons) / sizeof(left_horizontal_buttons[0]));
		} else if (style == 5) {
			send_buttons(input, device, pressed, released, right_horizontal_buttons, sizeof(right_horizontal_buttons) / sizeof(right_horizontal_buttons[0]));
		} else {
			send_buttons(input, device, pressed, released, standard_buttons, sizeof(standard_buttons) / sizeof(standard_buttons[0]));
		}
		send_buttons(input, device, pressed, released, shared_buttons, sizeof(shared_buttons) / sizeof(shared_buttons[0]));
		const u64 held = padGetButtons(&pads[index]);
		if (style == 4 || style == 5) {
			const u64 scroll_button = style == 4 ? HidNpadButton_L : HidNpadButton_R;
			if (pressed & scroll_button) {
				scroll_down[device] = !scroll_down[device];
			}
			const float direction = scroll_down[device] ? 1.0f : -1.0f;
			input->joy_axis(device, 3, (held & scroll_button) ? direction : 0.0f);
		}
	}
}

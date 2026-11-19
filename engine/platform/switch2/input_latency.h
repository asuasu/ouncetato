#ifndef INPUT_LATENCY_H
#define INPUT_LATENCY_H

#include "core/vector.h"
#include "switch_wrapper.h"
#include <GLES3/gl3.h>

class InputLatencySwitch {
	struct Event {
		bool axis;
		int device;
		int index;
		bool pressed;
		float value;
		uint64_t started;
		uint64_t dispatched;
	};

	static bool enabled;
	static Vector<Event> pending;
	static uint64_t window_start;
	static uint64_t last_present;
	static uint64_t axis_count;
	static uint64_t axis_dispatch_total;
	static uint64_t axis_present_total;
	static uint64_t axis_dispatch_max;
	static uint64_t axis_present_max;
	static uint64_t present_count;
	static uint64_t present_interval_total;
	static uint64_t present_interval_max;
	struct Fence {
		GLsync sync;
		uint64_t submitted;
	};
	static Vector<Fence> fences;
	static uint64_t swap_count;
	static uint64_t swap_total;
	static uint64_t swap_max;
	static uint64_t gpu_count;
	static uint64_t gpu_total;
	static uint64_t gpu_max;
	static uint64_t full_sync_count;
	static uint64_t limited_count;
	static uint64_t sync_work_total;
	static uint64_t sync_work_max;

public:
	static bool is_enabled() { return enabled; }
	static void set_enabled(bool p_enabled);
	static void button(int p_device, int p_index, bool p_pressed, uint64_t p_started);
	static void axis(int p_device, int p_index, float p_value, uint64_t p_started);
	static void sync(bool p_full_sync, uint64_t p_work_usec);
	static void present(uint64_t p_swap_started);
};

#endif

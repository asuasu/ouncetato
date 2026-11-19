#include "input_latency.h"

#include "core/os/os.h"

bool InputLatencySwitch::enabled = false;
Vector<InputLatencySwitch::Event> InputLatencySwitch::pending;
uint64_t InputLatencySwitch::window_start = 0;
uint64_t InputLatencySwitch::last_present = 0;
uint64_t InputLatencySwitch::axis_count = 0;
uint64_t InputLatencySwitch::axis_dispatch_total = 0;
uint64_t InputLatencySwitch::axis_present_total = 0;
uint64_t InputLatencySwitch::axis_dispatch_max = 0;
uint64_t InputLatencySwitch::axis_present_max = 0;
uint64_t InputLatencySwitch::present_count = 0;
uint64_t InputLatencySwitch::present_interval_total = 0;
uint64_t InputLatencySwitch::present_interval_max = 0;
Vector<InputLatencySwitch::Fence> InputLatencySwitch::fences;
uint64_t InputLatencySwitch::swap_count = 0;
uint64_t InputLatencySwitch::swap_total = 0;
uint64_t InputLatencySwitch::swap_max = 0;
uint64_t InputLatencySwitch::gpu_count = 0;
uint64_t InputLatencySwitch::gpu_total = 0;
uint64_t InputLatencySwitch::gpu_max = 0;
uint64_t InputLatencySwitch::full_sync_count = 0;
uint64_t InputLatencySwitch::limited_count = 0;
uint64_t InputLatencySwitch::sync_work_total = 0;
uint64_t InputLatencySwitch::sync_work_max = 0;

void InputLatencySwitch::set_enabled(bool p_enabled) {
	enabled = p_enabled;
	pending.clear();
	window_start = 0;
	last_present = 0;
	axis_count = 0;
	axis_dispatch_total = 0;
	axis_present_total = 0;
	axis_dispatch_max = 0;
	axis_present_max = 0;
	present_count = 0;
	present_interval_total = 0;
	present_interval_max = 0;
	swap_count = 0;
	swap_total = 0;
	swap_max = 0;
	gpu_count = 0;
	gpu_total = 0;
	gpu_max = 0;
	full_sync_count = 0;
	limited_count = 0;
	sync_work_total = 0;
	sync_work_max = 0;
}

void InputLatencySwitch::button(int p_device, int p_index, bool p_pressed, uint64_t p_started) {
	if (!enabled) {
		return;
	}
	Event event;
	event.axis = false;
	event.device = p_device;
	event.index = p_index;
	event.pressed = p_pressed;
	event.value = 0.0f;
	event.started = p_started;
	event.dispatched = OS::get_singleton()->get_ticks_usec();
	pending.push_back(event);
}

void InputLatencySwitch::axis(int p_device, int p_index, float p_value, uint64_t p_started) {
	if (!enabled) {
		return;
	}
	Event event;
	event.axis = true;
	event.device = p_device;
	event.index = p_index;
	event.pressed = false;
	event.value = p_value;
	event.started = p_started;
	event.dispatched = OS::get_singleton()->get_ticks_usec();
	pending.push_back(event);
}

void InputLatencySwitch::sync(bool p_full_sync, uint64_t p_work_usec) {
	if (!enabled) {
		return;
	}
	if (p_full_sync) {
		full_sync_count++;
	} else {
		limited_count++;
	}
	sync_work_total += p_work_usec;
	sync_work_max = MAX(sync_work_max, p_work_usec);
}

void InputLatencySwitch::present(uint64_t p_swap_started) {
	if (!enabled) {
		return;
	}
	uint64_t now = OS::get_singleton()->get_ticks_usec();
	uint64_t swap_duration = now - p_swap_started;
	swap_count++;
	swap_total += swap_duration;
	swap_max = MAX(swap_max, swap_duration);
	if (window_start == 0) {
		window_start = now;
	}
	if (last_present != 0) {
		uint64_t interval = now - last_present;
		present_interval_total += interval;
		present_interval_max = MAX(present_interval_max, interval);
		present_count++;
	}
	last_present = now;

	for (int i = 0; i < pending.size(); i++) {
		const Event &event = pending[i];
		uint64_t dispatch = event.dispatched - event.started;
		uint64_t presented = now - event.started;
		if (event.axis) {
			axis_count++;
			axis_dispatch_total += dispatch;
			axis_present_total += presented;
			axis_dispatch_max = MAX(axis_dispatch_max, dispatch);
			axis_present_max = MAX(axis_present_max, presented);
		} else {
			printf("INPUT_LATENCY_BUTTON device=%d button=%d state=%s dispatch_ms=%.3f present_ms=%.3f\n",
					event.device, event.index, event.pressed ? "pressed" : "released",
					dispatch / 1000.0, presented / 1000.0);
		}
	}
	pending.clear();

	for (int i = 0; i < fences.size();) {
		Fence &fence = fences.write[i];
		GLenum result = glClientWaitSync(fence.sync, 0, 0);
		if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
			uint64_t gpu_duration = now - fence.submitted;
			gpu_count++;
			gpu_total += gpu_duration;
			gpu_max = MAX(gpu_max, gpu_duration);
			glDeleteSync(fence.sync);
			fences.remove(i);
		} else if (result == GL_WAIT_FAILED) {
			printf("INPUT_LATENCY_GPU fence wait failed\n");
			glDeleteSync(fence.sync);
			fences.remove(i);
		} else {
			i++;
		}
	}
	Fence fence;
	fence.sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	fence.submitted = now;
	if (fence.sync) {
		fences.push_back(fence);
	}

	if (now - window_start >= 2000000) {
		if (axis_count > 0) {
			printf("INPUT_LATENCY_AXIS count=%llu dispatch_avg_ms=%.3f dispatch_max_ms=%.3f present_avg_ms=%.3f present_max_ms=%.3f\n",
					(unsigned long long)axis_count,
					axis_dispatch_total / 1000.0 / axis_count, axis_dispatch_max / 1000.0,
					axis_present_total / 1000.0 / axis_count, axis_present_max / 1000.0);
		}
		if (present_count > 0) {
			printf("INPUT_LATENCY_PRESENT frames=%llu interval_avg_ms=%.3f interval_max_ms=%.3f\n",
					(unsigned long long)present_count,
					present_interval_total / 1000.0 / present_count, present_interval_max / 1000.0);
		}
		if (swap_count > 0) {
			printf("INPUT_LATENCY_SWAP frames=%llu avg_ms=%.3f max_ms=%.3f\n",
					(unsigned long long)swap_count, swap_total / 1000.0 / swap_count, swap_max / 1000.0);
		}
		if (gpu_count > 0) {
			printf("INPUT_LATENCY_GPU frames=%llu pending=%d avg_ms=%.3f max_ms=%.3f\n",
					(unsigned long long)gpu_count, fences.size(), gpu_total / 1000.0 / gpu_count, gpu_max / 1000.0);
		}
		uint64_t sync_count = full_sync_count + limited_count;
		if (sync_count > 0) {
			printf("INPUT_LATENCY_ADAPTIVE full_sync=%llu queued=%llu work_avg_ms=%.3f work_max_ms=%.3f\n",
					(unsigned long long)full_sync_count, (unsigned long long)limited_count,
					sync_work_total / 1000.0 / sync_count, sync_work_max / 1000.0);
		}
		window_start = now;
		axis_count = 0;
		axis_dispatch_total = 0;
		axis_present_total = 0;
		axis_dispatch_max = 0;
		axis_present_max = 0;
		present_count = 0;
		present_interval_total = 0;
		present_interval_max = 0;
		swap_count = 0;
		swap_total = 0;
		swap_max = 0;
		gpu_count = 0;
		gpu_total = 0;
		gpu_max = 0;
		full_sync_count = 0;
		limited_count = 0;
		sync_work_total = 0;
		sync_work_max = 0;
	}
}

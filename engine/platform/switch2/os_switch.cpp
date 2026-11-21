#include "os_switch.h"
#include "switch_wrapper.h"

#include "drivers/gles2/rasterizer_gles2.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "input_latency.h"
#include "core/sort_array.h"
#include "drivers/unix/dir_access_unix.h"
#include "drivers/unix/file_access_unix.h"
#include "drivers/unix/thread_posix.h"
#include "main/main.h"
#include "servers/audio_server.h"
#include "servers/visual/visual_server_wrap_mt.h"

#include <stdio.h>
#include <time.h>

void OS_Switch::initialize_core() {
#ifndef NO_THREADS
	init_thread_posix();
#endif
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_RESOURCES);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_FILESYSTEM);
}

Error OS_Switch::initialize(const VideoMode &p_desired, int p_video_driver, int p_audio_driver) {
	current_videomode = p_desired;
	current_videomode.width = 1280;
	current_videomode.height = 720;
	bool gles3_context = p_video_driver != VIDEO_DRIVER_GLES2;
	bool gl_initialization_error = false;

	gl_context = memnew(ContextGLSwitchEGL(gles3_context));
	if (gl_context->initialize() != OK) {
		memdelete(gl_context);
		gl_context = nullptr;
		if (p_video_driver != VIDEO_DRIVER_GLES2 && GLOBAL_GET("rendering/quality/driver/fallback_to_gles2")) {
			p_video_driver = VIDEO_DRIVER_GLES2;
			gles3_context = false;
			gl_context = memnew(ContextGLSwitchEGL(false));
			if (gl_context->initialize() != OK) {
				memdelete(gl_context);
				gl_context = nullptr;
				gl_initialization_error = true;
			}
		} else {
			gl_initialization_error = true;
		}
	}

	if (!gl_initialization_error) {
		if (gles3_context) {
			if (RasterizerGLES3::is_viable() == OK) {
				RasterizerGLES3::register_config();
				RasterizerGLES3::make_current();
			} else {
				gl_initialization_error = true;
			}
		} else {
			if (RasterizerGLES2::is_viable() == OK) {
				RasterizerGLES2::register_config();
				RasterizerGLES2::make_current();
			} else {
				gl_initialization_error = true;
			}
		}
	}
	if (gl_initialization_error) {
		alert("Unable to initialize the OpenGL renderer.", "Godot Switch 2");
		return ERR_UNAVAILABLE;
	}

	current_videomode.width = gl_context->get_window_width();
	current_videomode.height = gl_context->get_window_height();
	video_driver_index = p_video_driver;
	gl_context->set_use_vsync(current_videomode.use_vsync);
	visual_server = memnew(VisualServerRaster);
	if (get_render_thread_mode() != RENDER_THREAD_UNSAFE) {
		visual_server = memnew(VisualServerWrapMT(visual_server, get_render_thread_mode() == RENDER_SEPARATE_THREAD));
	}
	visual_server->init();

	input = memnew(InputDefault);
	input->set_use_accumulated_input(false);
	input->set_use_input_buffering(false);
	input->set_emulate_mouse_from_touch(false);
	joypad = memnew(JoypadSwitch(input));
	AudioDriverManager::initialize(p_audio_driver);
	return OK;
}

void OS_Switch::set_main_loop(MainLoop *p_main_loop) {
	input->set_main_loop(p_main_loop);
	main_loop = p_main_loop;
}

void OS_Switch::delete_main_loop() {
	if (main_loop) {
		memdelete(main_loop);
	}
	main_loop = nullptr;
}

void OS_Switch::finalize() {
	if (joypad) {
		memdelete(joypad);
		joypad = nullptr;
	}
	if (input) {
		memdelete(input);
		input = nullptr;
	}
	if (visual_server) {
		visual_server->finish();
		memdelete(visual_server);
		visual_server = nullptr;
	}
	if (gl_context) {
		memdelete(gl_context);
		gl_context = nullptr;
	}
}

void OS_Switch::finalize_core() {
}

bool OS_Switch::_check_internal_feature_support(const String &p_feature) {
	return p_feature == "mobile" || p_feature == "arm64" || p_feature == "arm64-v8a" || p_feature == "switch2";
}

void OS_Switch::alert(const String &p_alert, const String &p_title) {
	fprintf(stderr, "%s: %s\n", p_title.utf8().ptr(), p_alert.utf8().ptr());
}

String OS_Switch::get_stdin_string() {
	return "";
}

Point2 OS_Switch::get_mouse_position() const {
	return Point2();
}

void OS_Switch::set_mouse_mode(MouseMode p_mode) {
}

OS::MouseMode OS_Switch::get_mouse_mode() const {
	return MOUSE_MODE_HIDDEN;
}

int OS_Switch::get_mouse_button_state() const {
	return 0;
}

void OS_Switch::set_window_title(const String &p_title) {
}

void OS_Switch::set_video_mode(const VideoMode &p_video_mode, int p_screen) {
	current_videomode = p_video_mode;
	current_videomode.width = 1280;
	current_videomode.height = 720;
}

OS::VideoMode OS_Switch::get_video_mode(int p_screen) const {
	return current_videomode;
}

void OS_Switch::get_fullscreen_mode_list(List<VideoMode> *p_list, int p_screen) const {
	p_list->push_back(current_videomode);
}

int OS_Switch::get_current_video_driver() const {
	return video_driver_index;
}

Size2 OS_Switch::get_window_size() const {
	if (gl_context) {
		return Size2(gl_context->get_window_width(), gl_context->get_window_height());
	}
	return Size2(current_videomode.width, current_videomode.height);
}

Error OS_Switch::execute(const String &p_path, const List<String> &p_arguments, bool p_blocking, ProcessID *r_child_id, String *r_pipe, int *r_exitcode, bool read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	return ERR_UNAVAILABLE;
}

Error OS_Switch::kill(const ProcessID &p_pid) {
	return ERR_UNAVAILABLE;
}

bool OS_Switch::is_process_running(const ProcessID &p_pid) const {
	return false;
}

String OS_Switch::get_executable_path() const {
	return switch_execpath;
}

void OS_Switch::set_executable_path(const char *p_execpath) {
	switch_execpath = p_execpath;
}

String OS_Switch::get_data_path() const {
	return "sdmc:/switch";
}

String OS_Switch::get_config_path() const {
	return get_user_data_dir().plus_file("config");
}

String OS_Switch::get_cache_path() const {
	return get_user_data_dir().plus_file("cache");
}

String OS_Switch::get_user_data_dir() const {
	String appname = get_safe_dir_name(ProjectSettings::get_singleton()->get("application/config/name"));
	if (appname.empty()) {
		appname = "unknown";
	}
	return get_data_path().plus_file(appname.to_lower());
}

bool OS_Switch::has_environment(const String &p_var) const {
	return false;
}

String OS_Switch::get_environment(const String &p_var) const {
	return "";
}

bool OS_Switch::set_environment(const String &p_var, const String &p_value) const {
	return false;
}

String OS_Switch::get_name() const {
	return "Seaven Ounce";
}

MainLoop *OS_Switch::get_main_loop() const {
	return main_loop;
}

static struct tm get_tm(bool p_local) {
	time_t value = time(nullptr);
	struct tm result;
	if (p_local) {
		localtime_r(&value, &result);
	} else {
		gmtime_r(&value, &result);
	}
	return result;
}

OS::Date OS_Switch::get_date(bool p_local) const {
	const struct tm value = get_tm(p_local);
	Date result;
	result.year = value.tm_year + 1900;
	result.month = (Month)(value.tm_mon + 1);
	result.day = value.tm_mday;
	result.weekday = (Weekday)value.tm_wday;
	result.dst = value.tm_isdst;
	return result;
}

OS::Time OS_Switch::get_time(bool p_local) const {
	const struct tm value = get_tm(p_local);
	Time result;
	result.hour = value.tm_hour;
	result.min = value.tm_min;
	result.sec = value.tm_sec;
	return result;
}

OS::TimeZoneInfo OS_Switch::get_time_zone_info() const {
	const struct tm value = get_tm(true);
	char name[16];
	char bias_buffer[16];
	strftime(name, sizeof(name), "%Z", &value);
	strftime(bias_buffer, sizeof(bias_buffer), "%z", &value);
	int bias = 0;
	sscanf(bias_buffer, "%d", &bias);
	const int hours = bias / 100;
	const int minutes = bias % 100;
	TimeZoneInfo result;
	result.name = name;
	result.bias = hours * 60 + (bias < 0 ? -minutes : minutes);
	return result;
}

uint64_t OS_Switch::get_unix_time() const {
	return time(nullptr);
}

uint64_t OS_Switch::get_system_time_secs() const {
	return time(nullptr);
}

uint64_t OS_Switch::get_system_time_msecs() const {
	return (uint64_t)time(nullptr) * 1000;
}

double OS_Switch::get_subsecond_unix_time() const {
	return (double)time(nullptr) + (get_ticks_usec() % 1000000) / 1000000.0;
}

void OS_Switch::delay_usec(uint32_t p_usec) const {
	svcSleepThread((int64_t)p_usec * 1000LL);
}

uint64_t OS_Switch::get_ticks_usec() const {
	static const u64 frequency = armGetSystemTickFreq();
	return armGetSystemTick() * 1000000ULL / frequency;
}

void OS_Switch::set_pacing_rate(int p_rate) {
	if (pacing_rate == p_rate) {
		return;
	}
	pacing_rate = p_rate;
	Engine::get_singleton()->set_iterations_per_second(p_rate);
	Engine::get_singleton()->set_target_fps(p_rate);
	uint64_t now = get_ticks_usec();
	last_rate_change = now;
	overload_started = 0;
	severe_overload_started = 0;
	stable_started = 0;
	ignore_pacing_until = now + 500000;
	pacing_window_started = now;
	frame_work_samples.clear();
	printf("Adaptive physics/render rate: %d Hz\n", pacing_rate);
}

void OS_Switch::update_frame_pacing(uint64_t p_now, uint64_t p_work_usec) {
	int engine_rate = Engine::get_singleton()->get_iterations_per_second();
	if (engine_rate != pacing_rate) {
		pacing_rate = engine_rate;
		overload_started = 0;
		severe_overload_started = 0;
		stable_started = 0;
		pacing_window_started = p_now;
		frame_work_samples.clear();
	}
	if (p_work_usec > 100000) {
		ignore_pacing_until = p_now + 2000000;
		pacing_window_started = p_now;
		frame_work_samples.clear();
		overload_started = 0;
		severe_overload_started = 0;
		stable_started = 0;
		return;
	}
	if (p_now < ignore_pacing_until) {
		return;
	}
	if (pacing_window_started == 0) {
		pacing_window_started = p_now;
	}
	frame_work_samples.push_back(p_work_usec);
	if (p_now - pacing_window_started < 1000000 || frame_work_samples.empty()) {
		return;
	}

	Vector<uint32_t> sorted = frame_work_samples;
	SortArray<uint32_t> sort;
	sort.sort(sorted.ptrw(), sorted.size());
	uint64_t median = sorted[sorted.size() / 2];
	uint64_t p90 = sorted[int((sorted.size() - 1) * 0.9)];
	uint64_t budget = 1000000 / pacing_rate;
	int lower_rate = 0;
	int higher_rate = 0;
	switch (pacing_rate) {
		case 60: lower_rate = 50; break;
		case 50: lower_rate = 40; higher_rate = 60; break;
		case 40: lower_rate = 30; higher_rate = 50; break;
		case 30: higher_rate = 40; break;
	}

	bool overloaded = (pacing_rate == 60 && median > 17000) ||
			(pacing_rate == 50 && median > 21000) ||
			(pacing_rate == 40 && median > 26000);
	if (overloaded) {
		if (overload_started == 0) {
			overload_started = p_now;
		}
	} else {
		overload_started = 0;
	}
	if (p90 > 30000 && lower_rate != 0) {
		if (severe_overload_started == 0) {
			severe_overload_started = p_now;
		}
	} else {
		severe_overload_started = 0;
	}
	bool stable = median * 4 < budget * 3 && p90 * 10 < budget * 9;
	if (stable) {
		if (stable_started == 0) {
			stable_started = p_now;
		}
	} else {
		stable_started = 0;
	}

	if (lower_rate != 0 && ((overload_started != 0 && p_now - overload_started >= 1000000) ||
			(severe_overload_started != 0 && p_now - severe_overload_started >= 500000)) &&
			p_now - last_rate_change >= 3000000) {
		set_pacing_rate(lower_rate);
	} else if (higher_rate != 0 && stable_started != 0 && p_now - stable_started >= 3000000 && p_now - last_rate_change >= 3000000) {
		set_pacing_rate(higher_rate);
	}
	frame_work_samples.clear();
	pacing_window_started = p_now;
}

void OS_Switch::update_cpu_boost(uint64_t p_now) {
	float fps = Engine::get_singleton()->get_frames_per_second();
	if (fps > 0 && fps < 40) {
		if (boost_low_fps_started == 0) {
			boost_low_fps_started = p_now;
		}
		boost_high_fps_started = 0;
		if (!boost_enabled && p_now - boost_low_fps_started >= 1000000 && p_now - boost_changed >= 3000000) {
			set_fast_cpu_mode(true);
		}
	} else if (fps > 50) {
		if (boost_high_fps_started == 0) {
			boost_high_fps_started = p_now;
		}
		boost_low_fps_started = 0;
		if (boost_enabled && p_now - boost_high_fps_started >= 5000000 && p_now - boost_changed >= 3000000) {
			set_fast_cpu_mode(false);
		}
	} else {
		boost_low_fps_started = 0;
		boost_high_fps_started = 0;
	}
}

void OS_Switch::set_fast_cpu_mode(bool p_enabled) {
	boost_enabled = p_enabled;
	boost_changed = get_ticks_usec();
	boost_low_fps_started = 0;
	boost_high_fps_started = 0;
	static uint64_t boost_started = 0;
	const Result result = appletSetCpuBoostMode(p_enabled ? ApmCpuBoostMode_FastLoad : ApmCpuBoostMode_Normal);
	printf("CPU boost native enabled=%d result=0x%x\n", p_enabled, result);
	if (R_FAILED(result)) {
		fprintf(stderr, "appletSetCpuBoostMode failed: 0x%x\n", result);
	}
	if (p_enabled) {
		boost_started = get_ticks_usec();
	} else if (boost_started != 0) {
		printf("Boosted scene load: %llu ms\n", (unsigned long long)((get_ticks_usec() - boost_started) / 1000));
		boost_started = 0;
	}
}

void OS_Switch::set_vsync_interval(int p_interval) {
	if (gl_context) {
		gl_context->set_vsync_interval(p_interval);
	}
}

int OS_Switch::get_controller_count() const {
	return joypad ? joypad->get_controller_count() : 0;
}

int OS_Switch::get_max_controller_count() const {
	return max_controller_count;
}

int OS_Switch::get_controller_style(int p_index) const {
	return joypad ? joypad->get_controller_style(p_index) : 0;
}

void OS_Switch::set_min_controller_count(int p_count) {
	min_controller_count = CLAMP(p_count, 1, JOYPADS_MAX);
}

void OS_Switch::set_max_controller_count(int p_count) {
	max_controller_count = CLAMP(p_count, 1, JOYPADS_MAX);
}

bool OS_Switch::show_controller_applet(int p_min, int p_max) {
	return true;
}

bool OS_Switch::can_draw() const {
	return true;
}

int OS_Switch::get_processor_count() const {
	return 4;
}

String OS_Switch::get_processor_name() const {
	return "Cortex-A78C";
}

OS::PowerState OS_Switch::get_power_state() {
	return POWERSTATE_UNKNOWN;
}

int OS_Switch::get_power_seconds_left() {
	return -1;
}

int OS_Switch::get_power_percent_left() {
	return -1;
}

bool OS_Switch::has_touchscreen_ui_hint() const {
	return false;
}

bool OS_Switch::has_virtual_keyboard() const {
	return false;
}

void OS_Switch::run() {
	if (!main_loop) {
		return;
	}
	main_loop->init();
	while (appletMainLoop()) {
		frame_started = OS::get_singleton()->get_ticks_usec();
		joypad->process();
		input->flush_buffered_events();
		if (Main::iteration()) {
			break;
		}
	}
	main_loop->finish();
}

void OS_Switch::swap_buffers() {
	uint64_t started = OS::get_singleton()->get_ticks_usec();
	uint64_t work = started - frame_started;
	update_cpu_boost(started);
	update_frame_pacing(started, work);
	bool full_sync = work <= 10000;
	InputLatencySwitch::sync(full_sync, work);
	if (gl_context) {
		gl_context->swap_buffers(full_sync);
	}
	InputLatencySwitch::present(started);
}

OS_Switch *OS_Switch::get_singleton() {
	return static_cast<OS_Switch *>(OS::get_singleton());
}

OS_Switch::OS_Switch() {
	current_videomode = VideoMode(1280, 720);
	AudioDriverManager::add_driver(&driver_audren);
}

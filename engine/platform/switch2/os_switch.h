#ifndef OS_SWITCH_H
#define OS_SWITCH_H

#include "context_gl_switch_egl.h"
#include "core/os/input.h"
#include "core/os/os.h"
#include "drivers/audren/audio_driver_audren.h"
#include "joypad_switch.h"
#include "main/input_default.h"
#include "servers/visual/visual_server_raster.h"

class OS_Switch : public OS {
	int video_driver_index = 0;
	MainLoop *main_loop = nullptr;
	VideoMode current_videomode;
	VisualServer *visual_server = nullptr;
	InputDefault *input = nullptr;
	ContextGLSwitchEGL *gl_context = nullptr;
	JoypadSwitch *joypad = nullptr;
	uint64_t frame_started = 0;
	AudioDriverAudren driver_audren;
	String switch_execpath;
	int min_controller_count = 1;
	int max_controller_count = 1;

protected:
	void initialize_core() override;
	Error initialize(const VideoMode &p_desired, int p_video_driver, int p_audio_driver) override;
	void set_main_loop(MainLoop *p_main_loop) override;
	void delete_main_loop() override;
	void finalize() override;
	void finalize_core() override;

public:
	bool _check_internal_feature_support(const String &p_feature) override;
	void alert(const String &p_alert, const String &p_title = "ALERT!") override;
	String get_stdin_string() override;
	Point2 get_mouse_position() const override;
	void set_mouse_mode(MouseMode p_mode) override;
	MouseMode get_mouse_mode() const override;
	int get_mouse_button_state() const override;
	void set_window_title(const String &p_title) override;
	void set_video_mode(const VideoMode &p_video_mode, int p_screen = 0) override;
	VideoMode get_video_mode(int p_screen = 0) const override;
	void get_fullscreen_mode_list(List<VideoMode> *p_list, int p_screen = 0) const override;
	int get_current_video_driver() const override;
	Size2 get_window_size() const override;
	Error execute(const String &p_path, const List<String> &p_arguments, bool p_blocking = true, ProcessID *r_child_id = nullptr, String *r_pipe = nullptr, int *r_exitcode = nullptr, bool read_stderr = false, Mutex *p_pipe_mutex = nullptr, bool p_open_console = false) override;
	Error kill(const ProcessID &p_pid) override;
	bool is_process_running(const ProcessID &p_pid) const override;
	String get_executable_path() const override;
	void set_executable_path(const char *p_execpath);
	String get_user_data_dir() const override;
	String get_data_path() const override;
	String get_config_path() const override;
	String get_cache_path() const override;
	bool has_environment(const String &p_var) const override;
	String get_environment(const String &p_var) const override;
	bool set_environment(const String &p_var, const String &p_value) const override;
	String get_name() const override;
	MainLoop *get_main_loop() const override;
	Date get_date(bool p_local = false) const override;
	Time get_time(bool p_local = false) const override;
	TimeZoneInfo get_time_zone_info() const override;
	uint64_t get_unix_time() const override;
	uint64_t get_system_time_secs() const override;
	uint64_t get_system_time_msecs() const override;
	double get_subsecond_unix_time() const override;
	void delay_usec(uint32_t p_usec) const override;
	uint64_t get_ticks_usec() const override;
	void set_fast_cpu_mode(bool p_enabled) override;
	void set_vsync_interval(int p_interval) override;
	int get_controller_count() const override;
	int get_max_controller_count() const override;
	int get_controller_style(int p_index) const override;
	void set_min_controller_count(int p_count) override;
	void set_max_controller_count(int p_count) override;
	bool show_controller_applet(int p_min, int p_max) override;
	bool can_draw() const override;
	int get_processor_count() const override;
	String get_processor_name() const override;
	PowerState get_power_state() override;
	int get_power_seconds_left() override;
	int get_power_percent_left() override;
	bool has_touchscreen_ui_hint() const override;
	bool has_virtual_keyboard() const override;
	void run();
	void swap_buffers() override;
	static OS_Switch *get_singleton();
	OS_Switch();
};

#endif

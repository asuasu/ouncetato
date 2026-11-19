#include "switch_wrapper.h"

#include "core/project_settings.h"
#include "input_latency.h"
#include "main/main.h"
#include "os_switch.h"

#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	printf("Godot Switch 2 starting\n");
	appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad);
	OS_Switch os;
	os.set_executable_path(argv[0]);

	Error err = Main::setup(argv[0], argc - 1, &argv[1]);
	ProjectSettings *settings = ProjectSettings::get_singleton();
	InputLatencySwitch::set_enabled(settings->has_setting("brotato/input_latency_logging") && settings->get_setting("brotato/input_latency_logging"));
	if (settings->has_setting("brotato/file_logging") && settings->get_setting("brotato/file_logging")) {
		String log_path = String(argv[0]).get_base_dir().plus_file(String(argv[0]).get_file().get_basename() + ".log");
		FILE *log = freopen(log_path.utf8().ptr(), "w", stdout);
		if (log) {
			setvbuf(stdout, nullptr, _IOLBF, 0);
			dup2(fileno(stdout), STDERR_FILENO);
		}
	}
	printf("Main::setup finished after %llu ms\n", (unsigned long long)(os.get_ticks_usec() / 1000));
	if (err != OK) {
		appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
		fprintf(stderr, "Main::setup failed: %d\n", err);
		return 255;
	}
	const bool started = Main::start();
	printf("Main::start finished after %llu ms\n", (unsigned long long)(os.get_ticks_usec() / 1000));
	appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
	if (started) {
		os.run();
	}
	Main::cleanup();
	printf("Godot Switch 2 exited\n");
	return os.get_exit_code();
}

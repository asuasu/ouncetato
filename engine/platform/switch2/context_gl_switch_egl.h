#pragma once

#include "core/os/os.h"
#include <EGL/egl.h>

class ContextGLSwitchEGL {
	bool gles3_context = true;
	bool vsync = true;
	EGLDisplay display = EGL_NO_DISPLAY;
	EGLContext context = EGL_NO_CONTEXT;
	EGLSurface surface = EGL_NO_SURFACE;
	int window_width = 1280;
	int window_height = 720;

public:
	void release_current();
	void make_current();
	int get_window_width();
	int get_window_height();
	void swap_buffers();
	void set_use_vsync(bool p_use);
	void set_vsync_interval(int p_interval);
	bool is_using_vsync() const { return vsync; }
	Error initialize();
	void reset();
	void cleanup();
	ContextGLSwitchEGL(bool p_gles3);
	~ContextGLSwitchEGL();
};

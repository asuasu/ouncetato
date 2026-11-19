#pragma once

#include "core/os/os.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>

class ContextGLSwitchEGL {
	bool gles3_context = true;
	bool vsync = true;
	EGLDisplay display = EGL_NO_DISPLAY;
	EGLContext context = EGL_NO_CONTEXT;
	EGLSurface surface = EGL_NO_SURFACE;
	int window_width = 1280;
	int window_height = 720;
	GLsync frame_fences[2] = { nullptr, nullptr };
	int frame_fence_count = 0;
	int frame_fence_next = 0;

public:
	void release_current();
	void make_current();
	int get_window_width();
	int get_window_height();
	void swap_buffers(bool p_full_sync);
	void set_use_vsync(bool p_use);
	void set_vsync_interval(int p_interval);
	bool is_using_vsync() const { return vsync; }
	Error initialize();
	void reset();
	void cleanup();
	ContextGLSwitchEGL(bool p_gles3);
	~ContextGLSwitchEGL();
};

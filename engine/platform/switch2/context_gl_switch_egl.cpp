#include "context_gl_switch_egl.h"
#include "switch_wrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <GLES3/gl3.h>

ContextGLSwitchEGL::ContextGLSwitchEGL(bool p_gles3) :
		gles3_context(p_gles3) {
}

ContextGLSwitchEGL::~ContextGLSwitchEGL() {
	cleanup();
}

Error ContextGLSwitchEGL::initialize() {
	setenv("MESA_VK_WSI_PRESENT_MODE", "fifo", 1);
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		fprintf(stderr, "eglGetDisplay failed: 0x%x\n", eglGetError());
		return ERR_UNAVAILABLE;
	}
	if (!eglInitialize(display, nullptr, nullptr)) {
		fprintf(stderr, "eglInitialize failed: 0x%x\n", eglGetError());
		return ERR_UNAVAILABLE;
	}
	eglBindAPI(EGL_OPENGL_ES_API);

	const EGLint config_attributes[] = {
		EGL_RENDERABLE_TYPE, gles3_context ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE,
	};
	EGLConfig config;
	EGLint config_count = 0;
	if (!eglChooseConfig(display, config_attributes, &config, 1, &config_count) || config_count == 0) {
		fprintf(stderr, "eglChooseConfig failed: 0x%x\n", eglGetError());
		cleanup();
		return ERR_UNAVAILABLE;
	}

	NWindow *window = nwindowGetDefault();
	const Result dimension_result = nwindowSetModeDimensions(window, 1920, 1080, 1920, 1080);
	if (R_FAILED(dimension_result)) {
		fprintf(stderr, "nwindowSetModeDimensions failed: 0x%x\n", dimension_result);
	}
	surface = eglCreateWindowSurface(display, config, window, nullptr);
	if (surface == EGL_NO_SURFACE) {
		fprintf(stderr, "eglCreateWindowSurface failed: 0x%x\n", eglGetError());
		cleanup();
		return ERR_UNAVAILABLE;
	}

	EGLint width = 0;
	EGLint height = 0;
	if (eglQuerySurface(display, surface, EGL_WIDTH, &width)) {
		window_width = width;
	}
	if (eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
		window_height = height;
	}
	u32 native_width = 0;
	u32 native_height = 0;
	nwindowGetDimensions(window, &native_width, &native_height);
	printf("EGL surface: %dx%d; NWindow: %ux%u\n", window_width, window_height, native_width, native_height);

	const EGLint context_attributes[] = {
		EGL_CONTEXT_MAJOR_VERSION, gles3_context ? 3 : 2,
		EGL_NONE,
	};
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
	if (context == EGL_NO_CONTEXT) {
		fprintf(stderr, "eglCreateContext failed: 0x%x\n", eglGetError());
		cleanup();
		return ERR_UNAVAILABLE;
	}
	if (!eglMakeCurrent(display, surface, surface, context)) {
		fprintf(stderr, "eglMakeCurrent failed: 0x%x\n", eglGetError());
		cleanup();
		return ERR_UNAVAILABLE;
	}
	eglSwapInterval(display, vsync ? 1 : 0);
	return OK;
}

void ContextGLSwitchEGL::cleanup() {
	if (display != EGL_NO_DISPLAY) {
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (context != EGL_NO_CONTEXT) {
			eglDestroyContext(display, context);
			context = EGL_NO_CONTEXT;
		}
		if (surface != EGL_NO_SURFACE) {
			eglDestroySurface(display, surface);
			surface = EGL_NO_SURFACE;
		}
		eglTerminate(display);
		display = EGL_NO_DISPLAY;
	}
}

void ContextGLSwitchEGL::reset() {
	cleanup();
	initialize();
}

void ContextGLSwitchEGL::release_current() {
	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void ContextGLSwitchEGL::make_current() {
	eglMakeCurrent(display, surface, surface, context);
}

int ContextGLSwitchEGL::get_window_width() {
	return window_width;
}

int ContextGLSwitchEGL::get_window_height() {
	return window_height;
}

void ContextGLSwitchEGL::swap_buffers() {
	glFinish();
	eglSwapBuffers(display, surface);
}

void ContextGLSwitchEGL::set_use_vsync(bool p_use) {
	vsync = p_use;
	if (display != EGL_NO_DISPLAY) {
		eglSwapInterval(display, vsync ? 1 : 0);
	}
}

void ContextGLSwitchEGL::set_vsync_interval(int p_interval) {
	if (display != EGL_NO_DISPLAY) {
		eglSwapInterval(display, p_interval);
	}
}

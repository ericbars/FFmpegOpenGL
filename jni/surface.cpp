#include "com_player_ffmpeg_VideoSurface.h"
#include <jni.h>

// for native window JNI
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include "player.h"

static ANativeWindow* mANativeWindow;


static jclass globalVideoSurfaceClass = NULL;
static jobject globalVideoSurfaceObject = NULL;

void renderSurface(uint8_t *pixel) {

	if (global_context.quit) {
		glDisable(GL_TEXTURE_2D);
		glDeleteTextures(1, &global_context.mTextureID);
		glDeleteProgram(global_context.glProgram);
		return;
	}

	if (global_context.pause) {
		return;
	}

	Render(pixel);
}

// format not used now.
int32_t setBuffersGeometry(int32_t width, int32_t height) {
	//int32_t format = WINDOW_FORMAT_RGB_565;

	if (NULL == mANativeWindow) {
		LOGV("mANativeWindow is NULL.");
		return -1;
	}

	return ANativeWindow_setBuffersGeometry(mANativeWindow, width, height,
			global_context.eglFormat);
}

int eglOpen() {
	EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY );
	if (eglDisplay == EGL_NO_DISPLAY ) {
		LOGV("eglGetDisplay failure.");
		return -1;
	}
	global_context.eglDisplay = eglDisplay;
	LOGV("eglGetDisplay ok");

	EGLint majorVersion;
	EGLint minorVersion;
	EGLBoolean success = eglInitialize(eglDisplay, &majorVersion,
			&minorVersion);
	if (!success) {
		LOGV("eglInitialize failure.");
		return -1;
	}
	LOGV("eglInitialize ok");

	GLint numConfigs;
	EGLConfig config;
	static const EGLint CONFIG_ATTRIBS[] = { EGL_BUFFER_SIZE, EGL_DONT_CARE,
			EGL_RED_SIZE, 5, EGL_GREEN_SIZE, 6, EGL_BLUE_SIZE, 5,
			EGL_DEPTH_SIZE, 16, EGL_ALPHA_SIZE, EGL_DONT_CARE, EGL_STENCIL_SIZE,
			EGL_DONT_CARE, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE // the end
			};
	success = eglChooseConfig(eglDisplay, CONFIG_ATTRIBS, &config, 1,
			&numConfigs);
	if (!success) {
		LOGV("eglChooseConfig failure.");
		return -1;
	}
	LOGV("eglChooseConfig ok");

	const EGLint attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	EGLContext elgContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT,
			attribs);
	if (elgContext == EGL_NO_CONTEXT ) {
		LOGV("eglCreateContext failure, error is %d", eglGetError());
		return -1;
	}
	global_context.eglContext = elgContext;
	LOGV("eglCreateContext ok");

	EGLint eglFormat;
	success = eglGetConfigAttrib(eglDisplay, config, EGL_NATIVE_VISUAL_ID,
			&eglFormat);
	if (!success) {
		LOGV("eglGetConfigAttrib failure.");
		return -1;
	}
	global_context.eglFormat = eglFormat;
	LOGV("eglGetConfigAttrib ok");

	EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, config,
			mANativeWindow, 0);
	if (NULL == eglSurface) {
		LOGV("eglCreateWindowSurface failure.");
		return -1;
	}
	global_context.eglSurface = eglSurface;
	LOGV("eglCreateWindowSurface ok");
	return 0;
}

int eglClose() {
	EGLBoolean success = eglDestroySurface(global_context.eglDisplay,
			global_context.eglSurface);
	if (!success) {
		LOGV("eglDestroySurface failure.");
	}

	success = eglDestroyContext(global_context.eglDisplay,
			global_context.eglContext);
	if (!success) {
		LOGV("eglDestroySurface failure.");
	}

	success = eglTerminate(global_context.eglDisplay);
	if (!success) {
		LOGV("eglDestroySurface failure.");
	}

	global_context.eglSurface = NULL;
	global_context.eglContext = NULL;
	global_context.eglDisplay = NULL;

	return 0;
}

/*
 * Class:     com_player_ffmpeg_VideoSurface
 * Method:    setSurface
 * Signature: (Landroid/view/Surface;)I
 */JNIEXPORT jint JNICALL Java_com_player_ffmpeg_VideoSurface_setSurface(
		JNIEnv *env, jobject obj, jobject surface) {

	pthread_t thread_1;

	//LOGV("fun env is %p", env);

	jclass localVideoSurfaceClass = env->FindClass(
			"com/player/ffmpeg/VideoSurface");
	if (NULL == localVideoSurfaceClass) {
		LOGV("FindClass VideoSurface failure.");
		return -1;
	}

	globalVideoSurfaceClass = (jclass) env->NewGlobalRef(
			localVideoSurfaceClass);
	if (NULL == globalVideoSurfaceClass) {
		LOGV("localVideoSurfaceClass to globalVideoSurfaceClass failure.");
	}

	globalVideoSurfaceObject = (jclass) env->NewGlobalRef(obj);
	if (NULL == globalVideoSurfaceObject) {
		LOGV("obj to globalVideoSurfaceObject failure.");
	}

	if (NULL == surface) {
		LOGV("surface is null, destroy?");
		mANativeWindow = NULL;
		return 0;
	}

	// obtain a native window from a Java surface
	mANativeWindow = ANativeWindow_fromSurface(env, surface);
	LOGV("mANativeWindow ok");

	if ((global_context.eglSurface != NULL)
			|| (global_context.eglContext != NULL)
			|| (global_context.eglDisplay != NULL)) {
		eglClose();
	}
	eglOpen();

	pthread_create(&thread_1, NULL, open_media, NULL);

	return 0;
}

/*
 * Class:     com_player_ffmpeg_VideoSurface
 * Method:    onPause
 * Signature: ()I
 */JNIEXPORT jint JNICALL Java_com_player_ffmpeg_VideoSurface_nativePausePlayer(
		JNIEnv *, jobject) {
	global_context.pause = 1;
	return 0;
}

/*
 * Class:     com_player_ffmpeg_VideoSurface
 * Method:    onResume
 * Signature: ()I
 */JNIEXPORT jint JNICALL Java_com_player_ffmpeg_VideoSurface_nativeResumePlayer(
		JNIEnv *, jobject) {
	global_context.pause = 0;
	return 0;
}

/*
 * Class:     com_player_ffmpeg_VideoSurface
 * Method:    onDestroy
 * Signature: ()I
 */JNIEXPORT jint JNICALL Java_com_player_ffmpeg_VideoSurface_nativeStopPlayer(
		JNIEnv *, jobject) {
	global_context.quit = 1;
	return 0;
}

#include "SDLWindow.h"
#include "SDLCursor.h"
#include "SDLApplication.h"
#include "../../graphics/opengl/OpenGLBindings.h"
#include <cstring>
#include <algorithm>

#ifdef HX_WINDOWS
#include <Windows.h>
#undef CreateWindow
#endif

namespace lime {

	static Cursor currentCursor = DEFAULT;

	SDL_Cursor* SDLCursor::arrowCursor = nullptr;
	SDL_Cursor* SDLCursor::crosshairCursor = nullptr;
	SDL_Cursor* SDLCursor::moveCursor = nullptr;
	SDL_Cursor* SDLCursor::pointerCursor = nullptr;
	SDL_Cursor* SDLCursor::resizeNESWCursor = nullptr;
	SDL_Cursor* SDLCursor::resizeNSCursor = nullptr;
	SDL_Cursor* SDLCursor::resizeNWSECursor = nullptr;
	SDL_Cursor* SDLCursor::resizeWECursor = nullptr;
	SDL_Cursor* SDLCursor::textCursor = nullptr;
	SDL_Cursor* SDLCursor::waitCursor = nullptr;
	SDL_Cursor* SDLCursor::waitArrowCursor = nullptr;
	SDL_Cursor* SDLCursor::spinnerCursor = nullptr;
	SDL_Cursor* SDLCursor::dragDropCursor = nullptr;
	SDL_Cursor* SDLCursor::dragDropCopyCursor = nullptr;
	SDL_Cursor* SDLCursor::dragDropMoveCursor = nullptr;
	SDL_Cursor* SDLCursor::dragDropNoneCursor = nullptr;
	SDL_Cursor* SDLCursor::resize_dope = nullptr;
	SDL_Cursor* SDLCursor::trans_diag = nullptr;
	SDL_Cursor* SDLCursor::trans_horz = nullptr;
	SDL_Cursor* SDLCursor::trans_vert = nullptr;
	SDL_Cursor* SDLCursor::trans_rotate = nullptr;
	SDL_Cursor* SDLCursor::trans_move = nullptr;
	SDL_Cursor* SDLCursor::trans_diag2 = nullptr;

	static bool displayModeSet = false;
	static bool firstContext = true;
	static bool sdlHintsInitialized = false;

	static SDL_Cursor* CreateCustomCursor(const char* image[]) {
		Uint8 data[4 * 32];
		Uint8 mask[4 * 32];
		int hot_x, hot_y;

		int i = -1;
		for (int row = 0; row < 32; ++row) {
			for (int col = 0; col < 32; ++col) {
				if (col % 8) {
					data[i] <<= 1;
					mask[i] <<= 1;
				} else {
					++i;
					data[i] = mask[i] = 0;
				}
				switch (image[4 + row][col]) {
					case '.':
						data[i] |= 0x01;
						mask[i] |= 0x01;
						break;
					case 'X':
						mask[i] |= 0x01;
						break;
					case ' ':
						break;
				}
			}
		}
		sscanf(image[4 + 32], "%d,%d", &hot_x, &hot_y);
		return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
	}

	template<SDL_SystemCursor systemCursor>
	static inline SDL_Cursor* GetOrCreateSystemCursor(SDL_Cursor*& cache) {
		if (!cache) {
			cache = SDL_CreateSystemCursor(systemCursor);
		}
		return cache;
	}

	SDLWindow::SDLWindow(Application* application, int width, int height, int flags, const char* title) {
		sdlTexture = nullptr;
		sdlRenderer = nullptr;
		sdlWindow = nullptr;
		context = nullptr;
		contextWidth = 0;
		contextHeight = 0;
		currentApplication = application;
		this->flags = flags;

		int sdlWindowFlags = 0;

		if (flags & WINDOW_FLAG_FULLSCREEN)  sdlWindowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		if (flags & WINDOW_FLAG_RESIZABLE)   sdlWindowFlags |= SDL_WINDOW_RESIZABLE;
		if (flags & WINDOW_FLAG_BORDERLESS)  sdlWindowFlags |= SDL_WINDOW_BORDERLESS;
		if (flags & WINDOW_FLAG_HIDDEN)      sdlWindowFlags |= SDL_WINDOW_HIDDEN;
		if (flags & WINDOW_FLAG_MINIMIZED)   sdlWindowFlags |= SDL_WINDOW_MINIMIZED;
		if (flags & WINDOW_FLAG_MAXIMIZED)   sdlWindowFlags |= SDL_WINDOW_MAXIMIZED;

		#ifndef EMSCRIPTEN
		if (flags & WINDOW_FLAG_ALWAYS_ON_TOP) sdlWindowFlags |= SDL_WINDOW_ALWAYS_ON_TOP;
		#endif

		#if defined(HX_WINDOWS) && defined(NATIVE_TOOLKIT_SDL_ANGLE) && !defined(HX_WINRT)
		{
			OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
			DWORDLONG const dwlConditionMask = VerSetConditionMask(
				VerSetConditionMask(
					VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
					VER_MINORVERSION, VER_GREATER_EQUAL
				),
				VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL
			);
			osvi.dwMajorVersion = HIBYTE(_WIN32_WINNT_VISTA);
			osvi.dwMinorVersion = LOBYTE(_WIN32_WINNT_VISTA);
			osvi.wServicePackMajor = 0;

			if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) == FALSE) {
				flags &= ~WINDOW_FLAG_HARDWARE;
			}
		}
		#endif

		#if !defined(EMSCRIPTEN) && !defined(LIME_SWITCH)
		if (!sdlHintsInitialized) {
			SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "0");
			SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
			SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
			SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
			SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
			SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");
			sdlHintsInitialized = true;
		}
		#endif

		if (flags & WINDOW_FLAG_HARDWARE) {
			sdlWindowFlags |= SDL_WINDOW_OPENGL;

			if (flags & WINDOW_FLAG_ALLOW_HIGHDPI) {
				sdlWindowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
			}

			#ifdef LIME_GLES
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			#endif

			#if LIME_GL
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			#endif

			if (flags & WINDOW_FLAG_DEPTH_BUFFER) {
				const int depthBits = (flags & WINDOW_FLAG_STENCIL_BUFFER) ? 24 : 32;
				SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBits);
			}

			if (flags & WINDOW_FLAG_STENCIL_BUFFER) {
				SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
			}

			if (flags & WINDOW_FLAG_HW_AA_HIRES) {
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
			} else if (flags & WINDOW_FLAG_HW_AA) {
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
				SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
			}

			if (flags & WINDOW_FLAG_COLOR_DEPTH_32_BIT) {
				SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
				SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
				SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
				SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
			} else {
				SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
				SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
				SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			}

			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		}

		#ifdef HX_WINDOWS
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, firstContext ? 0 : 1);
		if (firstContext) firstContext = false;
		#endif

		sdlWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, sdlWindowFlags);

		#if defined(IPHONE) || defined(APPLETV)
		if (sdlWindow && !SDL_GL_CreateContext(sdlWindow)) {
			SDL_DestroyWindow(sdlWindow);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			sdlWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, sdlWindowFlags);
		}
		#endif

		if (!sdlWindow) {
			printf("Could not create SDL window: %s.\n", SDL_GetError());
			return;
		}

		#if defined(HX_WINDOWS) && !defined(HX_WINRT)
		{
			HINSTANCE handle = ::GetModuleHandle(nullptr);
			HICON icon = ::LoadIcon(handle, MAKEINTRESOURCE(1));

			if (icon != nullptr) {
				SDL_SysWMinfo wminfo;
				SDL_VERSION(&wminfo.version);

				if (SDL_GetWindowWMInfo(sdlWindow, &wminfo) == 1) {
					HWND hwnd = wminfo.info.win.window;
					#ifdef _WIN64
					::SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
					#else
					::SetClassLong(hwnd, GCL_HICON, reinterpret_cast<LONG>(icon));
					#endif
				}
			}
		}
		#endif

		int sdlRendererFlags = 0;

		if (flags & WINDOW_FLAG_HARDWARE) {
			sdlRendererFlags |= SDL_RENDERER_ACCELERATED;

			#ifdef EMSCRIPTEN
			sdlRendererFlags |= SDL_RENDERER_PRESENTVSYNC;
			#endif

			context = SDL_GL_CreateContext(sdlWindow);

			if (context && SDL_GL_MakeCurrent(sdlWindow, context) == 0) {
				SDL_GL_SetSwapInterval((flags & WINDOW_FLAG_VSYNC) ? 1 : 0);

				OpenGLBindings::Init();

				#ifndef LIME_GLES
				int version = 0;
				glGetIntegerv(GL_MAJOR_VERSION, &version);

				int versionm = 0;
				glGetIntegerv(GL_MINOR_VERSION, &versionm);
				printf("GL Version %i.%i\n", version, versionm);

				if (version == 0) {
					float versionScan = 0;
					sscanf(reinterpret_cast<const char*>(glGetString(GL_VERSION)), "%f", &versionScan);
					version = static_cast<int>(versionScan);
				}

				if (version < 2 && !strstr(reinterpret_cast<const char*>(glGetString(GL_VERSION)), "OpenGL ES")) {
					SDL_GL_DeleteContext(context);
					context = nullptr;
				}
				#elif defined(IPHONE) || defined(APPLETV)
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &OpenGLBindings::defaultFramebuffer);
				glGetIntegerv(GL_RENDERBUFFER_BINDING, &OpenGLBindings::defaultRenderbuffer);
				#endif
			} else {
				SDL_GL_DeleteContext(context);
				context = nullptr;
			}
		}

		if (!context) {
			sdlRendererFlags &= ~SDL_RENDERER_ACCELERATED;
			sdlRendererFlags &= ~SDL_RENDERER_PRESENTVSYNC;
			sdlRendererFlags |= SDL_RENDERER_SOFTWARE;

			sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, sdlRendererFlags);
		}

		if (context || sdlRenderer) {
			static_cast<SDLApplication*>(currentApplication)->RegisterWindow(this);
		} else {
			printf("Could not create SDL renderer: %s.\n", SDL_GetError());
		}
	}

	SDLWindow::~SDLWindow() {
		if (sdlTexture) {
			SDL_DestroyTexture(sdlTexture);
			sdlTexture = nullptr;
		}

		if (sdlRenderer) {
			SDL_DestroyRenderer(sdlRenderer);
			sdlRenderer = nullptr;
		}

		if (context) {
			SDL_GL_DeleteContext(context);
			context = nullptr;
		}

		if (sdlWindow) {
			SDL_DestroyWindow(sdlWindow);
			sdlWindow = nullptr;
		}
	}

	void SDLWindow::Alert(const char* message, const char* title) {
		#if defined(HX_WINDOWS) && !defined(HX_WINRT)
		{
			SDL_SysWMinfo info;
			SDL_VERSION(&info.version);
			SDL_GetWindowWMInfo(sdlWindow, &info);

			FLASHWINFO fi;
			fi.cbSize = sizeof(FLASHWINFO);
			fi.hwnd = info.info.win.window;
			fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
			fi.uCount = 0;
			fi.dwTimeout = 0;
			FlashWindowEx(&fi);
		}
		#endif

		if (message) {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title, message, sdlWindow);
		}
	}

	void SDLWindow::Close() {
		if (sdlWindow) {
			SDL_DestroyWindow(sdlWindow);
			sdlWindow = nullptr;
		}
	}

	bool SDLWindow::SetVisible(bool visible) {
		if (visible) {
			SDL_ShowWindow(sdlWindow);
		} else {
			SDL_HideWindow(sdlWindow);
		}
		return (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_SHOWN) != 0;
	}

	void SDLWindow::ContextFlip() {
		if (context && !sdlRenderer) {
			SDL_GL_SwapWindow(sdlWindow);
		} else if (sdlRenderer) {
			SDL_RenderPresent(sdlRenderer);
		}
	}

	void* SDLWindow::ContextLock(bool useCFFIValue) {
		if (!sdlRenderer) {
			return useCFFIValue ? alloc_null() : nullptr;
		}

		int width, height;
		SDL_GetRendererOutputSize(sdlRenderer, &width, &height);

		if (width != contextWidth || height != contextHeight) {
			if (sdlTexture) {
				SDL_DestroyTexture(sdlTexture);
			}
			sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
			contextWidth = width;
			contextHeight = height;
		}

		void* pixels;
		int pitch;

		if (SDL_LockTexture(sdlTexture, nullptr, &pixels, &pitch) != 0) {
			return useCFFIValue ? alloc_null() : nullptr;
		}

		if (useCFFIValue) {
			value result = alloc_empty_object();
			alloc_field(result, val_id("width"), alloc_int(contextWidth));
			alloc_field(result, val_id("height"), alloc_int(contextHeight));
			alloc_field(result, val_id("pixels"), alloc_float(reinterpret_cast<uintptr_t>(pixels)));
			alloc_field(result, val_id("pitch"), alloc_int(pitch));
			return result;
		} else {
			static const int id_width = hl_hash_utf8("width");
			static const int id_height = hl_hash_utf8("height");
			static const int id_pixels = hl_hash_utf8("pixels");
			static const int id_pitch = hl_hash_utf8("pitch");

			vdynamic* result = reinterpret_cast<vdynamic*>(hl_alloc_dynobj());
			hl_dyn_seti(result, id_width, &hlt_i32, contextWidth);
			hl_dyn_seti(result, id_height, &hlt_i32, contextHeight);
			hl_dyn_setd(result, id_pixels, static_cast<double>(reinterpret_cast<uintptr_t>(pixels)));
			hl_dyn_seti(result, id_pitch, &hlt_i32, pitch);
			return result;
		}
	}

	void SDLWindow::ContextMakeCurrent() {
		if (sdlWindow && context) {
			SDL_GL_MakeCurrent(sdlWindow, context);
		}
	}

	void SDLWindow::ContextUnlock() {
		if (sdlTexture) {
			SDL_UnlockTexture(sdlTexture);
			SDL_RenderClear(sdlRenderer);
			SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, nullptr);
		}
	}

	void SDLWindow::Focus() {
		SDL_RaiseWindow(sdlWindow);
	}

	void* SDLWindow::GetHandle() {
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		SDL_GetWindowWMInfo(sdlWindow, &info);

		#if defined(SDL_VIDEO_DRIVER_WINDOWS)
		return info.info.win.window;
		#elif defined(SDL_VIDEO_DRIVER_WINRT)
		return info.info.winrt.window;
		#elif defined(SDL_VIDEO_DRIVER_X11)
		return reinterpret_cast<void*>(info.info.x11.window);
		#elif defined(SDL_VIDEO_DRIVER_DIRECTFB)
		return info.info.dfb.window;
		#elif defined(SDL_VIDEO_DRIVER_COCOA)
		return info.info.cocoa.window;
		#elif defined(SDL_VIDEO_DRIVER_UIKIT)
		return info.info.uikit.window;
		#elif defined(SDL_VIDEO_DRIVER_WAYLAND)
		return info.info.wl.surface;
		#elif defined(SDL_VIDEO_DRIVER_ANDROID)
		return info.info.android.window;
		#else
		return nullptr;
		#endif
	}

	void* SDLWindow::GetContext() {
		return context;
	}

	const char* SDLWindow::GetContextType() {
		if (context) {
			return "opengl";
		} else if (sdlRenderer) {
			SDL_RendererInfo info;
			SDL_GetRendererInfo(sdlRenderer, &info);
			return (info.flags & SDL_RENDERER_SOFTWARE) ? "software" : "opengl";
		}
		return "none";
	}

	int SDLWindow::GetDisplay() {
		return SDL_GetWindowDisplayIndex(sdlWindow);
	}

	void SDLWindow::GetDisplayMode(DisplayMode* displayMode) {
		SDL_DisplayMode mode;
		SDL_GetWindowDisplayMode(sdlWindow, &mode);

		displayMode->width = mode.w;
		displayMode->height = mode.h;

		switch (mode.format) {
			case SDL_PIXELFORMAT_ARGB8888:
				displayMode->pixelFormat = ARGB32;
				break;
			case SDL_PIXELFORMAT_BGRA8888:
			case SDL_PIXELFORMAT_BGRX8888:
				displayMode->pixelFormat = BGRA32;
				break;
			default:
				displayMode->pixelFormat = RGBA32;
		}

		displayMode->refreshRate = mode.refresh_rate;
	}

	int SDLWindow::GetHeight() {
		int width, height;
		SDL_GetWindowSize(sdlWindow, &width, &height);
		return height;
	}

	uint32_t SDLWindow::GetID() {
		return SDL_GetWindowID(sdlWindow);
	}

	bool SDLWindow::GetMouseLock() {
		return SDL_GetRelativeMouseMode() == SDL_TRUE;
	}

	float SDLWindow::GetOpacity() {
		float opacity = 1.0f;
		SDL_GetWindowOpacity(sdlWindow, &opacity);
		return opacity;
	}

	double SDLWindow::GetScale() {
		if (sdlRenderer) {
			int outputWidth, outputHeight;
			SDL_GetRendererOutputSize(sdlRenderer, &outputWidth, &outputHeight);

			int width, height;
			SDL_GetWindowSize(sdlWindow, &width, &height);

			return static_cast<double>(outputWidth) / width;
		} else if (context) {
			#ifdef HX_WINDOWS
			float dpi = 0.0f;
			if (SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr) == 0) {
				return dpi / 96.0;
			}
			return 1.0;
			#else
			int outputWidth, outputHeight;
			SDL_GL_GetDrawableSize(sdlWindow, &outputWidth, &outputHeight);

			int width, height;
			SDL_GetWindowSize(sdlWindow, &width, &height);

			return static_cast<double>(outputWidth) / width;
			#endif
		}
		return 1.0;
	}

	bool SDLWindow::GetTextInputEnabled() {
		return SDL_IsTextInputActive() == SDL_TRUE;
	}

	int SDLWindow::GetWidth() {
		int width, height;
		SDL_GetWindowSize(sdlWindow, &width, &height);
		return width;
	}

	int SDLWindow::GetX() {
		int x, y;
		SDL_GetWindowPosition(sdlWindow, &x, &y);
		return x;
	}

	int SDLWindow::GetY() {
		int x, y;
		SDL_GetWindowPosition(sdlWindow, &x, &y);
		return y;
	}

	void SDLWindow::GetMousePosition(int* x, int* y) {
		SDL_GetGlobalMouseState(x, y);
		int winX, winY;
		SDL_GetWindowPosition(sdlWindow, &winX, &winY);
		*x -= winX;
		*y -= winY;
	}

	void SDLWindow::Move(int x, int y) {
		SDL_SetWindowPosition(sdlWindow, x, y);
	}

	void SDLWindow::ReadPixels(ImageBuffer* buffer, Rectangle* rect) {
		if (sdlRenderer) {
			SDL_Rect bounds = { 0, 0, 0, 0 };

			if (rect) {
				bounds.x = static_cast<int>(rect->x);
				bounds.y = static_cast<int>(rect->y);
				bounds.w = static_cast<int>(rect->width);
				bounds.h = static_cast<int>(rect->height);
			} else {
				SDL_GetWindowSize(sdlWindow, &bounds.w, &bounds.h);
			}

			buffer->Resize(bounds.w, bounds.h, 32);
			SDL_RenderReadPixels(sdlRenderer, &bounds, SDL_PIXELFORMAT_ABGR8888, buffer->data->buffer->b, buffer->Stride());
		}
		// TODO: Implement for context
	}

	void SDLWindow::Resize(int width, int height) {
		SDL_SetWindowSize(sdlWindow, width, height);
	}

	void SDLWindow::SetMinimumSize(int width, int height) {
		SDL_SetWindowMinimumSize(sdlWindow, width, height);
	}

	void SDLWindow::SetMaximumSize(int width, int height) {
		SDL_SetWindowMaximumSize(sdlWindow, width, height);
	}

	bool SDLWindow::SetBorderless(bool borderless) {
		SDL_SetWindowBordered(sdlWindow, borderless ? SDL_FALSE : SDL_TRUE);
		return borderless;
	}

	void SDLWindow::SetCursor(Cursor cursor) {
		if (cursor == currentCursor) return;

		if (currentCursor == HIDDEN) {
			SDL_ShowCursor(SDL_ENABLE);
		}

		switch (cursor) {
			case HIDDEN:
				SDL_ShowCursor(SDL_DISABLE);
				currentCursor = HIDDEN;
				return;

			case CROSSHAIR:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_CROSSHAIR>(SDLCursor::crosshairCursor));
				break;

			case MOVE:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_SIZEALL>(SDLCursor::moveCursor));
				break;

			case POINTER:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_HAND>(SDLCursor::pointerCursor));
				break;

			case RESIZE_NESW:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_SIZENESW>(SDLCursor::resizeNESWCursor));
				break;

			case RESIZE_NS:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_SIZENS>(SDLCursor::resizeNSCursor));
				break;

			case RESIZE_NWSE:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_SIZENWSE>(SDLCursor::resizeNWSECursor));
				break;

			case RESIZE_WE:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_SIZEWE>(SDLCursor::resizeWECursor));
				break;

			case TEXT:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_IBEAM>(SDLCursor::textCursor));
				break;

			case WAIT:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_WAIT>(SDLCursor::waitCursor));
				break;

			case WAIT_ARROW:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_WAITARROW>(SDLCursor::waitArrowCursor));
				break;

			case SPINNER:
				if (!SDLCursor::spinnerCursor) SDLCursor::spinnerCursor = CreateCustomCursor(spinner);
				SDL_SetCursor(SDLCursor::spinnerCursor);
				break;

			case DRAG_DROP:
				if (!SDLCursor::dragDropCursor) SDLCursor::dragDropCursor = CreateCustomCursor(dragDrop);
				SDL_SetCursor(SDLCursor::dragDropCursor);
				break;

			case DRAG_DROP_COPY:
				if (!SDLCursor::dragDropCopyCursor) SDLCursor::dragDropCopyCursor = CreateCustomCursor(dragDropCopy);
				SDL_SetCursor(SDLCursor::dragDropCopyCursor);
				break;

			case DRAG_DROP_MOVE:
				if (!SDLCursor::dragDropMoveCursor) SDLCursor::dragDropMoveCursor = CreateCustomCursor(dragDropMove);
				SDL_SetCursor(SDLCursor::dragDropMoveCursor);
				break;

			case DRAG_DROP_NONE:
				if (!SDLCursor::dragDropNoneCursor) SDLCursor::dragDropNoneCursor = CreateCustomCursor(dragDropNone);
				SDL_SetCursor(SDLCursor::dragDropNoneCursor);
				break;

			case RESIZE_DOPE:
				if (!SDLCursor::resize_dope) SDLCursor::resize_dope = CreateCustomCursor(resize_dope);
				SDL_SetCursor(SDLCursor::resize_dope);
				break;

			case TRANS_DIAG:
				if (!SDLCursor::trans_diag) SDLCursor::trans_diag = CreateCustomCursor(trans_diag);
				SDL_SetCursor(SDLCursor::trans_diag);
				break;

			case TRANS_HORZ:
				if (!SDLCursor::trans_horz) SDLCursor::trans_horz = CreateCustomCursor(trans_horz);
				SDL_SetCursor(SDLCursor::trans_horz);
				break;

			case TRANS_ROTATE:
				if (!SDLCursor::trans_rotate) SDLCursor::trans_rotate = CreateCustomCursor(trans_rotate);
				SDL_SetCursor(SDLCursor::trans_rotate);
				break;

			case TRANS_VERT:
				if (!SDLCursor::trans_vert) SDLCursor::trans_vert = CreateCustomCursor(trans_vert);
				SDL_SetCursor(SDLCursor::trans_vert);
				break;

			case TRANS_MOVE:
				if (!SDLCursor::trans_move) SDLCursor::trans_move = CreateCustomCursor(trans_move);
				SDL_SetCursor(SDLCursor::trans_move);
				break;

			case TRANS_DIAG2:
				if (!SDLCursor::trans_diag2) SDLCursor::trans_diag2 = CreateCustomCursor(trans_diag2);
				SDL_SetCursor(SDLCursor::trans_diag2);
				break;

			default:
				SDL_SetCursor(GetOrCreateSystemCursor<SDL_SYSTEM_CURSOR_ARROW>(SDLCursor::arrowCursor));
				break;
		}

		currentCursor = cursor;
	}

	void SDLWindow::SetCursorDirectly(void* cursor) {
		SDL_Cursor* sdlCursor = static_cast<SDL_Cursor*>(cursor);
		if (sdlCursor) {
			if (currentCursor != CUSTOM) {
				if (currentCursor == HIDDEN) {
					SDL_ShowCursor(SDL_ENABLE);
				}
				currentCursor = CUSTOM;
			}
			SDL_SetCursor(sdlCursor);
		}
	}

	void SDLWindow::SetDisplayMode(DisplayMode* displayMode) {
		Uint32 pixelFormat;

		switch (displayMode->pixelFormat) {
			case ARGB32: pixelFormat = SDL_PIXELFORMAT_ARGB8888; break;
			case BGRA32: pixelFormat = SDL_PIXELFORMAT_BGRA8888; break;
			default: pixelFormat = SDL_PIXELFORMAT_RGBA8888;
		}

		SDL_DisplayMode mode = { pixelFormat, displayMode->width, displayMode->height, displayMode->refreshRate, 0 };

		if (SDL_SetWindowDisplayMode(sdlWindow, &mode) == 0) {
			displayModeSet = true;
			if (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
				SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN);
			}
		}
	}

	bool SDLWindow::SetFullscreen(bool fullscreen) {
		SDL_SetWindowFullscreen(sdlWindow, fullscreen ? (displayModeSet ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP) : 0);
		return fullscreen;
	}

	bool SDLWindow::SetFullscreenExclusiveMode(bool exclusive) {
		displayModeSet = exclusive;

		Uint32 flags = SDL_GetWindowFlags(sdlWindow);
		if (displayModeSet && (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN);
		} else if (flags & SDL_WINDOW_FULLSCREEN) {
			SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}

		return displayModeSet;
	}

	void SDLWindow::SetIcon(ImageBuffer* imageBuffer) {
		SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
			imageBuffer->data->buffer->b,
			imageBuffer->width,
			imageBuffer->height,
			imageBuffer->bitsPerPixel,
			imageBuffer->Stride(),
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
		);

		if (surface) {
			SDL_SetWindowIcon(sdlWindow, surface);
			SDL_FreeSurface(surface);
		}
	}

	bool SDLWindow::SetMaximized(bool maximized) {
		if (maximized) {
			SDL_MaximizeWindow(sdlWindow);
		} else {
			SDL_RestoreWindow(sdlWindow);
		}
		return maximized;
	}

	bool SDLWindow::SetMinimized(bool minimized) {
		if (minimized) {
			SDL_MinimizeWindow(sdlWindow);
		} else {
			SDL_RestoreWindow(sdlWindow);
		}
		return minimized;
	}

	void SDLWindow::SetMouseLock(bool mouseLock) {
		SDL_SetRelativeMouseMode(mouseLock ? SDL_TRUE : SDL_FALSE);
	}

	void SDLWindow::SetOpacity(float opacity) {
		SDL_SetWindowOpacity(sdlWindow, opacity);
	}

	bool SDLWindow::SetResizable(bool resizable) {
		#ifndef EMSCRIPTEN
		SDL_SetWindowResizable(sdlWindow, resizable ? SDL_TRUE : SDL_FALSE);
		return (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_RESIZABLE) != 0;
		#else
		return resizable;
		#endif
	}

	void SDLWindow::SetTextInputEnabled(bool enabled) {
		if (enabled) {
			SDL_StartTextInput();
		} else {
			SDL_StopTextInput();
		}
	}

	void SDLWindow::SetTextInputRect(Rectangle* rect) {
		SDL_Rect bounds = { 0, 0, 0, 0 };
		if (rect) {
			bounds.x = static_cast<int>(rect->x);
			bounds.y = static_cast<int>(rect->y);
			bounds.w = static_cast<int>(rect->width);
			bounds.h = static_cast<int>(rect->height);
		}
		SDL_SetTextInputRect(&bounds);
	}

	const char* SDLWindow::SetTitle(const char* title) {
		SDL_SetWindowTitle(sdlWindow, title);
		return title;
	}

	bool SDLWindow::SetVSync(bool vsync) {
		SDL_GL_SetSwapInterval(vsync ? 1 : 0);
		return vsync;
	}

	bool SDLWindow::SetAlwaysOnTop(bool alwaysOnTop) {
		SDL_SetWindowAlwaysOnTop(sdlWindow, alwaysOnTop ? SDL_TRUE : SDL_FALSE);
		return alwaysOnTop;
	}

	void SDLWindow::WarpMouse(int x, int y) {
		SDL_WarpMouseInWindow(sdlWindow, x, y);
	}

	Window* CreateWindow(Application* application, int width, int height, int flags, const char* title) {
		return new SDLWindow(application, width, height, flags, title);
	}

}
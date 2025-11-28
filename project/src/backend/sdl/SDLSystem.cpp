#include <graphics/PixelFormat.h>
#include <math/Rectangle.h>
#include <system/Clipboard.h>
#include <system/DisplayMode.h>
#include <system/JNI.h>
#include <system/System.h>

#ifdef HX_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef HX_WINDOWS
#include <shlobj.h>
#include <stdio.h>
//#include <io.h>
//#include <fcntl.h>
#ifdef __MINGW32__
#ifndef CSIDL_MYDOCUMENTS
#define CSIDL_MYDOCUMENTS CSIDL_PERSONAL
#endif
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif
#endif
#if UNICODE
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "UTF-16LE", (char *)(S), (SDL_wcslen(S)+1)*sizeof(WCHAR))
#define WIN_UTF8ToString(S) (WCHAR *)SDL_iconv_string("UTF-16LE", "UTF-8", (char *)(S), SDL_strlen(S)+1)
#else
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "ASCII", (char *)(S), (SDL_strlen(S)+1))
#define WIN_UTF8ToString(S) SDL_iconv_string("ASCII", "UTF-8", (char *)(S), SDL_strlen(S)+1)
#endif
#endif

#include <SDL.h>
#include <string>

#ifdef HX_WINDOWS
#include <locale>
#include <codecvt>
#endif


namespace lime {


	static int id_bounds;
	static int id_currentMode;
	static int id_dpi;
	static int id_height;
	static int id_name;
	static int id_pixelFormat;
	static int id_refreshRate;
	static int id_supportedModes;
	static int id_width;

	static bool init = false;

	static double performanceFrequency = 0.0;
	static double performanceCounter = 0.0;
	static bool timersInitialized = false;


	static inline void InitTimersOnce() {

		if (timersInitialized)
			return;

		performanceFrequency = (double)SDL_GetPerformanceFrequency();
		performanceCounter = (double)SDL_GetPerformanceCounter();
		timersInitialized = true;

	}


	double System::GetTimer () {

		InitTimersOnce();
		const double counter = (double)SDL_GetPerformanceCounter() - performanceCounter;
		return (counter / performanceFrequency) * 1000.0;

	}


	static inline DisplayMode MakeDisplayModeFromSDL(const SDL_DisplayMode& sdlMode) {
		DisplayMode mode;
		mode.height = sdlMode.h;
		mode.width = sdlMode.w;

		switch (sdlMode.format) {
			case SDL_PIXELFORMAT_ARGB8888:
				mode.pixelFormat = ARGB32;
				break;
			case SDL_PIXELFORMAT_BGRA8888:
			case SDL_PIXELFORMAT_BGRX8888:
				mode.pixelFormat = BGRA32;
				break;
			default:
				mode.pixelFormat = RGBA32;
				break;
		}

		mode.refreshRate = sdlMode.refresh_rate;
		return mode;
	}


	#ifdef HX_WINDOWS
	static inline std::wstring Utf8ToWString(const char* utf8) {

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(utf8 ? utf8 : "");

	}

	static inline std::wstring Utf8ToWString(const std::string& utf8) {

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(utf8);

	}
	#endif

	static inline std::wstring* MakeWStringPtr(const std::wstring& src) {

		return new std::wstring(src);

	}


	const char* Clipboard::GetText () {

		return SDL_GetClipboardText ();

	}


	bool Clipboard::HasText () {

		return SDL_HasClipboardText ();

	}


	bool Clipboard::SetText (const char* text) {

		return (SDL_SetClipboardText (text) == 0);

	}


	void *JNI::GetEnv () {

		#ifdef ANDROID
		return SDL_AndroidGetJNIEnv ();
		#else
		return 0;
		#endif

	}


	bool System::GetAllowScreenTimeout () {

		return SDL_IsScreenSaverEnabled ();

	}


	std::wstring* System::GetDirectory (SystemDirectory type, const char* company, const char* title) {

		std::wstring* result = nullptr;
		System::GCEnterBlocking ();

		switch (type) {

			case APPLICATION: {

				char* path = SDL_GetBasePath ();
				if (path) {
					#ifdef HX_WINDOWS
					std::wstring wpath = Utf8ToWString(path);
					#else
					std::wstring wpath(path, path + strlen(path));
					#endif
					SDL_free(path);
					result = MakeWStringPtr(wpath);
				}
				break;

			}

			case APPLICATION_STORAGE: {

				char* path = SDL_GetPrefPath(company, title);
				if (path) {
					#ifdef HX_WINDOWS
					std::wstring wpath = Utf8ToWString(path);
					#else
					std::wstring wpath(path, path + strlen(path));
					#endif
					SDL_free(path);
					result = MakeWStringPtr(wpath);
				}
				break;

			}

			case DESKTOP: {

				#if defined (HX_WINRT)

				Windows::Storage::StorageFolder^ folder = Windows::Storage::KnownFolders::HomeGroup;
				result = MakeWStringPtr(std::wstring(folder->Path->Data()));

				#elif defined (HX_WINDOWS)

				char folderPath[MAX_PATH] = "";
				if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, folderPath))) {
					result = MakeWStringPtr(Utf8ToWString(folderPath));
				}

				#elif defined (IPHONE)

				result = System::GetIOSDirectory (type);

				#elif !defined (ANDROID)

				const char* home = getenv("HOME");
				if (home) {
					std::string path = std::string(home) + "/Desktop";
					std::wstring wpath(path.begin(), path.end());
					result = MakeWStringPtr(wpath);
				}

				#endif
				break;

			}

			case DOCUMENTS: {

				#if defined (HX_WINRT)

				Windows::Storage::StorageFolder^ folder = Windows::Storage::KnownFolders::DocumentsLibrary;
				result = MakeWStringPtr(std::wstring(folder->Path->Data()));

				#elif defined (HX_WINDOWS)

				char folderPath[MAX_PATH] = "";
				if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, folderPath))) {
					result = MakeWStringPtr(Utf8ToWString(folderPath));
				}

				#elif defined (IPHONE)

				result = System::GetIOSDirectory (type);

				#elif defined (ANDROID)

				result = MakeWStringPtr(std::wstring(L"/mnt/sdcard/Documents"));

				#else

				const char* home = getenv("HOME");
				if (home) {
					std::string path = std::string(home) + "/Documents";
					std::wstring wpath(path.begin(), path.end());
					result = MakeWStringPtr(wpath);
				}

				#endif
				break;

			}

			case FONTS: {

				#if defined (HX_WINRT)
				// TODO
				#elif defined (HX_WINDOWS)
					char folderPath[MAX_PATH] = "";
					if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_FONTS, NULL, SHGFP_TYPE_CURRENT, folderPath))) {
						result = MakeWStringPtr(Utf8ToWString(folderPath));
					}
				#elif defined (HX_MACOS)
					result = MakeWStringPtr(std::wstring(L"/Library/Fonts"));
				#elif defined (IPHONE)
					result = MakeWStringPtr(std::wstring(L"/System/Library/Fonts"));
				#elif defined (ANDROID)
					result = MakeWStringPtr(std::wstring(L"/system/fonts"));
				#elif defined (BLACKBERRY)
					result = MakeWStringPtr(std::wstring(L"/usr/fonts/font_repository/monotype"));
				#else
					result = MakeWStringPtr(std::wstring(L"/usr/share/fonts/truetype"));
				#endif
				break;

			}

			case USER: {

				#if defined (HX_WINRT)

				Windows::Storage::StorageFolder^ folder = Windows::Storage::ApplicationData::Current->RoamingFolder;
				result = MakeWStringPtr(std::wstring(folder->Path->Data()));

				#elif defined (HX_WINDOWS)

				char folderPath[MAX_PATH] = "";
				if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, folderPath))) {
					result = MakeWStringPtr(Utf8ToWString(folderPath));
				}

				#elif defined (IPHONE)

				result = System::GetIOSDirectory (type);

				#elif defined (ANDROID)

				result = MakeWStringPtr(std::wstring(L"/mnt/sdcard"));

				#else

				const char* home = getenv("HOME");
				if (home) {
					std::string path = std::string(home);
					std::wstring wpath(path.begin(), path.end());
					result = MakeWStringPtr(wpath);
				}

				#endif
				break;

			}

		}

		System::GCExitBlocking ();
		return result;

	}


	void* System::GetDisplay (bool useCFFIValue, int id) {

		if (useCFFIValue) {

			if (!init) {
				// display shit
				id_bounds = val_id ("bounds");
				id_currentMode = val_id ("currentMode");
				id_dpi = val_id ("dpi");
				id_height = val_id ("height");
				id_name = val_id ("name");
				id_pixelFormat = val_id ("pixelFormat");
				id_refreshRate = val_id ("refreshRate");
				id_supportedModes = val_id ("supportedModes");
				id_width = val_id ("width");

				InitTimersOnce();
				init = true;

			}

			int numDisplays = GetNumDisplays ();

			if (id < 0 || id >= numDisplays) {

				return alloc_null ();

			}

			value display = alloc_empty_object ();
			alloc_field (display, id_name, alloc_string (SDL_GetDisplayName (id)));

			SDL_Rect bounds = { 0, 0, 0, 0 };
			SDL_GetDisplayBounds (id, &bounds);
			alloc_field (display, id_bounds, Rectangle (bounds.x, bounds.y, bounds.w, bounds.h).Value ());

			float dpi = 72.0f;
			#ifndef EMSCRIPTEN
			SDL_GetDisplayDPI(id, &dpi, NULL, NULL);
			#endif
			alloc_field(display, id_dpi, alloc_float (dpi));

			SDL_DisplayMode sdlMode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };
			SDL_GetDesktopDisplayMode(id, &sdlMode);

			DisplayMode mode = MakeDisplayModeFromSDL (sdlMode);
			alloc_field (display, id_currentMode, (value)mode.Value ());

			const int numDisplayModes = SDL_GetNumDisplayModes (id);
			value supportedModes = alloc_array (numDisplayModes);

			for (int i = 0; i < numDisplayModes; ++i) {

				SDL_GetDisplayMode(id, i, &sdlMode);
				DisplayMode m = MakeDisplayModeFromSDL (sdlMode);
				val_array_set_i(supportedModes, i, (value)m.Value());

			}

			alloc_field (display, id_supportedModes, supportedModes);
			return display;

		} else {

			if (!init) {
				InitTimersOnce();
				init = true;
			}

			const int id_bounds = hl_hash_utf8 ("bounds");
			const int id_currentMode = hl_hash_utf8 ("currentMode");
			const int id_dpi = hl_hash_utf8 ("dpi");
			const int id_height = hl_hash_utf8 ("height");
			const int id_name = hl_hash_utf8 ("name");
			const int id_pixelFormat = hl_hash_utf8 ("pixelFormat");
			const int id_refreshRate = hl_hash_utf8 ("refreshRate");
			const int id_supportedModes = hl_hash_utf8 ("supportedModes");
			const int id_width = hl_hash_utf8 ("width");
			const int id_x = hl_hash_utf8 ("x");
			const int id_y = hl_hash_utf8 ("y");

			int numDisplays = GetNumDisplays ();

			if (id < 0 || id >= numDisplays) {

				return 0;

			}

			vdynamic* display = (vdynamic*)hl_alloc_dynobj ();

			const char* displayName = SDL_GetDisplayName (id);
			const size_t nameLen = strlen(displayName) + 1;
			char* _displayName = (char*)malloc(nameLen);
			if (_displayName)
				memcpy(_displayName, displayName, nameLen);
			hl_dyn_setp (display, id_name, &hlt_bytes, _displayName);

			SDL_Rect bounds = { 0, 0, 0, 0 };
			SDL_GetDisplayBounds (id, &bounds);

			vdynamic* _bounds = (vdynamic*)hl_alloc_dynobj ();
			hl_dyn_seti (_bounds, id_x, &hlt_i32, bounds.x);
			hl_dyn_seti (_bounds, id_y, &hlt_i32, bounds.y);
			hl_dyn_seti (_bounds, id_width, &hlt_i32, bounds.w);
			hl_dyn_seti (_bounds, id_height, &hlt_i32, bounds.h);

			hl_dyn_setp (display, id_bounds, &hlt_dynobj, _bounds);

			float dpi = 72.0;
			#ifndef EMSCRIPTEN
			SDL_GetDisplayDPI (id, &dpi, NULL, NULL);
			#endif
			hl_dyn_setf (display, id_dpi, dpi);

			SDL_DisplayMode sdlMode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };
			SDL_GetDesktopDisplayMode (id, &sdlMode);
			DisplayMode mode = MakeDisplayModeFromSDL(sdlMode);

			vdynamic* _displayMode = (vdynamic*)hl_alloc_dynobj ();
			hl_dyn_seti (_displayMode, id_height, &hlt_i32, mode.height);
			hl_dyn_seti (_displayMode, id_pixelFormat, &hlt_i32, mode.pixelFormat);
			hl_dyn_seti (_displayMode, id_refreshRate, &hlt_i32, mode.refreshRate);
			hl_dyn_seti (_displayMode, id_width, &hlt_i32, mode.width);
			hl_dyn_setp (display, id_currentMode, &hlt_dynobj, _displayMode);

			const int numDisplayModes = SDL_GetNumDisplayModes (id);

			hl_varray* supportedModes = (hl_varray*)hl_alloc_array (&hlt_dynobj, numDisplayModes);
			vdynamic** supportedModesData = hl_aptr (supportedModes, vdynamic*);

			for (int i = 0; i < numDisplayModes; i++) {

				SDL_GetDisplayMode (id, i, &sdlMode);
				DisplayMode m = MakeDisplayModeFromSDL(sdlMode);

				vdynamic* dm = (vdynamic*)hl_alloc_dynobj ();
				hl_dyn_seti (dm, id_height, &hlt_i32, m.height);
				hl_dyn_seti (dm, id_pixelFormat, &hlt_i32, m.pixelFormat);
				hl_dyn_seti (dm, id_refreshRate, &hlt_i32, m.refreshRate);
				hl_dyn_seti (dm, id_width, &hlt_i32, m.width);

				*supportedModesData++ = dm;

			}

			hl_dyn_setp (display, id_supportedModes, &hlt_array, supportedModes);
			return display;

		}

	}

	#if defined(ANDROID) || defined (IPHONE)


	int System::GetFirstGyroscopeSensorId () {
		int numSensors = SDL_NumSensors ();
		for (int i = 0; i < numSensors; i++) {
			if (SDL_SensorGetDeviceType (i) == SDL_SENSOR_GYRO) {
				return SDL_SensorGetDeviceInstanceID(i);
			}
		}
		return -1;
	}


	int System::GetFirstAccelerometerSensorId () {
		int numSensors = SDL_NumSensors ();
		for (int i = 0; i < numSensors; i++) {
			if (SDL_SensorGetDeviceType (i) == SDL_SENSOR_ACCEL) {
				return SDL_SensorGetDeviceInstanceID(i);
			}
		}
		return -1;
	}


	#endif


	int System::GetNumDisplays () {

		return SDL_GetNumVideoDisplays ();

	}


	bool System::SetAllowScreenTimeout (bool allow) {

		if (allow) {

			SDL_EnableScreenSaver ();

		} else {

			SDL_DisableScreenSaver ();

		}

		return allow;

	}


	FILE* FILE_HANDLE::getFile () {

		#ifndef HX_WINDOWS

		if (!handle)
			return nullptr;

		SDL_RWops* rw = (SDL_RWops*)handle;

		switch (rw->type) {

			case SDL_RWOPS_STDFILE:
				return rw->hidden.stdio.fp;

			case SDL_RWOPS_JNIFILE:
			{
				#ifdef ANDROID
				GCBlockingScope gcScope;
				FILE* file = ::fdopen (rw->hidden.androidio.fd, "rb");
				if (file)
					::fseek (file, rw->hidden.androidio.offset, SEEK_SET);
				return file;
				#else
				return nullptr;
				#endif
			}

			case SDL_RWOPS_WINFILE:
			default:
				return nullptr;
		}

		return NULL;

		#else

		return (FILE*)handle;

		#endif

	}


	int FILE_HANDLE::getLength () {

		#ifndef HX_WINDOWS

		if (!handle)
			return 0;

		System::GCEnterBlocking ();
		const Sint64 size = SDL_RWsize((SDL_RWops*)handle);
		System::GCExitBlocking ();
		return size > 0 ? (int)size : 0;

		#else

		return 0;

		#endif

	}


	bool FILE_HANDLE::isFile () {

		#ifndef HX_WINDOWS

		if (!handle)
			return false;

		return ((SDL_RWops*)handle)->type == SDL_RWOPS_STDFILE;

		#else

		return handle != nullptr;

		#endif

	}


	int fclose (FILE_HANDLE *stream) {

		if (!stream)
			return 0;

		System::GCEnterBlocking ();

		#ifndef HX_WINDOWS
		int code = SDL_RWclose ((SDL_RWops*)stream->handle);
		#else
		int code = ::fclose ((FILE*)stream->handle);
		#endif

		delete stream;

		System::GCExitBlocking ();

		return code;

	}


	FILE_HANDLE *fdopen (int fd, const char *mode) {

		if (fd < 0 || !mode)
			return NULL;

		#ifndef HX_WINDOWS

		System::GCEnterBlocking ();
		FILE* fp = ::fdopen (fd, mode);
		if (!fp) {
			System::GCExitBlocking ();
			return NULL;
		}

		SDL_RWops *result = SDL_RWFromFP (fp, SDL_TRUE);
		if (!result) {
			::fclose (fp);
			System::GCExitBlocking ();
			return NULL;
		}
		System::GCExitBlocking ();

		return new FILE_HANDLE (result);

		#else

		System::GCEnterBlocking ();
		FILE* result = ::fdopen (fd, mode);
		System::GCExitBlocking ();

		if (result)
			return new FILE_HANDLE (result);

		return NULL;

		#endif
	}


	FILE_HANDLE *fopen (const char *filename, const char *mode) {

		if (!filename || !mode)
			return nullptr;

		#ifndef HX_WINDOWS

		SDL_RWops* result = nullptr;

		System::GCEnterBlocking ();

		#ifdef HX_MACOS

		result = SDL_RWFromFile (filename, mode);

		if (!result) {

			CFStringRef str = CFStringCreateWithCString (NULL, filename, kCFStringEncodingUTF8);
			CFURLRef path   = CFBundleCopyResourceURL (CFBundleGetMainBundle (), str, NULL, NULL);
			CFRelease (str);

			if (path) {

				str = CFURLCopyPath (path);
				CFIndex maxSize = CFStringGetMaximumSizeForEncoding (CFStringGetLength (str), kCFStringEncodingUTF8);
				char* buffer = (char*)malloc ((size_t)maxSize + 1);

				if (buffer) {

					if (CFStringGetCString (str, buffer, maxSize + 1, kCFStringEncodingUTF8)) {

						FILE* fp = ::fopen (buffer, mode);
						if (fp) {
							result = SDL_RWFromFP (fp, SDL_TRUE);
							if (!result)
								::fclose(fp);
						}

					}

					free (buffer);
				}

				CFRelease (str);
				CFRelease (path);

			}
		}

		#else

		result = SDL_RWFromFile (filename, mode);

		#endif

		System::GCExitBlocking ();


		if (!result)
			return nullptr;

		return new FILE_HANDLE (result);

		#else

		std::wstring wfilename = Utf8ToWString(filename);
		std::wstring wmode  = Utf8ToWString(mode);

		System::GCEnterBlocking ();
		FILE* result = ::_wfopen (wfilename.c_str(), wmode.c_str());
		if (!result)
			return nullptr;
		System::GCExitBlocking ();

		return new FILE_HANDLE (result);

		#endif

	}


	size_t fread (void *ptr, size_t size, size_t count, FILE_HANDLE *stream) {

		if (!stream || !ptr || size == 0 || count == 0)
			return 0;

		size_t nmem = 0;

		System::GCEnterBlocking ();

		#ifndef HX_WINDOWS

		nmem = SDL_RWread ((SDL_RWops*)stream->handle, ptr, size, count);

		#else

		nmem = ::fread (ptr, size, count, (FILE*)stream->handle);

		#endif

		System::GCExitBlocking ();
		return nmem;
	}


	int fseek (FILE_HANDLE *stream, long int offset, int origin) {

		if (!stream)
			return -1;

		int success;

		System::GCEnterBlocking ();

		#ifndef HX_WINDOWS

		success = SDL_RWseek ((SDL_RWops*)stream->handle, offset, origin);

		#else

		success = ::fseek ((FILE*)stream->handle, offset, origin);

		#endif

		System::GCExitBlocking ();
		return success;
	}


	long int ftell (FILE_HANDLE *stream) {

		if (!stream)
			return -1L;


		long int pos;

		System::GCEnterBlocking ();

		#ifndef HX_WINDOWS

		Sint64 p = SDL_RWtell ((SDL_RWops*)stream->handle);
		pos = (p >= 0) ? (long int)p : -1L;

		#else

		pos = ::ftell ((FILE*)stream->handle);

		#endif

		System::GCExitBlocking ();
		return pos;
	}


	size_t fwrite (const void *ptr, size_t size, size_t count, FILE_HANDLE *stream) {

		if (!stream || !ptr || size == 0 || count == 0)
			return 0;

		size_t nmem = 0;

		System::GCEnterBlocking ();

		#ifndef HX_WINDOWS

		nmem = SDL_RWwrite ((SDL_RWops*)stream->handle, ptr, size, count);

		#else

		nmem = ::fwrite (ptr, size, count, (FILE*)stream->handle);

		#endif

		System::GCExitBlocking ();
		return nmem;

	}


}
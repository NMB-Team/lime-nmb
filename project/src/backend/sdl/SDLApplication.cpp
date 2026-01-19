#include "SDLApplication.h"
#include "SDLGamepad.h"
#include "SDLJoystick.h"
#include <system/System.h>
#include <algorithm>
#include <cstring>

#ifdef HX_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef HX_WINDOWS
#include <windows.h>
#include <shellapi.h>
#endif

#ifdef EMSCRIPTEN
#include "emscripten.h"
#endif

#ifdef LIME_SDL_SOUND
#include "media/SDLSound.h"
#include "SDL_sound.h"
#endif

namespace lime {

	AutoGCRoot* Application::callback = nullptr;
	SDLApplication* SDLApplication::currentApplication = nullptr;

	static constexpr int ANALOG_AXIS_DEADZONE = 1000;
	static constexpr double FRAME_TIME_CATCHUP_THRESHOLD = 2.0;

	static thread_local std::map<int, std::map<int, int>> gamepadsAxisMap;
	static bool inBackground = false;

	static double performanceFrequency = 0.0;
	static double performanceCounter = 0.0;
	static bool performanceInitialized = false;

	#if defined(ANDROID) || defined(IPHONE)
	static SDL_SensorID gyroscopeSensorID = -1;
	static SDL_Sensor* gyroscopeSensor = nullptr;
	static SDL_SensorID accelerometerSensorID = -1;
	static SDL_Sensor* accelerometerSensor = nullptr;
	#endif

	inline double NormalizeAxis(Sint16 value) {
		if (value > 0)
			return value / 32767.;
		else
			return value / 32768.;
	}

	static inline double GetTimeMs() noexcept {
		const double counter = static_cast<double>(SDL_GetPerformanceCounter()) - performanceCounter;
		return (counter / performanceFrequency) * 1000.0;
	}

	static inline void PushUpdate() noexcept {
		SDL_Event event;
		std::memset(&event, 0, sizeof(event));
		event.type = SDL_USEREVENT;
		event.user.type = SDL_USEREVENT;
		event.user.code = 0;
		event.user.data1 = nullptr;
		event.user.data2 = nullptr;
		SDL_PushEvent(&event);
	}

	SDLApplication::SDLApplication() {
		initFlags = SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER | SDL_INIT_JOYSTICK;

		#if defined(LIME_MOJOAL) || defined(LIME_OPENALSOFT)
		initFlags |= SDL_INIT_AUDIO;
		#endif

		#ifdef HX_WINDOWS
		// Define DPI awareness types if not available
		#ifndef DPI_AWARENESS_CONTEXT
		DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
		#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
		#endif

		// Windows 10 1607+ DPI awareness
		HMODULE user32 = GetModuleHandleA("user32.dll");
		BOOL dpiSet = FALSE;

		if (user32) {
			typedef BOOL (WINAPI * SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT);

			SetProcessDpiAwarenessContextFunc pSetProcessDpiAwarenessContext =
				(SetProcessDpiAwarenessContextFunc)GetProcAddress(user32, "SetProcessDpiAwarenessContext");

			if (pSetProcessDpiAwarenessContext)
				dpiSet = pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		}

		// Windows 8.1+ SetProcessDpiAwareness
		if (!dpiSet) {
			HMODULE shcore = LoadLibraryA("shcore.dll");
			if (shcore) {
				typedef enum { PROCESS_DPI_UNAWARE = 0, PROCESS_SYSTEM_DPI_AWARE = 1, PROCESS_PER_MONITOR_DPI_AWARE = 2 } PROCESS_DPI_AWARENESS;
				typedef HRESULT (WINAPI * SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);

				SetProcessDpiAwarenessFunc pSetProcessDpiAwareness =
					(SetProcessDpiAwarenessFunc)GetProcAddress(shcore, "SetProcessDpiAwareness");

				if (pSetProcessDpiAwareness)
					dpiSet = SUCCEEDED(pSetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE));

				FreeLibrary(shcore);
			}
		}

		// Windows Vista+ SetProcessDPIAware
		if (!dpiSet && user32) {
			typedef BOOL (WINAPI * SetProcessDPIAwareFunc)(void);
			SetProcessDPIAwareFunc pSetProcessDPIAware =
				(SetProcessDPIAwareFunc)GetProcAddress(user32, "SetProcessDPIAware");

			if (pSetProcessDPIAware)
				pSetProcessDPIAware();

		}
		#endif

		if (SDL_Init(initFlags) != 0) {
			printf("Could not initialize SDL: %s.\n", SDL_GetError());
			return;
		}

		#ifdef LIME_SDL_SOUND
		Sound_Init();
		#endif

		if (!performanceInitialized) {
			performanceFrequency = static_cast<double>(SDL_GetPerformanceFrequency());
			performanceCounter = static_cast<double>(SDL_GetPerformanceCounter());
			performanceInitialized = true;
		}

		SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN);

		currentApplication = this;

		framePeriod = 0.0;
		firstFrame = true;
		currentUpdate = 0.0;
		lastUpdate = 0.0;
		nextFrameTime = 0.0;
		active = false;

		#if defined(ANDROID) || defined(IPHONE)
		SDL_EventState(SDL_SENSORUPDATE, SDL_ENABLE);
		#endif
		SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

		#if defined(ANDROID) || defined(IPHONE)
		InitializeSensors();
		#endif

		SDLJoystick::Init();

		#ifdef HX_MACOS
		CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
		char path[PATH_MAX];
		if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, reinterpret_cast<UInt8*>(path), PATH_MAX)) {
			chdir(path);
		}
		CFRelease(resourcesURL);
		#endif
	}

	#if defined(ANDROID) || defined(IPHONE)
	void SDLApplication::InitializeSensors() {
		gyroscopeSensorID = System::GetFirstGyroscopeSensorId();
		if (gyroscopeSensorID > 0) {
			gyroscopeSensor = SDL_SensorOpen(gyroscopeSensorID);
		}

		accelerometerSensorID = System::GetFirstAccelerometerSensorId();
		if (accelerometerSensorID > 0) {
			accelerometerSensor = SDL_SensorOpen(accelerometerSensorID);
		}
	}
	#endif

	SDLApplication::~SDLApplication() {
		#if defined(ANDROID) || defined(IPHONE)
		if (gyroscopeSensor) {
			SDL_SensorClose(gyroscopeSensor);
			gyroscopeSensor = nullptr;
			gyroscopeSensorID = -1;
		}

		if (accelerometerSensor) {
			SDL_SensorClose(accelerometerSensor);
			accelerometerSensor = nullptr;
			accelerometerSensorID = -1;
		}
		#endif

		gamepadsAxisMap.clear();
	}

	int SDLApplication::Exec() {
		Init();

		#ifdef EMSCRIPTEN
		emscripten_cancel_main_loop();
		emscripten_set_main_loop(UpdateFrame, 0, 0);
		emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
		#endif

		#if defined(IPHONE) || defined(EMSCRIPTEN)
		return 0;
		#else
		while (active) {
			Update();
		}
		return Quit();
		#endif
	}

	void SDLApplication::HandleEvent(SDL_Event* event) {
		#if defined(IPHONE) || defined(EMSCRIPTEN)
		int top = 0;
		gc_set_top_of_stack(&top, false);
		#endif

		switch (event -> type) {
			case SDL_USEREVENT:
				if (!inBackground) {
					applicationEvent.type = UPDATE;
					applicationEvent.deltaTime = currentUpdate - lastUpdate;
					ApplicationEvent::Dispatch(&applicationEvent);
					RenderEvent::Dispatch(&renderEvent);
				}
				break;

			case SDL_APP_WILLENTERBACKGROUND:
				inBackground = true;
				windowEvent.type = WINDOW_DEACTIVATE;
				WindowEvent::Dispatch(&windowEvent);
				break;

			case SDL_APP_WILLENTERFOREGROUND:
				break;

			case SDL_APP_DIDENTERFOREGROUND:
				windowEvent.type = WINDOW_ACTIVATE;
				WindowEvent::Dispatch(&windowEvent);
				inBackground = false;
				break;

			case SDL_CLIPBOARDUPDATE:
				ProcessClipboardEvent(event);
				break;

			case SDL_CONTROLLERAXISMOTION:
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
				ProcessGamepadEvent(event);
				break;

			case SDL_DROPFILE:
				ProcessDropEvent(event);
				break;

			case SDL_FINGERMOTION:
			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
				ProcessTouchEvent(event);
				break;

			#if defined(ANDROID) || defined(IPHONE)
			case SDL_SENSORUPDATE:
				ProcessSensorEvent(event);
				break;
			#endif

			case SDL_JOYAXISMOTION:
			case SDL_JOYBALLMOTION:
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYHATMOTION:
			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED:
				ProcessJoystickEvent(event);
				break;

			case SDL_KEYDOWN:
			case SDL_KEYUP:
				ProcessKeyEvent(event);
				break;

			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEWHEEL:
				ProcessMouseEvent(event);
				break;

			#ifndef EMSCRIPTEN
			case SDL_RENDER_DEVICE_RESET:
				renderEvent.type = RENDER_CONTEXT_LOST;
				RenderEvent::Dispatch(&renderEvent);
				renderEvent.type = RENDER_CONTEXT_RESTORED;
				RenderEvent::Dispatch(&renderEvent);
				renderEvent.type = RENDER;
				break;
			#endif

			case SDL_TEXTINPUT:
			case SDL_TEXTEDITING:
			case SDL_TEXTEDITING_EXT:
				ProcessTextEvent(event);
				break;

			case SDL_WINDOWEVENT:
				HandleWindowEvent(event);
				break;

			case SDL_QUIT:
				active = false;
				break;
		}
	}

	void SDLApplication::HandleWindowEvent(SDL_Event* event) {
		switch (event -> window.event) {
			case SDL_WINDOWEVENT_ENTER:
			case SDL_WINDOWEVENT_LEAVE:
			case SDL_WINDOWEVENT_SHOWN:
			case SDL_WINDOWEVENT_HIDDEN:
			case SDL_WINDOWEVENT_FOCUS_GAINED:
			case SDL_WINDOWEVENT_FOCUS_LOST:
			case SDL_WINDOWEVENT_MAXIMIZED:
			case SDL_WINDOWEVENT_MINIMIZED:
			case SDL_WINDOWEVENT_MOVED:
			case SDL_WINDOWEVENT_RESTORED:
				ProcessWindowEvent(event);
				break;

			case SDL_WINDOWEVENT_EXPOSED:
				ProcessWindowEvent(event);
				if (!inBackground) {
					RenderEvent::Dispatch(&renderEvent);
				}
				break;

			case SDL_WINDOWEVENT_SIZE_CHANGED:
				ProcessWindowEvent(event);
				if (!inBackground) {
					RenderEvent::Dispatch(&renderEvent);
				}
				break;

			case SDL_WINDOWEVENT_CLOSE:
				ProcessWindowEvent(event);
				{
					SDL_Event nextEvent;
					if (SDL_PollEvent(&nextEvent)) {
						if (nextEvent.type != SDL_QUIT) {
							HandleEvent(&nextEvent);
						}
					}
				}
				break;
		}
	}

	void SDLApplication::Init() {
		active = true;
		const double now = GetTimeMs();
		lastUpdate = now;
		currentUpdate = now;
		nextFrameTime = now;
	}

	void SDLApplication::ProcessClipboardEvent(SDL_Event* event) {
		if (ClipboardEvent::callback) {
			clipboardEvent.type = CLIPBOARD_UPDATE;
			ClipboardEvent::Dispatch(&clipboardEvent);
		}
	}

	void SDLApplication::ProcessDropEvent(SDL_Event* event) {
		if (DropEvent::callback) {
			dropEvent.type = DROP_FILE;
			dropEvent.file = reinterpret_cast<vbyte*>(event -> drop.file);
			DropEvent::Dispatch(&dropEvent);
			SDL_free(event -> drop.file);
		}
	}

	void SDLApplication::ProcessGamepadEvent(SDL_Event* event) {
		if (!GamepadEvent::callback) return;

		switch (event -> type) {
			case SDL_CONTROLLERAXISMOTION: {
				auto& axisMap = gamepadsAxisMap[event -> caxis.which];
				auto& storedValue = axisMap[event -> caxis.axis];

				if (storedValue == event -> caxis.value) break;

				gamepadEvent.type = GAMEPAD_AXIS_MOVE;
				gamepadEvent.axis = event -> caxis.axis;
				gamepadEvent.id = event -> caxis.which;

				const int value = event -> caxis.value;

				if (std::abs(value) < ANALOG_AXIS_DEADZONE) {
					if (storedValue != 0) {
						storedValue = 0;
						gamepadEvent.axisValue = 0.0;
						GamepadEvent::Dispatch(&gamepadEvent);
					}
					break;
				}

				storedValue = value;
				gamepadEvent.axisValue = NormalizeAxis(value);
				GamepadEvent::Dispatch(&gamepadEvent);
				break;
			}

			case SDL_CONTROLLERBUTTONDOWN:
				gamepadEvent.type = GAMEPAD_BUTTON_DOWN;
				gamepadEvent.button = event -> cbutton.button;
				gamepadEvent.id = event -> cbutton.which;
				GamepadEvent::Dispatch(&gamepadEvent);
				break;

			case SDL_CONTROLLERBUTTONUP:
				gamepadEvent.type = GAMEPAD_BUTTON_UP;
				gamepadEvent.button = event -> cbutton.button;
				gamepadEvent.id = event -> cbutton.which;
				GamepadEvent::Dispatch(&gamepadEvent);
				break;

			case SDL_CONTROLLERDEVICEADDED:
				if (SDLGamepad::Connect(event -> cdevice.which)) {
					gamepadEvent.type = GAMEPAD_CONNECT;
					gamepadEvent.id = SDLGamepad::GetInstanceID(event -> cdevice.which);
					GamepadEvent::Dispatch(&gamepadEvent);
				}
				break;

			case SDL_CONTROLLERDEVICEREMOVED:
				gamepadEvent.type = GAMEPAD_DISCONNECT;
				gamepadEvent.id = event -> cdevice.which;
				GamepadEvent::Dispatch(&gamepadEvent);
				SDLGamepad::Disconnect(event -> cdevice.which);
				gamepadsAxisMap.erase(event -> cdevice.which);
				break;
		}
	}

	void SDLApplication::ProcessJoystickEvent(SDL_Event* event) {
		if (!JoystickEvent::callback) return;

		switch (event -> type) {
			case SDL_JOYAXISMOTION:
				if (!SDLJoystick::IsAccelerometer(event -> jaxis.which)) {
					joystickEvent.type = JOYSTICK_AXIS_MOVE;
					joystickEvent.index = event -> jaxis.axis;
					joystickEvent.x = NormalizeAxis(event -> jaxis.value);
					joystickEvent.id = event -> jaxis.which;
					JoystickEvent::Dispatch(&joystickEvent);
				}
				break;

			case SDL_JOYBUTTONDOWN:
				if (!SDLJoystick::IsAccelerometer(event -> jbutton.which)) {
					joystickEvent.type = JOYSTICK_BUTTON_DOWN;
					joystickEvent.index = event -> jbutton.button;
					joystickEvent.id = event -> jbutton.which;
					JoystickEvent::Dispatch(&joystickEvent);
				}
				break;

			case SDL_JOYBUTTONUP:
				if (!SDLJoystick::IsAccelerometer(event -> jbutton.which)) {
					joystickEvent.type = JOYSTICK_BUTTON_UP;
					joystickEvent.index = event -> jbutton.button;
					joystickEvent.id = event -> jbutton.which;
					JoystickEvent::Dispatch(&joystickEvent);
				}
				break;

			case SDL_JOYHATMOTION:
				if (!SDLJoystick::IsAccelerometer(event -> jhat.which)) {
					joystickEvent.type = JOYSTICK_HAT_MOVE;
					joystickEvent.index = event -> jhat.hat;
					joystickEvent.eventValue = event -> jhat.value;
					joystickEvent.id = event -> jhat.which;
					JoystickEvent::Dispatch(&joystickEvent);
				}
				break;

			case SDL_JOYDEVICEADDED:
				if (SDLJoystick::Connect(event -> jdevice.which)) {
					joystickEvent.type = JOYSTICK_CONNECT;
					joystickEvent.id = SDLJoystick::GetInstanceID(event -> jdevice.which);
					JoystickEvent::Dispatch(&joystickEvent);
				}
				break;

			case SDL_JOYDEVICEREMOVED:
				if (!SDLJoystick::IsAccelerometer(event -> jdevice.which)) {
					joystickEvent.type = JOYSTICK_DISCONNECT;
					joystickEvent.id = event -> jdevice.which;
					JoystickEvent::Dispatch(&joystickEvent);
					SDLJoystick::Disconnect(event -> jdevice.which);
				}
				break;
		}
	}

	void SDLApplication::ProcessKeyEvent(SDL_Event* event) {
		if (!KeyEvent::callback) return;

		if (event -> type == SDL_KEYDOWN)
			keyEvent.type = KEY_DOWN;
		else
			keyEvent.type = KEY_UP;

		keyEvent.keyCode = event -> key.keysym.sym;
		keyEvent.modifier = event -> key.keysym.mod;
		keyEvent.windowID = event -> key.windowID;

		if (keyEvent.type == KEY_DOWN) {
			const SDL_Keycode code = static_cast<SDL_Keycode>(keyEvent.keyCode);

			switch (code) {
				case SDLK_CAPSLOCK:     keyEvent.modifier |= KMOD_CAPS; break;
				case SDLK_LALT:         keyEvent.modifier |= KMOD_LALT; break;
				case SDLK_LCTRL:        keyEvent.modifier |= KMOD_LCTRL; break;
				case SDLK_LGUI:         keyEvent.modifier |= KMOD_LGUI; break;
				case SDLK_LSHIFT:       keyEvent.modifier |= KMOD_LSHIFT; break;
				case SDLK_MODE:         keyEvent.modifier |= KMOD_MODE; break;
				case SDLK_NUMLOCKCLEAR: keyEvent.modifier |= KMOD_NUM; break;
				case SDLK_RALT:         keyEvent.modifier |= KMOD_RALT; break;
				case SDLK_RCTRL:        keyEvent.modifier |= KMOD_RCTRL; break;
				case SDLK_RGUI:         keyEvent.modifier |= KMOD_RGUI; break;
				case SDLK_RSHIFT:       keyEvent.modifier |= KMOD_RSHIFT; break;
				default: break;
			}
		}

		KeyEvent::Dispatch(&keyEvent);
	}

	void SDLApplication::ProcessMouseEvent(SDL_Event* event) {
		if (!MouseEvent::callback) return;

		switch (event -> type) {
			case SDL_MOUSEMOTION:
				mouseEvent.type = MOUSE_MOVE;
				mouseEvent.x = event -> motion.x;
				mouseEvent.y = event -> motion.y;
				mouseEvent.movementX = event -> motion.xrel;
				mouseEvent.movementY = event -> motion.yrel;
				mouseEvent.windowID = event -> motion.windowID;
				break;

			case SDL_MOUSEBUTTONDOWN:
				SDL_CaptureMouse(SDL_TRUE);
				mouseEvent.type = MOUSE_DOWN;
				mouseEvent.button = event -> button.button - 1;
				mouseEvent.x = event -> button.x;
				mouseEvent.y = event -> button.y;
				mouseEvent.clickCount = event -> button.clicks;
				mouseEvent.windowID = event -> button.windowID;
				break;

			case SDL_MOUSEBUTTONUP:
				SDL_CaptureMouse(SDL_FALSE);
				mouseEvent.type = MOUSE_UP;
				mouseEvent.button = event -> button.button - 1;
				mouseEvent.x = event -> button.x;
				mouseEvent.y = event -> button.y;
				mouseEvent.clickCount = event -> button.clicks;
				mouseEvent.windowID = event -> button.windowID;
				break;

			case SDL_MOUSEWHEEL:
				mouseEvent.type = MOUSE_WHEEL;
				if (event -> wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
					mouseEvent.x = -event -> wheel.x;
					mouseEvent.y = -event -> wheel.y;
				} else {
					mouseEvent.x = event -> wheel.x;
					mouseEvent.y = event -> wheel.y;
				}
				mouseEvent.windowID = event -> wheel.windowID;
				break;

			default:
				return;
		}

		MouseEvent::Dispatch(&mouseEvent);
	}

	#if defined(ANDROID) || defined(IPHONE)
	void SDLApplication::ProcessSensorEvent(SDL_Event* event) {
		if (!SensorEvent::callback) return;

		if (event -> sensor.which == gyroscopeSensorID) {
			sensorEvent.type = SENSOR_GYROSCOPE;
			sensorEvent.id = event -> sensor.which;
			sensorEvent.x = event -> sensor.data[0];
			sensorEvent.y = event -> sensor.data[1];
			sensorEvent.z = event -> sensor.data[2];
			SensorEvent::Dispatch(&sensorEvent);
		} else if (event -> sensor.which == accelerometerSensorID) {
			sensorEvent.type = SENSOR_ACCELEROMETER;
			sensorEvent.id = event -> sensor.which;
			sensorEvent.x = event -> sensor.data[0];
			sensorEvent.y = event -> sensor.data[1];
			sensorEvent.z = event -> sensor.data[2];
			SensorEvent::Dispatch(&sensorEvent);
		}
	}
	#endif

	void SDLApplication::ProcessTextEvent(SDL_Event* event) {
		if (!TextEvent::callback) return;

		switch (event -> type) {
			case SDL_TEXTINPUT:
				textEvent.type = TEXT_INPUT;
				break;

			case SDL_TEXTEDITING:
				textEvent.type = TEXT_EDIT;
				textEvent.start = event -> edit.start;
				textEvent.length = event -> edit.length;
				break;

			case SDL_TEXTEDITING_EXT:
				textEvent.type = TEXT_EDIT;
				textEvent.start = event -> editExt.start;
				textEvent.length = event -> editExt.length;
				if (textEvent.text)
					free(textEvent.text);

				const char* extText = event -> editExt.text;
				size_t n = strlen(extText);
				textEvent.text = (vbyte*)malloc(n + 1);
				memcpy(textEvent.text, extText, n + 1);
				SDL_free(event -> editExt.text);

				textEvent.windowID = event -> editExt.windowID;
				TextEvent::Dispatch(&textEvent);
				return;
		}

		const size_t textLen = std::strlen(event -> text.text) + 1;
		vbyte* newText = static_cast<vbyte*>(std::realloc(textEvent.text, textLen));
		if (newText) {
			textEvent.text = newText;
			std::memcpy(textEvent.text, event -> text.text, textLen);
		} else {
			if (textEvent.text) {
				free(textEvent.text);
				textEvent.text = nullptr;
			}
		}

		textEvent.windowID = event -> text.windowID;
		TextEvent::Dispatch(&textEvent);
	}

	void SDLApplication::ProcessTouchEvent(SDL_Event* event) {
		if (!TouchEvent::callback) return;

		switch (event -> type) {
			case SDL_FINGERMOTION: touchEvent.type = TOUCH_MOVE; break;
			case SDL_FINGERDOWN:   touchEvent.type = TOUCH_START; break;
			case SDL_FINGERUP:     touchEvent.type = TOUCH_END; break;
		}

		touchEvent.x = event -> tfinger.x;
		touchEvent.y = event -> tfinger.y;
		touchEvent.id = event -> tfinger.fingerId;
		touchEvent.dx = event -> tfinger.dx;
		touchEvent.dy = event -> tfinger.dy;
		touchEvent.pressure = event -> tfinger.pressure;
		touchEvent.device = event -> tfinger.touchId;

		TouchEvent::Dispatch(&touchEvent);
	}

	void SDLApplication::ProcessWindowEvent(SDL_Event* event) {
		if (!WindowEvent::callback) return;

		switch (event -> window.event) {
			case SDL_WINDOWEVENT_SHOWN:        windowEvent.type = WINDOW_SHOW; break;
			case SDL_WINDOWEVENT_CLOSE:        windowEvent.type = WINDOW_CLOSE; break;
			case SDL_WINDOWEVENT_HIDDEN:       windowEvent.type = WINDOW_HIDE; break;
			case SDL_WINDOWEVENT_ENTER:        windowEvent.type = WINDOW_ENTER; break;
			case SDL_WINDOWEVENT_FOCUS_GAINED: windowEvent.type = WINDOW_FOCUS_IN; break;
			case SDL_WINDOWEVENT_FOCUS_LOST:   windowEvent.type = WINDOW_FOCUS_OUT; break;
			case SDL_WINDOWEVENT_LEAVE:        windowEvent.type = WINDOW_LEAVE; break;
			case SDL_WINDOWEVENT_MAXIMIZED:    windowEvent.type = WINDOW_MAXIMIZE; break;
			case SDL_WINDOWEVENT_MINIMIZED:    windowEvent.type = WINDOW_MINIMIZE; break;
			case SDL_WINDOWEVENT_EXPOSED:      windowEvent.type = WINDOW_EXPOSE; break;
			case SDL_WINDOWEVENT_RESTORED:     windowEvent.type = WINDOW_RESTORE; break;

			case SDL_WINDOWEVENT_MOVED:
				windowEvent.type = WINDOW_MOVE;
				windowEvent.x = event -> window.data1;
				windowEvent.y = event -> window.data2;
				break;

			case SDL_WINDOWEVENT_SIZE_CHANGED:
				windowEvent.type = WINDOW_RESIZE;
				windowEvent.width = event -> window.data1;
				windowEvent.height = event -> window.data2;
				break;
		}

		windowEvent.windowID = event -> window.windowID;
		WindowEvent::Dispatch(&windowEvent);
	}

	int SDLApplication::Quit() {
		applicationEvent.type = EXIT;
		ApplicationEvent::Dispatch(&applicationEvent);

		#ifdef LIME_SDL_SOUND
		Sound_Quit();
		#endif

		SDL_QuitSubSystem(initFlags);
		SDL_Quit();
		return 0;
	}

	void SDLApplication::RegisterWindow(SDLWindow* window) {
		#ifdef IPHONE
		SDL_iPhoneSetAnimationCallback(window -> sdlWindow, 1, UpdateFrame, nullptr);
		#endif
	}

	void SDLApplication::SetFrameRate(double frameRate) {
		if (frameRate > 0)
			framePeriod = 1000.0 / frameRate;
		else
			framePeriod = 0.0;
	}

	bool SDLApplication::Update() {
		SDL_Event event;

		if (framePeriod <= 0.0) {
			if (SDL_WaitEvent(&event)) {
				HandleEvent(&event);
				while (active && SDL_PollEvent(&event)) {
					HandleEvent(&event);
				}
			}

			if (active) {
				lastUpdate = currentUpdate;
				currentUpdate = GetTimeMs();
				PushUpdate();
			}
			return active;
		}

		const double now = GetTimeMs();

		if (firstFrame) {
			firstFrame = false;
			nextFrameTime = now;
		}

		while (SDL_PollEvent(&event)) {
			HandleEvent(&event);
			if (!active) return false;
		}

		if (now < nextFrameTime) {
			const int timeout = static_cast<int>(nextFrameTime - now);
			if (timeout > 0) {
				if (SDL_WaitEventTimeout(&event, timeout)) {
					HandleEvent(&event);
					while (active && SDL_PollEvent(&event)) {
						HandleEvent(&event);
					}
				}
			}
			return active;
		}

		lastUpdate = currentUpdate;
		currentUpdate = now;
		PushUpdate();

		nextFrameTime += framePeriod;

		if (now - nextFrameTime > framePeriod * FRAME_TIME_CATCHUP_THRESHOLD) {
			nextFrameTime = now;
		}

		return active;
	}

	void SDLApplication::UpdateFrame() {
		if (currentApplication) {
			currentApplication -> Update();
		}
	}

	void SDLApplication::UpdateFrame(void*) {
		UpdateFrame();
	}

	Application* CreateApplication() {
		return new SDLApplication();
	}

}

#ifdef ANDROID
int SDL_main(int argc, char* argv[]) { return 0; }
#endif
#include "SDLApplication.h"
#include "SDLGamepad.h"
#include "SDLJoystick.h"
#include <system/System.h>

#ifdef HX_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef EMSCRIPTEN
#include "emscripten.h"
#endif


namespace lime {

	AutoGCRoot* Application::callback = 0;
	SDLApplication* SDLApplication::currentApplication = 0;

	const int ANALOG_AXIS_DEADZONE = 1000;
	std::map<int, std::map<int, int> > gamepadsAxisMap;
	bool inBackground = false;

	double performanceFrequency = 0.0;
	double performanceCounter = 0.0;

	#if defined(ANDROID) || defined (IPHONE)
	SDL_SensorID gyroscopeSensorID = -1;
	SDL_Sensor* gyroscopeSensor = nullptr;

	SDL_SensorID accelerometerSensorID = -1;
	SDL_Sensor* accelerometerSensor = nullptr;
	#endif

	SDLApplication::SDLApplication () {
		initFlags = SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER | SDL_INIT_JOYSTICK;
		#if defined(LIME_MOJOAL) || defined(LIME_OPENALSOFT)
		initFlags |= SDL_INIT_AUDIO;
		#endif

		#ifdef HX_WINDOWS
		// Set the DPI awareness of this app
		// printf("Setting DPI awareness \n");

		// This will make everything small and not blurry in Windows apps (Windows and HL) - without this
		// it will always treat everything like 1x DPI and will scale the contents of the window up
		// making it look blurry and ugly.
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		#endif

		if (SDL_Init (initFlags) != 0) {

			printf ("Could not initialize SDL: %s.\n", SDL_GetError ());

		}
		performanceFrequency = (double)SDL_GetPerformanceFrequency();
		performanceCounter = (double)SDL_GetPerformanceCounter();

		SDL_LogSetPriority (SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN);

		currentApplication = this;

		framePeriod = 0.0;
		firstFrame = true;
		currentUpdate = 0.0;
		lastUpdate = 0.0;
		nextFrameTime = 0.0;

		#if defined(ANDROID) || defined (IPHONE)
			SDL_EventState (SDL_SENSORUPDATE, SDL_ENABLE);
		#endif

			SDL_EventState (SDL_DROPFILE, SDL_ENABLE);

		#if defined(ANDROID) || defined (IPHONE)
			InitializeSensors ();
		#endif
			SDLJoystick::Init ();

		#ifdef HX_MACOS
			CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL (CFBundleGetMainBundle ());
			char path[PATH_MAX];

			if (CFURLGetFileSystemRepresentation (resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) {
				chdir (path);
			}

			CFRelease (resourcesURL);
		#endif

	}

	#if defined(ANDROID) || defined (IPHONE)
	void SDLApplication::InitializeSensors () {

		gyroscopeSensorID = System::GetFirstGyroscopeSensorId ();

		if (gyroscopeSensorID > 0)
			gyroscopeSensor = SDL_SensorOpen (gyroscopeSensorID);

		accelerometerSensorID = System::GetFirstAccelerometerSensorId ();

		if (accelerometerSensorID > 0)
			accelerometerSensor = SDL_SensorOpen (accelerometerSensorID);

	}
	#endif

	SDLApplication::~SDLApplication () {

		#if defined(ANDROID) || defined(IPHONE)
		if (gyroscopeSensor) {

			SDL_SensorClose (gyroscopeSensor);
			gyroscopeSensor = nullptr;
			gyroscopeSensorID = -1;

		}

		if (accelerometerSensor) {

			SDL_SensorClose (accelerometerSensor);
			accelerometerSensor = nullptr;
			accelerometerSensorID = -1;

		}
		#endif

	}


	int SDLApplication::Exec () {

		Init ();

		#ifdef EMSCRIPTEN
		emscripten_cancel_main_loop ();
		emscripten_set_main_loop (UpdateFrame, 0, 0);
		emscripten_set_main_loop_timing (EM_TIMING_RAF, 1);
		#endif

		#if defined(IPHONE) || defined(EMSCRIPTEN)

		return 0;

		#else

		while (active) {

			Update ();

		}

		return Quit ();

		#endif

	}


	double GetTimeMs() {
		const double counter = static_cast<double>(SDL_GetPerformanceCounter()) - performanceCounter;
		return (counter / performanceFrequency) * 1000.0;
	}


	void SDLApplication::HandleEvent (SDL_Event* event) {

		#if defined(IPHONE) || defined(EMSCRIPTEN)

		int top = 0;
		gc_set_top_of_stack(&top,false);

		#endif

		switch (event->type) {

			case SDL_USEREVENT:

				if (!inBackground) {
					applicationEvent.type = UPDATE;

					applicationEvent.deltaTime = currentUpdate - lastUpdate;

					ApplicationEvent::Dispatch (&applicationEvent);
					RenderEvent::Dispatch (&renderEvent);
				}

				break;

			case SDL_APP_WILLENTERBACKGROUND:

				inBackground = true;

				windowEvent.type = WINDOW_DEACTIVATE;
				WindowEvent::Dispatch (&windowEvent);
				break;

			case SDL_APP_WILLENTERFOREGROUND:

				break;

			case SDL_APP_DIDENTERFOREGROUND:

				windowEvent.type = WINDOW_ACTIVATE;
				WindowEvent::Dispatch (&windowEvent);

				inBackground = false;
				break;

			case SDL_CLIPBOARDUPDATE:

				ProcessClipboardEvent (event);
				break;

			case SDL_CONTROLLERAXISMOTION:
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:

				ProcessGamepadEvent (event);
				break;

			case SDL_DROPFILE:

				ProcessDropEvent (event);
				break;

			case SDL_FINGERMOTION:
			case SDL_FINGERDOWN:
			case SDL_FINGERUP:

				ProcessTouchEvent (event);
				break;

			#if defined(ANDROID) || defined (IPHONE)
			case SDL_SENSORUPDATE:

				ProcessSensorEvent (event);
				break;

			#endif

			case SDL_JOYAXISMOTION:

				ProcessJoystickEvent (event);
				break;

			case SDL_JOYBALLMOTION:
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYHATMOTION:
			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED:

				ProcessJoystickEvent (event);
				break;

			case SDL_KEYDOWN:
			case SDL_KEYUP:

				ProcessKeyEvent (event);
				break;

			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEWHEEL:

				ProcessMouseEvent (event);
				break;

			#ifndef EMSCRIPTEN
			case SDL_RENDER_DEVICE_RESET:

				renderEvent.type = RENDER_CONTEXT_LOST;
				RenderEvent::Dispatch (&renderEvent);

				renderEvent.type = RENDER_CONTEXT_RESTORED;
				RenderEvent::Dispatch (&renderEvent);

				renderEvent.type = RENDER;
				break;
			#endif

			case SDL_TEXTINPUT:
			case SDL_TEXTEDITING:

				ProcessTextEvent (event);
				break;

			case SDL_WINDOWEVENT:

				switch (event->window.event) {

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

						ProcessWindowEvent (event);
						break;

					case SDL_WINDOWEVENT_EXPOSED:

						ProcessWindowEvent (event);

						if (!inBackground) {

							RenderEvent::Dispatch (&renderEvent);

						}

						break;

					case SDL_WINDOWEVENT_SIZE_CHANGED:

						ProcessWindowEvent (event);

						if (!inBackground) {

							RenderEvent::Dispatch (&renderEvent);

						}

						break;

					case SDL_WINDOWEVENT_CLOSE:

						ProcessWindowEvent (event);

						// Avoid handling SDL_QUIT if in response to window.close
						SDL_Event event;

						if (SDL_PollEvent (&event)) {

							if (event.type != SDL_QUIT) {

								HandleEvent (&event);

							}

						}
						break;

				}

				break;

			case SDL_QUIT:

				active = false;
				break;

		}

	}


	void SDLApplication::Init () {
		active = true;

		const double now = GetTimeMs();
		lastUpdate = now;
		currentUpdate = now;
	}


	void SDLApplication::ProcessClipboardEvent (SDL_Event* event) {

		if (ClipboardEvent::callback) {

			clipboardEvent.type = CLIPBOARD_UPDATE;

			ClipboardEvent::Dispatch (&clipboardEvent);

		}

	}


	void SDLApplication::ProcessDropEvent (SDL_Event* event) {

		if (DropEvent::callback) {

			dropEvent.type = DROP_FILE;
			dropEvent.file = (vbyte*)event->drop.file;

			DropEvent::Dispatch (&dropEvent);
			SDL_free (dropEvent.file);

		}

	}


	void SDLApplication::ProcessGamepadEvent (SDL_Event* event) {

		if (GamepadEvent::callback) {

			switch (event->type) {

				case SDL_CONTROLLERAXISMOTION: {
					auto& axisMap = gamepadsAxisMap[event->caxis.which];
					auto& storedValue = axisMap[event->caxis.axis];

					if (storedValue == event->caxis.value)
						break;

					gamepadEvent.type = GAMEPAD_AXIS_MOVE;
					gamepadEvent.axis = event->caxis.axis;
					gamepadEvent.id   = event->caxis.which;

					const int value = event->caxis.value;

					if (value > -ANALOG_AXIS_DEADZONE && value < ANALOG_AXIS_DEADZONE) {
						if (storedValue != 0) {
							storedValue = 0;
							gamepadEvent.axisValue = 0.0;
							GamepadEvent::Dispatch(&gamepadEvent);
						}
						break;
					}

					storedValue = value;
					gamepadEvent.axisValue = value / (value > 0 ? 32767.0 : 32768.0);
					GamepadEvent::Dispatch(&gamepadEvent);
					break;
				}

				case SDL_CONTROLLERBUTTONDOWN:
					gamepadEvent.type = GAMEPAD_BUTTON_DOWN;
					gamepadEvent.button = event->cbutton.button;
					gamepadEvent.id = event->cbutton.which;
					GamepadEvent::Dispatch (&gamepadEvent);
					break;

				case SDL_CONTROLLERBUTTONUP:
					gamepadEvent.type = GAMEPAD_BUTTON_UP;
					gamepadEvent.button = event->cbutton.button;
					gamepadEvent.id = event->cbutton.which;
					GamepadEvent::Dispatch (&gamepadEvent);
					break;

				case SDL_CONTROLLERDEVICEADDED:

					if (SDLGamepad::Connect (event->cdevice.which)) {

						gamepadEvent.type = GAMEPAD_CONNECT;
						gamepadEvent.id = SDLGamepad::GetInstanceID (event->cdevice.which);
						GamepadEvent::Dispatch (&gamepadEvent);

					}

					break;

				case SDL_CONTROLLERDEVICEREMOVED:
					gamepadEvent.type = GAMEPAD_DISCONNECT;
					gamepadEvent.id = event->cdevice.which;
					GamepadEvent::Dispatch (&gamepadEvent);
					SDLGamepad::Disconnect (event->cdevice.which);
					gamepadsAxisMap.erase (event->cdevice.which);
					break;

			}

		}

	}


	void SDLApplication::ProcessJoystickEvent (SDL_Event* event) {

		if (JoystickEvent::callback) {

			switch (event->type) {

				case SDL_JOYAXISMOTION:

					if (!SDLJoystick::IsAccelerometer (event->jaxis.which)) {

						joystickEvent.type = JOYSTICK_AXIS_MOVE;
						joystickEvent.index = event->jaxis.axis;
						joystickEvent.x = event->jaxis.value / (event->jaxis.value > 0 ? 32767.0 : 32768.0);
						joystickEvent.id = event->jaxis.which;

						JoystickEvent::Dispatch (&joystickEvent);

					}
					break;

				case SDL_JOYBALLMOTION:

					if (!SDLJoystick::IsAccelerometer (event->jball.which)) {

						joystickEvent.type = JOYSTICK_TRACKBALL_MOVE;
						joystickEvent.index = event->jball.ball;
						joystickEvent.x = event->jball.xrel / (event->jball.xrel > 0 ? 32767.0 : 32768.0);
						joystickEvent.y = event->jball.yrel / (event->jball.yrel > 0 ? 32767.0 : 32768.0);
						joystickEvent.id = event->jball.which;

						JoystickEvent::Dispatch (&joystickEvent);

					}
					break;

				case SDL_JOYBUTTONDOWN:

					if (!SDLJoystick::IsAccelerometer (event->jbutton.which)) {

						joystickEvent.type = JOYSTICK_BUTTON_DOWN;
						joystickEvent.index = event->jbutton.button;
						joystickEvent.id = event->jbutton.which;

						JoystickEvent::Dispatch (&joystickEvent);

					}
					break;

				case SDL_JOYBUTTONUP:

					if (!SDLJoystick::IsAccelerometer (event->jbutton.which)) {

						joystickEvent.type = JOYSTICK_BUTTON_UP;
						joystickEvent.index = event->jbutton.button;
						joystickEvent.id = event->jbutton.which;

						JoystickEvent::Dispatch (&joystickEvent);

					}
					break;

				case SDL_JOYHATMOTION:

					if (!SDLJoystick::IsAccelerometer (event->jhat.which)) {

						joystickEvent.type = JOYSTICK_HAT_MOVE;
						joystickEvent.index = event->jhat.hat;
						joystickEvent.eventValue = event->jhat.value;
						joystickEvent.id = event->jhat.which;

						JoystickEvent::Dispatch (&joystickEvent);

					}
					break;

				case SDL_JOYDEVICEADDED:

					if (SDLJoystick::Connect (event->jdevice.which)) {

						joystickEvent.type = JOYSTICK_CONNECT;
						joystickEvent.id = SDLJoystick::GetInstanceID (event->jdevice.which);

						JoystickEvent::Dispatch (&joystickEvent);

					}
					break;

				case SDL_JOYDEVICEREMOVED:

					if (!SDLJoystick::IsAccelerometer (event->jdevice.which)) {

						joystickEvent.type = JOYSTICK_DISCONNECT;
						joystickEvent.id = event->jdevice.which;

						JoystickEvent::Dispatch (&joystickEvent);
						SDLJoystick::Disconnect (event->jdevice.which);

					}
					break;

			}

		}

	}


	void SDLApplication::ProcessKeyEvent (SDL_Event* event) {

		if (KeyEvent::callback) {

			switch (event->type) {

				case SDL_KEYDOWN: keyEvent.type = KEY_DOWN; break;
				case SDL_KEYUP: keyEvent.type = KEY_UP; break;

			}

			keyEvent.keyCode = event->key.keysym.sym;
			keyEvent.modifier = event->key.keysym.mod;
			keyEvent.windowID = event->key.windowID;

			if (keyEvent.type == KEY_DOWN) {

				// fixes warn C5055
				const SDL_Keycode code = static_cast<SDL_Keycode>(keyEvent.keyCode);

				if (code == SDLK_CAPSLOCK)      keyEvent.modifier |= KMOD_CAPS;
				if (code == SDLK_LALT)          keyEvent.modifier |= KMOD_LALT;
				if (code == SDLK_LCTRL)         keyEvent.modifier |= KMOD_LCTRL;
				if (code == SDLK_LGUI)         keyEvent.modifier |= KMOD_LGUI;
				if (code == SDLK_LSHIFT)        keyEvent.modifier |= KMOD_LSHIFT;
				if (code == SDLK_MODE)          keyEvent.modifier |= KMOD_MODE;
				if (code == SDLK_NUMLOCKCLEAR)  keyEvent.modifier |= KMOD_NUM;
				if (code == SDLK_RALT)          keyEvent.modifier |= KMOD_RALT;
				if (code == SDLK_RCTRL)         keyEvent.modifier |= KMOD_RCTRL;
				if (code == SDLK_RGUI)          keyEvent.modifier |= KMOD_RGUI;
				if (code == SDLK_RSHIFT)        keyEvent.modifier |= KMOD_RSHIFT;
			}

			KeyEvent::Dispatch (&keyEvent);

		}

	}


	void SDLApplication::ProcessMouseEvent (SDL_Event* event) {

		if (!MouseEvent::callback)
			return;

		switch (event->type) {

			case SDL_MOUSEMOTION:
				mouseEvent.type = MOUSE_MOVE;
				mouseEvent.x = event->motion.x;
				mouseEvent.y = event->motion.y;
				mouseEvent.movementX = event->motion.xrel;
				mouseEvent.movementY = event->motion.yrel;
				mouseEvent.windowID = event->motion.windowID;
				break;

			case SDL_MOUSEBUTTONDOWN:
				SDL_CaptureMouse (SDL_TRUE);

				mouseEvent.type = MOUSE_DOWN;
				mouseEvent.button = event->button.button - 1;
				mouseEvent.x = event->button.x;
				mouseEvent.y = event->button.y;
				mouseEvent.clickCount = event->button.clicks;
				mouseEvent.windowID = event->button.windowID;
				break;

			case SDL_MOUSEBUTTONUP:

				SDL_CaptureMouse (SDL_FALSE);

				mouseEvent.type = MOUSE_UP;
				mouseEvent.button = event->button.button - 1;
				mouseEvent.x = event->button.x;
				mouseEvent.y = event->button.y;
				mouseEvent.clickCount = event->button.clicks;
				mouseEvent.windowID = event->button.windowID;
				break;

			case SDL_MOUSEWHEEL:

				mouseEvent.type = MOUSE_WHEEL;

				if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
					mouseEvent.x = -event->wheel.x;
					mouseEvent.y = -event->wheel.y;
				} else {
					mouseEvent.x = event->wheel.x;
					mouseEvent.y = event->wheel.y;
				}

				mouseEvent.windowID = event->wheel.windowID;
				break;

			default:
				return;

		}

		MouseEvent::Dispatch (&mouseEvent);

	}


	#if defined(ANDROID) || defined (IPHONE)
	void SDLApplication::ProcessSensorEvent(SDL_Event* event) {

		if (SensorEvent::callback) {

			if (event->sensor.which == gyroscopeSensorID) {

				sensorEvent.type = SENSOR_GYROSCOPE;
				sensorEvent.id = event->sensor.which;
				sensorEvent.x = event->sensor.data[0];
				sensorEvent.y = event->sensor.data[1];
				sensorEvent.z = event->sensor.data[2];
				SensorEvent::Dispatch(&sensorEvent);

			} else if (event->sensor.which == accelerometerSensorID) {

				sensorEvent.type = SENSOR_ACCELEROMETER;
				sensorEvent.id = event->sensor.which;
				sensorEvent.x = event->sensor.data[0];
				sensorEvent.y = event->sensor.data[1];
				sensorEvent.z = event->sensor.data[2];
				SensorEvent::Dispatch(&sensorEvent);

			}

		}

	}
	#endif


	void SDLApplication::ProcessTextEvent (SDL_Event* event) {

		if (TextEvent::callback) {

			switch (event->type) {

				case SDL_TEXTINPUT:

					textEvent.type = TEXT_INPUT;
					break;

				case SDL_TEXTEDITING:

					textEvent.type = TEXT_EDIT;
					textEvent.start = event->edit.start;
					textEvent.length = event->edit.length;
					break;

			}

			if (textEvent.text) {

				free (textEvent.text);

			}

			textEvent.text = (vbyte*)malloc (strlen (event->text.text) + 1);
			strcpy ((char*)textEvent.text, event->text.text);

			textEvent.windowID = event->text.windowID;
			TextEvent::Dispatch (&textEvent);

		}

	}


	void SDLApplication::ProcessTouchEvent (SDL_Event* event) {

		if (TouchEvent::callback) {

			switch (event->type) {

				case SDL_FINGERMOTION:

					touchEvent.type = TOUCH_MOVE;
					break;

				case SDL_FINGERDOWN:

					touchEvent.type = TOUCH_START;
					break;

				case SDL_FINGERUP:

					touchEvent.type = TOUCH_END;
					break;

			}

			touchEvent.x = event->tfinger.x;
			touchEvent.y = event->tfinger.y;
			touchEvent.id = event->tfinger.fingerId;
			touchEvent.dx = event->tfinger.dx;
			touchEvent.dy = event->tfinger.dy;
			touchEvent.pressure = event->tfinger.pressure;
			touchEvent.device = event->tfinger.touchId;

			TouchEvent::Dispatch (&touchEvent);

		}

	}


	void SDLApplication::ProcessWindowEvent (SDL_Event* event) {

		if (WindowEvent::callback) {

			switch (event->window.event) {

				case SDL_WINDOWEVENT_SHOWN: windowEvent.type = WINDOW_SHOW; break;
				case SDL_WINDOWEVENT_CLOSE: windowEvent.type = WINDOW_CLOSE; break;
				case SDL_WINDOWEVENT_HIDDEN: windowEvent.type = WINDOW_HIDE; break;
				case SDL_WINDOWEVENT_ENTER: windowEvent.type = WINDOW_ENTER; break;
				case SDL_WINDOWEVENT_FOCUS_GAINED: windowEvent.type = WINDOW_FOCUS_IN; break;
				case SDL_WINDOWEVENT_FOCUS_LOST: windowEvent.type = WINDOW_FOCUS_OUT; break;
				case SDL_WINDOWEVENT_LEAVE: windowEvent.type = WINDOW_LEAVE; break;
				case SDL_WINDOWEVENT_MAXIMIZED: windowEvent.type = WINDOW_MAXIMIZE; break;
				case SDL_WINDOWEVENT_MINIMIZED: windowEvent.type = WINDOW_MINIMIZE; break;
				case SDL_WINDOWEVENT_EXPOSED: windowEvent.type = WINDOW_EXPOSE; break;

				case SDL_WINDOWEVENT_MOVED:

					windowEvent.type = WINDOW_MOVE;
					windowEvent.x = event->window.data1;
					windowEvent.y = event->window.data2;
					break;

				case SDL_WINDOWEVENT_SIZE_CHANGED:

					windowEvent.type = WINDOW_RESIZE;
					windowEvent.width = event->window.data1;
					windowEvent.height = event->window.data2;
					break;

				case SDL_WINDOWEVENT_RESTORED: windowEvent.type = WINDOW_RESTORE; break;

			}

			windowEvent.windowID = event->window.windowID;
			WindowEvent::Dispatch (&windowEvent);

		}

	}


	int SDLApplication::Quit () {
		applicationEvent.type = EXIT;
		ApplicationEvent::Dispatch (&applicationEvent);

		SDL_QuitSubSystem (initFlags);

		SDL_Quit ();
		return 0;

	}


	void SDLApplication::RegisterWindow (SDLWindow *window) {

		#ifdef IPHONE
		SDL_iPhoneSetAnimationCallback (window->sdlWindow, 1, UpdateFrame, NULL);
		#endif

	}


	void SDLApplication::SetFrameRate (double frameRate) {

		if (frameRate > 0) {

			framePeriod = 1000.0 / frameRate;

		} else {

			framePeriod = 0.0;

		}

	}

	void PushUpdate(void) {
		SDL_Event event {};
		SDL_UserEvent userevent {};

		userevent.type = SDL_USEREVENT;
		userevent.code = 0;
		userevent.data1 = nullptr;
		userevent.data2 = nullptr;

		event.type = SDL_USEREVENT;
		event.user = userevent;

		SDL_PushEvent (&event);
	}


	bool SDLApplication::Update () {
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
			if (timeout > 0 && SDL_WaitEventTimeout(&event, timeout)) {
				HandleEvent(&event);
				while (active && SDL_PollEvent(&event)) {
					HandleEvent(&event);
				}
			}
			return active;
		}

		lastUpdate = currentUpdate;
		currentUpdate = now;
		PushUpdate();

		nextFrameTime += framePeriod;
		if (now - nextFrameTime > framePeriod * 2.0)
			nextFrameTime = now;

		return active;
	}


	void SDLApplication::UpdateFrame () {
		currentApplication->Update ();
	}


	void SDLApplication::UpdateFrame (void*) {

		UpdateFrame ();

	}

	Application* CreateApplication () {

		return new SDLApplication ();

	}


}


#ifdef ANDROID
int SDL_main (int argc, char *argv[]) { return 0; }
#endif

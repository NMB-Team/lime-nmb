#ifndef LIME_SDL_APPLICATION_H
#define LIME_SDL_APPLICATION_H

#include <SDL.h>
#include <app/Application.h>
#include <app/ApplicationEvent.h>
#include <graphics/RenderEvent.h>
#include <system/ClipboardEvent.h>
#include <system/SensorEvent.h>
#include <ui/DropEvent.h>
#include <ui/GamepadEvent.h>
#include <ui/JoystickEvent.h>
#include <ui/KeyEvent.h>
#include <ui/MouseEvent.h>
#include <ui/TextEvent.h>
#include <ui/TouchEvent.h>
#include <ui/GestureEvent.h>
#include <ui/Gesture.h>
#include <ui/WindowEvent.h>
#include "SDLWindow.h"

namespace lime {

	class SDLApplication : public Application {
		public:
			SDLApplication();
			~SDLApplication();

			virtual int Exec() override;
			virtual void Init() override;
			virtual int Quit() override;
			virtual void SetFrameRate(double frameRate) override;
			virtual bool Update() override;

			void RegisterWindow(SDLWindow* window);

		private:
			void HandleEvent(SDL_Event* event);
			void HandleWindowEvent(SDL_Event* event);
			void ProcessClipboardEvent(SDL_Event* event);
			void ProcessDropEvent(SDL_Event* event);
			void ProcessGamepadEvent(SDL_Event* event);
			void ProcessJoystickEvent(SDL_Event* event);
			void ProcessKeyEvent(SDL_Event* event);
			void ProcessMouseEvent(SDL_Event* event);

			#ifdef HX_MACOS
			void InitializeGesture(SDL_Window *window);
			#endif

			#if defined(ANDROID) || defined(IPHONE)
			void InitializeSensors();
			void ProcessSensorEvent(SDL_Event* event);
			#endif

			void ProcessTextEvent(SDL_Event* event);
			void ProcessTouchEvent(SDL_Event* event);
			void ProcessWindowEvent(SDL_Event* event);
			void ProcessGestureEvent(SDL_Event* event);

			static void UpdateFrame();
			static void UpdateFrame(void*);

			static SDLApplication* currentApplication;

			bool active;
			bool firstFrame;
			Uint32 initFlags;

			double framePeriod;
			double currentUpdate;
			double lastUpdate;
			double nextFrameTime;

			ApplicationEvent applicationEvent;
			ClipboardEvent clipboardEvent;
			DropEvent dropEvent;
			GamepadEvent gamepadEvent;
			JoystickEvent joystickEvent;
			KeyEvent keyEvent;
			MouseEvent mouseEvent;
			RenderEvent renderEvent;
			SensorEvent sensorEvent;
			TextEvent textEvent;
			TouchEvent touchEvent;
			GestureEvent gestureEvent;
			WindowEvent windowEvent;
	};

}

#endif
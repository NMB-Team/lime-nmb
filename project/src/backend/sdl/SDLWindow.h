#ifndef LIME_SDL_WINDOW_H
#define LIME_SDL_WINDOW_H

#include <SDL_syswm.h>
#include <SDL.h>
#include <graphics/ImageBuffer.h>
#include <ui/Cursor.h>
#include <ui/Window.h>

#if defined (LIME_ANGLE) && defined (IPHONE)
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
#endif

#include <vector>

namespace lime {

	class SDLWindow : public Window {
		public:
			SDLWindow(Application* application, int width, int height, int flags, const char* title);
			~SDLWindow();

			virtual void Alert(const char* message, const char* title) override;
			virtual void Close() override;
			virtual void Focus() override;
			virtual void Move(int x, int y) override;
			virtual void Resize(int width, int height) override;

			virtual void ContextFlip() override;
			virtual void* ContextLock(bool useCFFIValue) override;
			virtual void ContextMakeCurrent() override;
			virtual void ContextUnlock() override;

			virtual void* GetHandle() override;
			virtual void* GetContext() override;
			virtual const char* GetContextType() override;
			virtual int GetDisplay() override;
			virtual void GetDisplayMode(DisplayMode* displayMode) override;
			virtual int GetHeight() override;
			virtual uint32_t GetID() override;
			virtual bool GetMouseLock() override;
			virtual float GetOpacity() override;
			virtual double GetScale() override;
			virtual bool GetTextInputEnabled() override;
			virtual int GetWidth() override;
			virtual int GetX() override;
			virtual int GetY() override;
			virtual void GetMousePosition(int* x, int* y) override;

			virtual void SetMinimumSize(int width, int height) override;
			virtual void SetMaximumSize(int width, int height) override;
			virtual bool SetBorderless(bool borderless) override;
			virtual void SetCursor(Cursor cursor) override;
			virtual void SetCursorDirectly(void* cursor) override;
			virtual void SetDisplayMode(DisplayMode* displayMode) override;
			virtual bool SetFullscreen(bool fullscreen) override;
			virtual bool SetFullscreenExclusiveMode(bool exclusive) override;
			virtual void SetIcon(ImageBuffer* imageBuffer) override;
			virtual bool SetMaximized(bool maximized) override;
			virtual bool SetMinimized(bool minimized) override;
			virtual void SetMouseLock(bool mouseLock) override;
			virtual void SetOpacity(float opacity) override;
			virtual bool SetResizable(bool resizable) override;
			virtual void SetTextInputEnabled(bool enabled) override;
			virtual void SetTextInputRect(Rectangle* rect) override;
			virtual const char* SetTitle(const char* title) override;
			virtual bool SetVisible(bool visible) override;
			virtual bool SetAlwaysOnTop(bool alwaysOnTop) override;
			virtual bool SetVSync(bool vsync) override;
			virtual void WarpMouse(int x, int y) override;

			virtual void ReadPixels(ImageBuffer* buffer, Rectangle* rect) override;

			SDL_Renderer* sdlRenderer;
			SDL_Texture* sdlTexture;
			SDL_Window* sdlWindow;

		private:
			#if defined (LIME_ANGLE) && defined (IPHONE)
			std::vector<EGLAttrib> egl_display_attribs;
			std::vector<EGLint> egl_config_attribs;
			std::vector<EGLint> egl_context_attribs;

			SDL_MetalView eglMetalView;
			EGLDisplay eglDisplay;
			EGLContext eglContext;
			EGLSurface eglSurface;
			#else
			SDL_GLContext context;
			#endif

			int contextWidth;
			int contextHeight;
	};

}

#endif
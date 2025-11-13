#include "SDLCursor.h"

namespace lime {


	SDL_Cursor* SDLCursor::CreateImageCursor(ImageBuffer *imageBuffer, int hot_x, int hot_y) {

		SDL_Surface *surface = SDL_CreateRGBSurfaceFrom (imageBuffer->data->buffer->b, 	imageBuffer->width, imageBuffer->height, imageBuffer->bitsPerPixel, 	imageBuffer->Stride (), 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

		if (surface) {

			SDL_Cursor* cursor = SDL_CreateColorCursor(surface, hot_x, hot_y);
			SDL_FreeSurface (surface);
			return cursor;

		}

		return nullptr;

	}

}
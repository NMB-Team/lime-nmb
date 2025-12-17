#ifndef LIME_SDL_CURSOR_H
#define LIME_SDL_CURSOR_H


#include <SDL.h>
#include <graphics/ImageBuffer.h>


namespace lime {

	// Define some extra standard generic cursors. The standard ones lime/SDL comes with
	// are created automatically.

	// Would probably be better if these were in files separately.

	// Generate XPM format from a 2 color PNG and use the inertia.utils.CursorHelper.hx file to convert
	static const char *dragDrop[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		".                               ",
		"..                              ",
		".X.                             ",
		".XX.                            ",
		".XXX.                           ",
		".XXXX.                          ",
		".XXXXX.                         ",
		".XXXXXX.                        ",
		".XXXXXXX.                       ",
		".XXXXXXXX.                      ",
		".XXXXXXXXX.                     ",
		".XXXXXX....                     ",
		".XXX.XX.                        ",
		".XX..XX.                        ",
		".X.  .XX.                       ",
		"..   .XX.                       ",
		".     .XX.                      ",
		"      .XX.                      ",
		"   . . .XX.. . .                ",
		"    . ..XX. . .                 ",
		"   .    ..     .                ",
		"    .         .                 ",
		"   .           .                ",
		"    .         .                 ",
		"   .           .                ",
		"    . . . . . .                 ",
		"   . . . . . . .                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"0,0"
		};

	static const char *dragDropCopy[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		".                               ",
		"..                              ",
		".X.                             ",
		".XX.                            ",
		".XXX.                           ",
		".XXXX.                          ",
		".XXXXX.                         ",
		".XXXXXX.                        ",
		".XXXXXXX.                       ",
		".XXXXXXXX.                      ",
		".XXXXXXXXX.                     ",
		".XXXXXX...............          ",
		".XXX.XX.   .XXXXXXXXX.          ",
		".XX..XX.   .XXXXXXXXX.          ",
		".X.  .XX.  .XXXX.XXXX.          ",
		"..   .XX.  .XXXX.XXXX.          ",
		".     .XX. .XX.....XX.          ",
		"      .XX. .XXXX.XXXX.          ",
		"       .XX..XXXX.XXXX.          ",
		"       .XX..XXXXXXXXX.          ",
		"        .. .XXXXXXXXX.          ",
		"           ...........          ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"0,0"
		};


	static const char *dragDropMove[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		".                               ",
		"..                              ",
		".X.                             ",
		".XX.                            ",
		".XXX.                           ",
		".XXXX.                          ",
		".XXXXX.                         ",
		".XXXXXX.                        ",
		".XXXXXXX.                       ",
		".XXXXXXXX.                      ",
		".XXXXXXXXX.                     ",
		".XXXXXX...............          ",
		".XXX.XX.   .XXXXXXXXX.          ",
		".XX..XX.   .XXX....XX.          ",
		".X.  .XX.  .XXXX...XX.          ",
		"..   .XX.  .XXX....XX.          ",
		".     .XX. .XX...X.XX.          ",
		"      .XX. .XX..XXXXX.          ",
		"       .XX..XX.XXXXXX.          ",
		"       .XX..XXX.XXXXX.          ",
		"        .. .XXXXXXXXX.          ",
		"           ...........          ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"0,0"
		};

	static const char *dragDropNone[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		"      ......                    ",
		"    ..........                  ",
		"   ....    ....                 ",
		"  ...        ...                ",
		" .....        ...               ",
		" .. ...        ..               ",
		"...  ...       ...              ",
		"..    ...       ..              ",
		"..     ...      ..              ",
		"..      ...     ..              ",
		"..       ...    ..              ",
		"...       ...  ...              ",
		" ..        ... ..               ",
		" ...        .....               ",
		"  ...        ...                ",
		"   ....    ....                 ",
		"    ..........                  ",
		"      ......                    ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"9,9"
		};


	static const char *spinner[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		"      .          .              ",
		"     ..          ..             ",
		"    .X.  .       .X.            ",
		"   .XX.  ..      .XX.           ",
		"  .XXX.  .X.     .XXX.          ",
		"   .XX.  .XX.    .XX.           ",
		"    .X.  .XXX.   .X.            ",
		"     ..  .XXXX.  ..             ",
		"      .  .XXXXX. .              ",
		"         .XXXXXX.               ",
		"         .XXXXXXX.              ",
		"         .XXXXXXXX.             ",
		"         .XXXXXXXXX.            ",
		"         .XXXXXX....            ",
		"         .XXX.XX.               ",
		"         .XX..XX.               ",
		"         .X.  .XX.              ",
		"         ..   .XX.              ",
		"         .     .XX.             ",
		"               .XX.             ",
		"                .XX.            ",
		"                .XX.            ",
		"                 ..             ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"9,2"
		};


	static const char *resize_dope[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		"                                ",
		"     XX     XX                  ",
		"    X.X     X.X                 ",
		"   X..X     X..X                ",
		"  X...XXX XXX...X               ",
		" X.......X.......X              ",
		"  X...XXX XXX...X               ",
		"   X..X     X..X                ",
		"    X.X     X.X                 ",
		"     XX     XX                  ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"9,6"
		};

	static const char *trans_diag[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		"XXXXXX                          ",
		"X.....X                         ",
		"X....X                          ",
		"X...X                           ",
		"X..X.X  X                       ",
		"X.X X.XX.X                      ",
		" X   XX.X                       ",
		"     X.XX   X                   ",
		"    X.XX.X X.X                  ",
		"     X  X.X..X                  ",
		"         X...X                  ",
		"        X....X                  ",
		"       X.....X                  ",
		"        XXXXXX                  ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"7,8"
		};


	static const char *trans_horz[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		"   X         X                  ",
		"  X.X   X   X.X                 ",
		" X..XXXX.XXXX..X                ",
		"X......X.X......X               ",
		" X..XXXX.XXXX..X                ",
		"  X.X   X   X.X                 ",
		"   X         X                  ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"9,4"
		};


	static const char *trans_rotate[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		"                                ",
		"         XXXX                   ",
		"       XX....XX                 ",
		"      X..XXXX..X                ",
		"     X.XX    XX.X               ",
		"     X.X      X.X               ",
		"  XXX.XXX      X.X              ",
		"  X.....X      X.X              ",
		"   X...X       X.X              ",
		"    X.X        X.X              ",
		"     X        X.X               ",
		"             XX.X               ",
		"          XXX..X                ",
		"         X...XX                 ",
		"          XXX                   ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"11,8"
		};

	static const char *trans_vert[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		"   X                            ",
		"  X.X                           ",
		" X...X                          ",
		"X.....X                         ",
		" XX.XX                          ",
		"  X.X                           ",
		"  X.X                           ",
		"  XXX                           ",
		" X...X                          ",
		"  XXX                           ",
		"  X.X                           ",
		"  X.X                           ",
		" XX.XX                          ",
		"X.....X                         ",
		" X...X                          ",
		"  X.X                           ",
		"   X                            ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"4,9"
		};


	static const char *trans_move[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		"        X                       ",
		"       X.X                      ",
		"      X...X                     ",
		"     X.....X                    ",
		"      XX.XX                     ",
		"   X   X.X   X                  ",
		"  X.X  XXX  X.X                 ",
		" X..XXXX.XXXX..X                ",
		"X.....X...X.....X               ",
		" X..XXXX.XXXX..X                ",
		"  X.X  XXX  X.X                 ",
		"   X   X.X   X                  ",
		"      XX.XX                     ",
		"     X.....X                    ",
		"      X...X                     ",
		"       X.X                      ",
		"        X                       ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"8,8"
		};


	static const char *trans_diag2[] = {
		/* columns rows colors chars-per-pixel */
		"32 32 3 1 ",
		"  c None",
		". c black",
		"X c white",
		/* pixels */
		"        XXXXXX                  ",
		"       X.....X                  ",
		"        X....X                  ",
		"         X...X                  ",
		"     X  X.X..X                  ",
		"    X.XX.X X.X                  ",
		"     X.XX   X                   ",
		" X   XX.X                       ",
		"X.X X.XX.X                      ",
		"X..X.X  X                       ",
		"X...X                           ",
		"X....X                          ",
		"X.....X                         ",
		"XXXXXX                          ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"                                ",
		"7,7"
		};

	class SDLCursor {

		public:

			static SDL_Cursor* arrowCursor;
			static SDL_Cursor* crosshairCursor;
			static SDL_Cursor* moveCursor;
			static SDL_Cursor* pointerCursor;
			static SDL_Cursor* resizeNESWCursor;
			static SDL_Cursor* resizeNSCursor;
			static SDL_Cursor* resizeNWSECursor;
			static SDL_Cursor* resizeWECursor;
			static SDL_Cursor* textCursor;
			static SDL_Cursor* waitCursor;
			static SDL_Cursor* waitArrowCursor;

			static SDL_Cursor* spinnerCursor;
			static SDL_Cursor* dragDropCursor;
			static SDL_Cursor* dragDropCopyCursor;
			static SDL_Cursor* dragDropMoveCursor;
			static SDL_Cursor* dragDropNoneCursor;
			static SDL_Cursor* resize_dope;
			static SDL_Cursor* trans_diag;
			static SDL_Cursor* trans_horz;
			static SDL_Cursor* trans_vert;
			static SDL_Cursor* trans_rotate;
			static SDL_Cursor* trans_move;
			static SDL_Cursor* trans_diag2;

			static SDL_Cursor* CreateImageCursor(ImageBuffer *imageBuffer, int hot_x, int hot_y);

	};


}


#endif
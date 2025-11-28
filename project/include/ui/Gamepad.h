#ifndef LIME_UI_GAMEPAD_H
#define LIME_UI_GAMEPAD_H

#include <string>

namespace lime {


	class Gamepad {

		public:

			static void AddMapping (const char* content);
			static std::string GetDeviceGUID(int id);
			static std::string GetDeviceName(int id);

	};


}


#endif
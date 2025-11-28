#include "SDLGamepad.h"
#include <map>
#include <unordered_map>
#include <string>

namespace lime {


	static std::unordered_map<int, SDL_GameController*> gameControllers;
	static std::unordered_map<int, int> deviceToInstanceID;


	bool SDLGamepad::Connect (int deviceID) {

		if (!SDL_IsGameController(deviceID))
			return false;

		SDL_GameController* controller = SDL_GameControllerOpen(deviceID);
		if (!controller)
			return false;

		SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
		if (!joystick) {
			SDL_GameControllerClose(controller);
			return false;
		}

		int instanceID = SDL_JoystickInstanceID(joystick);
		if (instanceID < 0) {
			SDL_GameControllerClose(controller);
			return false;
		}

		auto existing = gameControllers.find(instanceID);
		if (existing != gameControllers.end()) {
			SDL_GameControllerClose(existing->second);
			gameControllers.erase(existing);
		}

		gameControllers[instanceID] = controller;
		deviceToInstanceID[deviceID] = instanceID;

		return true;

	}


	bool SDLGamepad::Disconnect (int instanceID) {

		auto it = gameControllers.find(instanceID);
		if (it == gameControllers.end())
			return false;

		SDL_GameController* controller = it->second;
		SDL_GameControllerClose(controller);
		gameControllers.erase(it);

		for (auto devIt = deviceToInstanceID.begin(); devIt != deviceToInstanceID.end(); ) {
			if (devIt->second == instanceID)
				devIt = deviceToInstanceID.erase(devIt);
			else
				++devIt;
		}

		return true;

	}


	int SDLGamepad::GetInstanceID (int deviceIndex) {

		auto it = deviceToInstanceID.find(deviceIndex);
		if (it == deviceToInstanceID.end())
			return -1;

		return it->second;

	}


	void Gamepad::AddMapping (const char* content) {

		int result = SDL_GameControllerAddMapping (content);
		(void)result;

	}


	std::string Gamepad::GetDeviceGUID (int instanceID) {

		auto it = gameControllers.find(instanceID);
		if (it == gameControllers.end())
			return {};

		SDL_Joystick* joystick = SDL_GameControllerGetJoystick(it->second);
		if (!joystick)
			return {};

		char guid[64] = {};
		SDL_JoystickGetGUIDString (SDL_JoystickGetGUID(joystick), guid, sizeof(guid));
		return std::string(guid);

	}


	std::string Gamepad::GetDeviceName (int instanceID) {

		auto it = gameControllers.find(instanceID);
		if (it == gameControllers.end())
			return {};

		const char* name = SDL_GameControllerName(it->second);
		return name ? std::string(name) : std::string{};

	}


}
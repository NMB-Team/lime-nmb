package lime.media;

import haxe.MainLoop;
import haxe.Timer;

import lime.system.CFFI;
import lime.system.CFFIPointer;
import lime.app.Application;
import lime.app.Event;

#if (js && html5)
import js.Browser;
#elseif lime_openal
#if sys
import haxe.io.Path;

import lime.system.System;

import sys.FileSystem;
import sys.io.File;
#end
import sys.thread.Deque;

import lime._internal.backend.native.NativeCFFI;
import lime.media.openal.AL;
import lime.media.openal.ALC;
import lime.media.openal.ALContext;
import lime.media.openal.ALDevice;
#end

#if !lime_debug
@:fileXml('tags="haxe,release"')
@:noDebug
#end
@:access(lime._internal.backend.native.NativeCFFI)
@:access(lime.media.openal.ALDevice)
class AudioManager {
	public static var onDefaultPlaybackDeviceChanged = new Event<String -> Void>();
	public static var onPlaybackDeviceAdded = new Event<String -> Void>();
	public static var onPlaybackDeviceRemoved = new Event<String -> Void>();
	public static var onDefaultCaptureDeviceChanged = new Event<String -> Void>();
	public static var onCaptureDeviceAdded = new Event<String -> Void>();
	public static var onCaptureDeviceRemoved = new Event<String -> Void>();

	public static var automaticDefaultPlaybackDevice:Bool = true;

	public static var context:AudioContext;

	@:noCompletion private static var __updateTimer:Timer;

	#if lime_openal
	@:noCompletion private static var __systemEventsSupported:Bool;
	@:noCompletion private static var __reopenDeviceSupported:Bool;

	@:noCompletion private static var __alRequestEvents:Deque<ALDeviceEvent> = new Deque();
	@:noCompletion private static var __alCurrentDevice:ALDevice;
	@:noCompletion private static var __alCurrentDeviceName:String;
	#end

	public static function init(context:AudioContext = null) {
		#if !lime_doc_gen
		if (AudioManager.context != null)
			return;

		if (context == null) {
			AudioManager.context = new AudioContext();
			context = AudioManager.context;

			#if lime_openal
			if (context.type == OPENAL) {
				__setupConfig();
				refresh();

				if (__reopenDeviceSupported)
					AL.disable(AL.STOP_SOURCES_ON_DISCONNECT_SOFT);
				if (__systemEventsSupported) {
					ALC.eventControlSOFT(3, [
						ALC.EVENT_TYPE_DEFAULT_DEVICE_CHANGED_SOFT,
						ALC.EVENT_TYPE_DEVICE_ADDED_SOFT,
						ALC.EVENT_TYPE_DEVICE_REMOVED_SOFT
					], true);
					ALC.eventCallbackSOFT(__alDeviceEventCallback);
				}
			}
			#end
		}

		#if lime_openal
		if (__updateTimer == null) {
			__updateTimer = new Timer(100);
			__updateTimer.run = __update;
		}
		#end
		#end
	}

	public static function refresh(?deviceName:String):Bool {
		#if (lime_openal && !lime_doc_gen)
		if (context == null || context.type != OPENAL)
			return false;

		var currentContext = ALC.getCurrentContext();
		var device = currentContext != null ? ALC.getContextsDevice(currentContext) : null;

		if (device != null && __reopenDeviceSupported && ALC.reopenDeviceSOFT(device, deviceName, null)) {
			__refresh();
			return true;
		}

		if (currentContext != null) {
			ALC.destroyContext(currentContext);
			currentContext = null;
		}

		if (device != null) {
			ALC.closeDevice(device);
			device = null;
		}

		if ((device = ALC.openDevice()) == null
			|| (currentContext = ALC.createContext(device)) == null || !ALC.makeContextCurrent(currentContext)) {
			return false;
		}

		ALC.processContext(currentContext);
		__refresh();
		return true;
		#else
		return false;
		#end
	}

	public static function getPlaybackDefaultDeviceName():String {
		#if (lime_openal && !lime_doc_gen)
		if (context == null || context.type != OPENAL)
			return "";

		return __formatDeviceName(ALC.getString(null, ALC.DEFAULT_ALL_DEVICES_SPECIFIER));
		#else
		return "";
		#end
	}

	public static function getPlaybackDeviceNames():Array<String> {
		#if (lime_openal && !lime_doc_gen)
		if (context == null || context.type != OPENAL)
			return [];

		final arr = ALC.getStrings(null, ALC.ALL_DEVICES_SPECIFIER);
		for (i in 0...arr.length)
			arr[i] = __formatDeviceName(arr[i]);
		return arr;
		#else
		return [];
		#end
	}

	public static function getCaptureDefaultDeviceName():String {
		#if (lime_openal && !lime_doc_gen)
		if (context == null || context.type != OPENAL)
			return "";

		return __formatDeviceName(ALC.getString(null, ALC.CAPTURE_DEFAULT_DEVICE_SPECIFIER));
		#else
		return "";
		#end
	}

	public static function getCaptureDeviceNames():Array<String> {
		#if (lime_openal && !lime_doc_gen)
		if (context == null || context.type != OPENAL)
			return [];

		final arr = ALC.getStrings(null, ALC.CAPTURE_DEVICE_SPECIFIER);
		for (i in 0...arr.length)
			arr[i] = __formatDeviceName(arr[i]);
		return arr;
		#else
		return [];
		#end
	}

	public static function resume():Void {
		#if (lime_openal && !lime_doc_gen)
		if (context == null || context.type != OPENAL)
			return;

		var currentContext = ALC.getCurrentContext();
		if (currentContext == null)
			return;

		var device = ALC.getContextsDevice(currentContext);
		if (device != null)
			ALC.resumeDevice(device);

		ALC.processContext(currentContext);
		#end
	}

	public static function shutdown():Void {
		#if (lime_openal && !lime_doc_gen)
		if (context == null || context.type != OPENAL)
			return;

		var currentContext = ALC.getCurrentContext();
		if (currentContext == null)
			return;

		ALC.makeContextCurrent(null);
		ALC.destroyContext(currentContext);

		var device = ALC.getContextsDevice(currentContext);
		if (device != null)
			ALC.closeDevice(device);
		#end

		context = null;
	}

	public static function suspend():Void {
		#if (lime_openal && !lime_doc_gen)
		if (context == null || context.type != OPENAL)
			return;

		var currentContext = ALC.getCurrentContext();
		if (currentContext == null)
			return;

		ALC.suspendContext(currentContext);

		var device = ALC.getContextsDevice(currentContext);
		if (device != null)
			ALC.pauseDevice(device);
		#end
	}

	#if (lime_openal && !lime_doc_gen)
	@:noCompletion private static function __setupConfig():Void {
		#if sys
		final alConfig:Array<String> = [];

		alConfig.push("[General]");
		alConfig.push("sample-type=float32");
		alConfig.push("channels=stereo");
		alConfig.push("hrtf=false");
		alConfig.push("cf_level=0");
		alConfig.push("output-limiter=false");
		alConfig.push("front-stablizer=false");
		alConfig.push("volume-adjust=0");
		alConfig.push("period_size=441");
		alConfig.push("sources=512");
		alConfig.push("sends=64");
		alConfig.push("dither=false");

		alConfig.push("[decoder]");
		alConfig.push("hq-mode=true");
		alConfig.push("distance-comp=false");
		alConfig.push("nfc=false");

		try {
			final directory:String = Path.directory(Path.withoutExtension(System.applicationStorageDirectory));
			final path:String = Path.join([directory, #if windows "audio-config.ini" #else "audio-config.conf" #end]);
			final content:String = alConfig.join('\n');

			if (!FileSystem.exists(directory))
				FileSystem.createDirectory(directory);

			if (!FileSystem.exists(path))
				File.saveContent(path, content);

			Sys.putEnv("ALSOFT_CONF", path);
		} catch (e:Dynamic) {}
		#end
	}

	@:noCompletion private static function __update():Void {
		#if !macro
		NativeCFFI.lime_al_cleanup();
		#end

		var request;
		while ((request = __alRequestEvents.pop(false)) != null)
			__deviceEventCallback(request.eventType, request.deviceType, request.device, request.deviceName, request.message);
	}

	// device is null... and its actually intended cuz its tied to the device its being used rn.
	// why
	// and in ALC.EVENT_TYPE_DEFAULT_DEVICE_CHANGED_SOFT, deviceName is the device GUID
	@:noCompletion private static function __deviceEventCallback(eventType:Int, deviceType:Int, device:ALDevice, deviceName:String, message:String) {
		var currentContext = ALC.getCurrentContext();
		var currentDevice = currentContext != null ? ALC.getContextsDevice(currentContext) : null;
		if (deviceType == ALC.PLAYBACK_DEVICE_SOFT) {
			switch (eventType) {
				case ALC.EVENT_TYPE_DEFAULT_DEVICE_CHANGED_SOFT:
					if (automaticDefaultPlaybackDevice)
						refresh();
					onDefaultPlaybackDeviceChanged.dispatch(getPlaybackDefaultDeviceName());
				case ALC.EVENT_TYPE_DEVICE_ADDED_SOFT:
					onPlaybackDeviceAdded.dispatch(__formatDeviceName(deviceName));
				case ALC.EVENT_TYPE_DEVICE_REMOVED_SOFT:
					if (ALC.getIntegerv(currentDevice, ALC.CONNECTED, 1)[0] != 1)
						refresh();
					onPlaybackDeviceRemoved.dispatch(__formatDeviceName(deviceName));
			}
		} else if (deviceType == ALC.CAPTURE_DEVICE_SOFT) {
			switch (eventType) {
				case ALC.EVENT_TYPE_DEFAULT_DEVICE_CHANGED_SOFT:
					onDefaultCaptureDeviceChanged.dispatch(getCaptureDefaultDeviceName());
				case ALC.EVENT_TYPE_DEVICE_ADDED_SOFT:
					onCaptureDeviceAdded.dispatch(__formatDeviceName(deviceName));
				case ALC.EVENT_TYPE_DEVICE_REMOVED_SOFT:
					onCaptureDeviceRemoved.dispatch(__formatDeviceName(deviceName));
			}
		}
	}

	@:noCompletion private static function __refresh():Void {
		if (context == null || context.type != OPENAL)
			return;

		var currentContext = ALC.getCurrentContext();
		if (currentContext == null)
			return;

		var device = ALC.getContextsDevice(currentContext);
		if (device == null)
			return;

		__alCurrentDeviceName = __formatDeviceName(ALC.getString(device, ALC.ALL_DEVICES_SPECIFIER));

		#if hl // temporary fix
		__systemEventsSupported = false;
		__reopenDeviceSupported = false;
		#else
		__systemEventsSupported = ALC.isExtensionPresent(null, "ALC_SOFT_system_events");
		__reopenDeviceSupported = ALC.isExtensionPresent(null, "ALC_SOFT_reopen_device");
		#end
	}

	@:noCompletion private static function __formatDeviceName(deviceName:String) {
		if (StringTools.startsWith(deviceName, "OpenAL Soft on "))
			return deviceName.substr(15);
		else if (StringTools.startsWith(deviceName, "OpenAL on "))
			return deviceName.substr(10);
		else if (StringTools.startsWith(deviceName, "Generic Software on "))
			return deviceName.substr(20);
		else
			return deviceName;
	}

	// permanent band-aid fix whatever
	@:noCompletion private static function __getDeviceNameFromMessage(message:String):Null<String> {
		if (StringTools.startsWith(message, "Device removed: "))
			return message.substr(16);
		else if (StringTools.startsWith(message, "Device added: "))
			return message.substr(14);
		else
			return null;
	}

	@:noCompletion private static function __alDeviceEventCallback(eventType:Int, deviceType:Int, handle:CFFIPointer, #if hl _message:hl.Bytes #else message:String #end) {
		#if hl var message:String = CFFI.stringValue(_message); #end
		var device:ALDevice = handle != null ? new ALDevice(handle) : null;
		var deviceName = __getDeviceNameFromMessage(message);
		__alRequestEvents.add({
			eventType: eventType,
			deviceType: deviceType,
			device: device,
			deviceName: deviceName,
			message: message
		});
	}
	#end
}

#if (lime_openal && !lime_doc_gen)
private typedef ALDeviceEvent = {
	eventType:Int,
	deviceType:Int,
	device:Null<ALDevice>,
	deviceName:Null<String>,
	message:String
}
#end

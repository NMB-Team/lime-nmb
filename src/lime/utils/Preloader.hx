package lime.utils;

import haxe.ds.ObjectMap;
import haxe.io.Bytes;
import haxe.io.Path;
import haxe.macro.Compiler;
import haxe.Timer;
import lime.app.Event;
import lime.media.AudioBuffer;
import lime.system.System;
import lime.utils.AssetLibrary;
import lime.utils.Assets;
import lime.utils.AssetType;
import lime.utils.Log;
#if (js && html5)
import js.html.Image;
import js.html.SpanElement;
import js.Browser;
import lime.net.HTTPRequest;
#elseif flash
import flash.display.LoaderInfo;
import flash.display.Sprite;
import flash.events.ProgressEvent;
import flash.Lib;
#end

@:access(lime.utils.AssetLibrary)
#if !lime_debug
@:fileXml('tags="haxe,release"')
@:noDebug
#end
class Preloader
{
	public var complete(default, null):Bool;
	public var onComplete = new Event<Void->Void>();
	public var onProgress = new Event<Int->Int->Void>();

	@:noCompletion private var bytesLoaded:Int;
	@:noCompletion private var bytesLoadedCache = new ObjectMap<#if !disable_preloader_assets AssetLibrary #else Dynamic #end, Int>();
	@:noCompletion private var bytesLoadedCache2 = new Map<String, Int>();
	@:noCompletion private var bytesTotal:Int;
	@:noCompletion private var bytesTotalCache = new Map<String, Int>();
	@:noCompletion private var initLibraryNames:Bool;
	@:noCompletion private var libraries:Array<#if !disable_preloader_assets AssetLibrary #else Dynamic #end>;
	@:noCompletion private var libraryNames:Array<String>;
	@:noCompletion private var loadedLibraries:Int;
	@:noCompletion private var loadedStage:Bool;
	@:noCompletion private var preloadComplete:Bool;
	@:noCompletion private var preloadStarted:Bool;
	@:noCompletion private var simulateProgress:Bool;

	public function new()
	{
		// TODO: Split out core preloader support from generic Preloader type

		bytesLoaded = 0;
		bytesTotal = 0;

		libraries = new Array<#if !disable_preloader_assets AssetLibrary #else Dynamic #end>();
		libraryNames = new Array<String>();

		onProgress.add(update);

		#if simulate_preloader
		var preloadTime = Std.parseInt(Compiler.getDefine("simulate_preloader"));

		if (preloadTime == 1)
		{
			preloadTime = 3000;
		}

		var startTime = Std.int(System.getTimer());
		var currentTime = 0;
		var timeStep = Std.int(1000 / 60);
		var timer = new Timer(timeStep);

		simulateProgress = true;

		timer.run = function()
		{
			currentTime = Std.int(System.getTimer() - startTime);
			if (currentTime > preloadTime) currentTime = preloadTime;
			onProgress.dispatch(currentTime, preloadTime);

			if (currentTime >= preloadTime)
			{
				timer.stop();

				simulateProgress = false;
				start();
			}
		};
		#end
	}

	public function addLibrary(library:#if !disable_preloader_assets AssetLibrary #else Dynamic #end):Void
	{
		libraries.push(library);
	}

	public function addLibraryName(name:String):Void
	{
		if (libraryNames.indexOf(name) == -1)
		{
			libraryNames.push(name);
		}
	}

	public function load():Void
	{
		for (library in libraries)
		{
			bytesTotal += library.bytesTotal;
		}

		loadedLibraries = -1;
		preloadStarted = false;

		for (library in libraries)
		{
			Log.verbose("Preloading asset library");

			library.load()
				.onProgress(function(loaded, total)
				{
					var cache = bytesLoadedCache.get(library);
					if (cache == null)
						bytesLoaded += loaded;
					else
						bytesLoaded += loaded - cache;

					bytesLoadedCache.set(library, loaded);

					if (!simulateProgress) onProgress.dispatch(bytesLoaded, bytesTotal);
				})
				.onComplete(function(_)
				{
					var cache = bytesLoadedCache.get(library);
					if (cache == null)
						bytesLoaded += library.bytesTotal;
					else
						bytesLoaded += Std.int(library.bytesTotal) - cache;

					loadedAssetLibrary();
				})
				.onError(function(e)
				{
					Log.error(e);
				});
		}

		// TODO: Handle bytes total better

		for (name in libraryNames)
		{
			bytesTotal += 200;
		}

		loadedLibraries++;
		preloadStarted = true;
		updateProgress();
	}

	@:noCompletion private function loadedAssetLibrary(name:String = null):Void
	{
		loadedLibraries++;

		var current = loadedLibraries;
		if (!preloadStarted) current++;

		var totalLibraries = libraries.length + libraryNames.length;

		if (name != null)
		{
			Log.verbose("Loaded asset library: " + name + " [" + current + "/" + totalLibraries + "]");
		}
		else
		{
			Log.verbose("Loaded asset library [" + current + "/" + totalLibraries + "]");
		}

		updateProgress();
	}

	@:noCompletion private function start():Void
	{
		if (complete || simulateProgress || !preloadComplete) return;

		complete = true;

		onComplete.dispatch();
	}

	@:noCompletion private function update(loaded:Int, total:Int):Void {}

	@:noCompletion private function updateProgress():Void
	{
		if (!simulateProgress)
		{
			onProgress.dispatch(bytesLoaded, bytesTotal);
		}

		#if !disable_preloader_assets
		if (loadedLibraries == libraries.length && !initLibraryNames)
		{
			initLibraryNames = true;

			for (name in libraryNames)
			{
				Log.verbose("Preloading asset library: " + name);

				Assets.loadLibrary(name)
					.onProgress(function(loaded, total)
					{
						if (total > 0)
						{
							var totalCache = bytesTotalCache.get(name);
							if (totalCache == null)
							{
								bytesTotalCache.set(name, total);
								bytesTotal += (total - 200);
							}

							if (loaded > total) loaded = total;

							var loadedCache = bytesLoadedCache2.get(name);
							if (loadedCache == null)
								bytesLoaded += loaded;
							else
								bytesLoaded += loaded - loadedCache;

							bytesLoadedCache2.set(name, loaded);

							if (!simulateProgress) onProgress.dispatch(bytesLoaded, bytesTotal);
						}
					})
					.onComplete(function(library)
					{
						var total = bytesTotalCache.get(name);
						if (total == null) total = 200;

						var cache = bytesLoadedCache2.get(name);
						if (cache == null)
							bytesLoaded += total;
						else
							bytesLoaded += total - cache;

						loadedAssetLibrary(name);
					})
					.onError(function(e)
					{
						Log.error(e);
					});
			}
		}
		#end

		if (!simulateProgress
			&& loadedLibraries == (libraries.length + libraryNames.length))
		{
			if (!preloadComplete)
			{
				preloadComplete = true;

				Log.verbose("Preload complete");
			}

			start();
		}
	}
}

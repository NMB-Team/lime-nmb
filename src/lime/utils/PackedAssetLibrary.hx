package lime.utils;

import haxe.io.Bytes;
import haxe.io.Path;
import lime.app.Event;
import lime.app.Future;
import lime.app.Promise;
import lime.media.AudioBuffer;
import lime.graphics.Image;
import lime.net.HTTPRequest;
import lime.text.Font;
import lime.utils.AssetType;
import lime.utils.Bytes;

#if !lime_debug
@:fileXml('tags="haxe,release"')
@:noDebug
#end
@:keep class PackedAssetLibrary extends AssetLibrary
{
	@:noCompletion private var id:String;
	@:noCompletion private var lengths = new Map<String, Int>();
	@:noCompletion private var packedData:Bytes;
	@:noCompletion private var positions = new Map<String, Int>();
	@:noCompletion private var type:String;
	@:noCompletion private var rootPath:String;

	public function new(id:String, type:String)
	{
		super();

		this.id = id;
		this.type = type;
	}

	@:noCompletion private inline function extractBytes(id:String):Bytes
	{
		var bytes = Bytes.alloc(lengths[id]);
		bytes.blit(0, packedData, positions[id], lengths[id]);
		if (type == "gzip") return bytes.decompress(GZIP);
		if (type == "zip" || type == "deflate") return bytes.decompress(DEFLATE);
		return bytes;
	}

	public static function fromBytes(bytes:Bytes, rootPath:String = null):PackedAssetLibrary
	{
		return cast fromManifest(AssetManifest.fromBytes(bytes, rootPath));
	}

	public static function fromFile(path:String, rootPath:String = null):PackedAssetLibrary
	{
		return cast fromManifest(AssetManifest.fromFile(path, rootPath));
	}

	public static function fromManifest(manifest:AssetManifest):PackedAssetLibrary
	{
		return cast AssetLibrary.fromManifest(manifest);
	}

	public override function getAudioBuffer(id:String):AudioBuffer
	{
		#if (js && html5)
		return super.getAudioBuffer(id);
		#else
		var cached = cachedAudioBuffers.get(id);
		if (cached != null) return cached;
		return AudioBuffer.fromBytes(extractBytes(id));
		#end
	}

	public override function getBytes(id:String):Bytes
	{
		var cached = cachedBytes.get(id);
		if (cached != null) return cached;

		var text = cachedText.get(id);
		if (text != null)
		{
			var bytes = Bytes.ofString(text);
			cachedBytes.set(id, bytes);
			return bytes;
		}

		return extractBytes(id);
	}

	public override function getFont(id:String):Font
	{
		#if (js && html5)
		return super.getFont(id);
		#else
		var cached = cachedFonts.get(id);
		if (cached != null) return cached;
		return Font.fromBytes(extractBytes(id));
		#end
	}

	public override function getImage(id:String):Image
	{
		var cached = cachedImages.get(id);
		if (cached != null) return cached;
		return Image.fromBytes(extractBytes(id));
	}

	public override function getText(id:String):String
	{
		var cached = cachedText.get(id);
		if (cached != null) return cached;

		if (type == "gzip" || type == "zip" || type == "deflate")
		{
			var bytes = extractBytes(id);
			return bytes.getString(0, bytes.length);
		}

		return packedData.getString(positions[id], lengths[id]);
	}

	public override function isLocal(id:String, type:String):Bool
	{
		return true;
	}

	public override function load():Future<AssetLibrary>
	{
		if (loaded)
		{
			return Future.withValue(cast this);
		}

		if (promise == null)
		{
			promise = new Promise<AssetLibrary>();

			// TODO: Handle `preload` for individual assets
			// TODO: Do not preload bytes on native, if we can read from it instead (all non-Android targets?)

			var packedData_onComplete = function(data:Bytes)
			{
				cachedBytes.set(id, data);
				packedData = data;

				assetsLoaded = 0;
				assetsTotal = 1;

				for (id in preload.keys())
				{
					if (!preload.get(id)) continue;

					Log.verbose("Preloading asset: " + id + " [" + types.get(id) + "]");

					switch (types.get(id))
					{
						case BINARY:
							assetsTotal++;

							var future = loadBytes(id);
							// future.onProgress (load_onProgress.bind (id));
							future.onError(load_onError.bind(id));
							future.onComplete(loadBytes_onComplete.bind(id));

						case FONT:
							assetsTotal++;

							var future = loadFont(id);
							// future.onProgress (load_onProgress.bind (id));
							future.onError(load_onError.bind(id));
							future.onComplete(loadFont_onComplete.bind(id));

						case IMAGE:
							assetsTotal++;

							var future = loadImage(id);
							// future.onProgress (load_onProgress.bind (id));
							future.onError(load_onError.bind(id));
							future.onComplete(loadImage_onComplete.bind(id));

						case MUSIC, SOUND:
							assetsTotal++;

							var future = loadAudioBuffer(id);
							// future.onProgress (load_onProgress.bind (id));
							future.onError(load_onError.bind(id));
							future.onComplete(loadAudioBuffer_onComplete.bind(id));

						case TEXT:
							assetsTotal++;

							var future = loadText(id);
							// future.onProgress (load_onProgress.bind (id));
							future.onError(load_onError.bind(id));
							future.onComplete(loadText_onComplete.bind(id));

						default:
					}
				}

				__assetLoaded(null);
			};

			if (cachedBytes.exists(id))
			{
				packedData_onComplete(cachedBytes.get(id));
			}
			else
			{
				var basePath = rootPath == null || rootPath == "" ?  "" : Path.addTrailingSlash(rootPath);
				var libPath = getPath(id);
				if (libPath == null) libPath = id;

				var path = Path.join([basePath, libPath]);
				path = __cacheBreak(path);

				Bytes.loadFromFile(path).onError(promise.error).onComplete(packedData_onComplete);
			}
		}

		return promise.future;
	}

	public override function loadAudioBuffer(id:String):Future<AudioBuffer>
	{
		#if (js && html5)
		return super.loadAudioBuffer(id);
		#else
		var cached = cachedAudioBuffers.get(id);
		if (cached != null) return Future.withValue(cached);
		return Future.withValue(AudioBuffer.fromBytes(extractBytes(id)));
		#end
	}

	public override function loadBytes(id:String):Future<Bytes>
	{
		var cached = cachedBytes.get(id);
		if (cached != null) return Future.withValue(cached);
		return Future.withValue(extractBytes(id));
	}

	public override function loadFont(id:String):Future<Font>
	{
		#if (js && html5)
		return super.loadFont(id);
		#else
		var cached = cachedFonts.get(id);
		if (cached != null) return Future.withValue(cached);
		return Font.loadFromBytes(extractBytes(id));
		#end
	}

	public static function loadFromBytes(bytes:Bytes, rootPath:String = null):Future<PackedAssetLibrary>
	{
		return AssetLibrary.loadFromBytes(bytes, rootPath).then(function(library)
		{
			var assetLibrary:PackedAssetLibrary = cast library;
			return Future.withValue(assetLibrary);
		});
	}

	public static function loadFromFile(path:String, rootPath:String = null):Future<PackedAssetLibrary>
	{
		return AssetLibrary.loadFromFile(path, rootPath).then(function(library)
		{
			var assetLibrary:PackedAssetLibrary = cast library;
			return Future.withValue(assetLibrary);
		});
	}

	public static function loadFromManifest(manifest:AssetManifest):Future<PackedAssetLibrary>
	{
		return AssetLibrary.loadFromManifest(manifest).then(function(library)
		{
			var assetLibrary:PackedAssetLibrary = cast library;
			return Future.withValue(assetLibrary);
		});
	}

	public override function loadImage(id:String):Future<Image>
	{
		var cached = cachedImages.get(id);
		if (cached != null) return Future.withValue(cached);
		return Image.loadFromBytes(extractBytes(id));
	}

	public override function loadText(id:String):Future<String>
	{
		var cached = cachedText.get(id);
		if (cached != null) return Future.withValue(cached);

		var cachedB = cachedBytes.get(id);
		if (cachedB != null)
		{
			var bytes = getBytes(id);
			if (bytes == null) return cast Future.withValue(null);

			var text = bytes.getString(0, bytes.length);
			cachedText.set(id, text);
			return Future.withValue(text);
		}

		if (type == "gzip" || type == "deflate")
		{
			var bytes = extractBytes(id);
			return Future.withValue(bytes.getString(0, bytes.length));
		}

		return Future.withValue(packedData.getString(positions[id], lengths[id]));
	}

	public override function unload():Void {}

	@:noCompletion private override function __fromManifest(manifest:AssetManifest):Void
	{
		rootPath = manifest.rootPath;

		super.__fromManifest(manifest);

		for (asset in manifest.assets)
		{
			if (Reflect.hasField(asset, "position"))
			{
				positions.set(asset.id, Reflect.field(asset, "position"));
			}

			if (Reflect.hasField(asset, "length"))
			{
				lengths.set(asset.id, Reflect.field(asset, "length"));
			}
		}
	}

	@:noCompletion private override function __assetLoaded(id:String):Void
	{
		assetsLoaded++;

		if (id != null)
		{
			Log.verbose("Loaded asset: " + id + " [" + types.get(id) + "] (" + (assetsLoaded - 1) + "/" + (assetsTotal - 1) + ")");
		}

		// if (id != null) {

		// 	var size = sizes.get (id);

		// 	if (!bytesLoadedCache.exists (id)) {

		// 		bytesLoaded += size;

		// 	} else {

		// 		var cache = bytesLoadedCache.get (id);

		// 		if (cache < size) {

		// 			bytesLoaded += (size - cache);

		// 		}

		// 	}

		// 	bytesLoadedCache.set (id, size);

		// }

		if (assetsLoaded < assetsTotal)
		{
			// promise.progress (bytesLoaded, bytesTotal);
		}
		else
		{
			loaded = true;
			// promise.progress (bytesTotal, bytesTotal);
			promise.complete(this);
		}
	}
}

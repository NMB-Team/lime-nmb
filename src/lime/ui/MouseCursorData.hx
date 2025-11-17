package lime.ui;

#if (!macro && lime_cffi)
import lime._internal.backend.native.NativeCFFI;
#end
import lime.graphics.Image;
import lime.graphics.ImageType;

#if !lime_debug
@:fileXml('tags="haxe,release"')
@:noDebug
#end
@:access(lime._internal.backend.native.NativeCFFI)
class MouseCursorData {
	public var image(default, set):Image;
	public var hotX(default, set):Int;
	public var hotY(default, set):Int;

	@:noCompletion private var __dirty:Bool = true;
	@:noCompletion private var __handle:Dynamic;

	public function new(image:Image, hotX:Int, hotY:Int) {
		this.image = image;
		this.hotX = hotX;
		this.hotY = hotY;
	}

	@:noCompletion private function __update():Void {
		if (!__dirty)
			return;
		#if (js && html5)
		final fallback = "default";
		__handle = (image == null) ? fallback : switch (image.type) {
			case DATA | CANVAS:
				'url(${image.src.toDataURL("image/png")}) ${hotX} ${hotY}, $fallback';
			default:
				fallback;
		}
		#elseif (!macro && lime_cffi)
		__handle = (image == null) ? null : NativeCFFI.lime_create_cursor_image(image.buffer, hotX, hotY);
		#else
		__handle = null;
		#end
		__dirty = false;
	}

	@:noCompletion private function set_image(val:Image):Image {
		if (image != val)
			__dirty = true;
		image = val;
		set_hotX(hotX);
		set_hotY(hotY);
		return image;
	}

	@:noCompletion private function set_hotX(val:Int):Int {
		val = __clamp(val, image != null ? image.width : 0);
		if (hotX != val)
			__dirty = true;
		return hotX = val;
	}

	@:noCompletion private function set_hotY(val:Int):Int {
		val = __clamp(val, image != null ? image.height : 0);
		if (hotY != val)
			__dirty = true;
		return hotY = val;
	}

	@:noCompletion private function __clamp(val:Int, max:Int):Int {
		return val < 0 ? 0 : val > max ? max : val;
	}
}

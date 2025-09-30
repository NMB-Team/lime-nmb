package lime.graphics.opengl.ext;

#if (js && html5)
@:keep
@:native("WEBGL_blend_equation_advanced_coherent")
@:noCompletion extern class WEBGL_blend_equation_advanced_coherent
{
	public var MULTIPLY_KHR;
	public var SCREEN_KHR;
	public var OVERLAY_KHR;
	public var DARKEN_KHR;
	public var LIGHTEN_KHR;
	public var COLORDODGE_KHR;
	public var COLORBURN_KHR;
	public var HARDLIGHT_KHR;
	public var SOFTLIGHT_KHR;
	public var DIFFERENCE_KHR;
	public var EXCLUSION_KHR;
	public var HSL_HUE_KHR;
	public var HSL_SATURATION_KHR;
	public var HSL_COLOR_KHR;
	public var HSL_LUMINOSITY_KHR;

	@:noCompletion private function new() {}
}
#end
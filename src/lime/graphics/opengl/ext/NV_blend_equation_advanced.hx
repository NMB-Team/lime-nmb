package lime.graphics.opengl.ext;

@:keep
@:noCompletion class NV_blend_equation_advanced
{
	public var BLEND_PREMULTIPLIED_SRC_NV = 0x9280;
	public var BLEND_OVERLAP_NV = 0x9281;

	public var UNCORRELATED_NV = 0x9282;
	public var DISJOINT_NV = 0x9283;
	public var CONJOINT_NV = 0x9284;

	// public var ZERO= // reused public var from core
	public var SRC_NV = 0x9286;
	public var DST_NV = 0x9287;
	public var SRC_OVER_NV = 0x9288;
	public var DST_OVER_NV = 0x9289;
	public var SRC_IN_NV = 0x928A;
	public var DST_IN_NV = 0x928B;
	public var SRC_OUT_NV = 0x928C;
	public var DST_OUT_NV = 0x928D;
	public var SRC_ATOP_NV = 0x928E;
	public var DST_ATOP_NV = 0x928F;
	public var XOR_NV= 0x1506;
	public var MULTIPLY_NV = 0x9294;
	public var SCREEN_NV = 0x9295;
	public var OVERLAY_NV = 0x9296;
	public var DARKEN_NV = 0x9297;
	public var LIGHTEN_NV = 0x9298;
	public var COLORDODGE_NV = 0x9299;
	public var COLORBURN_NV = 0x929A;
	public var HARDLIGHT_NV = 0x929B;
	public var SOFTLIGHT_NV = 0x929C;
	public var DIFFERENCE_NV = 0x929E;
	public var EXCLUSION_NV = 0x92A0;
	// public var INVERT= // reused public var from core
	public var INVERT_RGB_NV = 0x92A3;
	public var LINEARDODGE_NV = 0x92A4;
	public var LINEARBURN_NV = 0x92A5;
	public var VIVIDLIGHT_NV = 0x92A6;
	public var LINEARLIGHT_NV = 0x92A7;
	public var PINLIGHT_NV = 0x92A8;
	public var HARDMIX_NV = 0x92A9;

	public var HSL_HUE_NV= 0x92AD;
	public var HSL_SATURATION_NV = 0x92AE;
	public var HSL_COLOR_NV= 0x92AF;
	public var HSL_LUMINOSITY_NV = 0x92B0;

	public var PLUS_NV = 0x9291;
	public var PLUS_CLAMPED_NV = 0x92B1;
	public var PLUS_CLAMPED_ALPHA_NV = 0x92B2;
	public var PLUS_DARKER_NV= 0x9292;
	public var MINUS_NV = 0x929F;
	public var MINUS_CLAMPED_NV= 0x92B3;
	public var CONTRAST_NV = 0x92A1;
	public var INVERT_OVG_NV = 0x92B4;
	public var RED_NV = 0x1903;
	public var GREEN_NV = 0x1904;
	public var BLUE_NV = 0x1905;

	@:noCompletion private function new() {}
   	// public function BlendParameteriNV(pname:Int, value:Int);
   	// public function BlendBarrierNV(param:Dynamic);

}
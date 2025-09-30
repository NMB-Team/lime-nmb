package lime.system;

enum abstract HintPriority(Int) to Int {
	var HINT_DEFAULT:HintPriority = -1;
	var HINT_NORMAL:HintPriority = 0;
	var HINT_OVERRIDE:HintPriority = 1;
}
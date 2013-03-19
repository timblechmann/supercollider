TuningInfo {
	*new {
		this.deprecated(thisMethod);
		^Tuning
	}
}



ScaleInfo {
	*new {
		this.deprecated(thisMethod);
		^Scale
	}
}

Proutine : Prout {
	*new { |routineFunc|
		this.deprecated(thisMethod, Prout.findMethod(\new));
		^Prout(routineFunc)
	}
}

// in 3.6, both "help" and "openHelpFile" will work fine. In 3.7, "openHelpFile" will be deprecated. In 3.8 it will be gone.

+ Object {
	openHelpFile {
		this.help
	}
}

+ String {
	openHelpFile {
		this.help
	}
}

+ Method {
	openHelpFile {
		this.help
	}
}

+ Quark {
	openHelpFile {
		this.help
	}
}


// openTextFile is actually the same as openDocument
+ String {
	openTextFile { arg selectionStart=0, selectionLength=0;
		this.openDocument(selectionStart, selectionLength)
	}
}

+ Symbol {
	openTextFile { arg selectionStart=0, selectionLength=0;
		^this.openDocument(selectionStart, selectionLength)
	}
}

// Document: themes are cocoa-specific
+ Document {
	*theme_ { |...args|
		this.deprecated(thisMethod);
		this.implementationClass.tryPerform(\theme_, args)
	}

	*theme {
		this.deprecated(thisMethod);
		^this.implementationClass.tryPerform(\theme)
	}

	*postColor {
		this.deprecated(thisMethod);
		^this.implementationClass.tryPerform(\postColor)
	}

	*postColor_ {|...args|
		this.deprecated(thisMethod);
		^this.implementationClass.tryPerform(\postColor_, args)
	}

	*background {
		this.deprecated(thisMethod);
		^this.implementationClass.tryPerform(\background)
	}

	*background_ {|...args|
		this.deprecated(thisMethod);
		^this.implementationClass.tryPerform(\background_, args)
	}

	*selectedBackground {
		this.deprecated(thisMethod);
		^this.implementationClass.tryPerform(\selectedBackground)
	}

	*selectedBackground_ {|...args|
		this.deprecated(thisMethod);
		^this.implementationClass.tryPerform(\selectedBackground_, args)
	}

	*stringColor_ {|...args|
		this.deprecated(thisMethod);
		^this.implementationClass.tryPerform(\stringColor_, args)
	}

	setFont { | ...args |
		this.deprecated(thisMethod);
		^this.subclassResponsibility(thisMethod)
	}

	setTextColor { | ...args |
		this.deprecated(thisMethod);
		^this.subclassResponsibility(thisMethod)
	}

	syntaxColorize {
		this.deprecated(thisMethod);
		^this.subclassResponsibility(thisMethod)
	}
}

// Date-bootTime
+ Date {
	bootTime {
		this.deprecated(thisMethod);
	}
	bootTime_ {
		this.deprecated(thisMethod);
	}
}

+ Platform {
	getMouseCoords {
		this.deprecated(thisMethod);
		warn("Use \"GUI.cursorPosition\" instead.");
		^GUI.cursorPosition
	}
	*getMouseCoords {
		this.deprecated(thisMethod);
		warn("Use \"GUI.cursorPosition\" instead.");
		^GUI.cursorPosition
	}
}

// degreeToKey should be implemented via Scale
+ SequenceableCollection {
	performDegreeToKey { arg scaleDegree, stepsPerOctave = 12, accidental = 0;
		var baseKey = (stepsPerOctave * (scaleDegree div: this.size)) + this.wrapAt(scaleDegree);
		this.deprecated(thisMethod);

		^if(accidental == 0) { baseKey } { baseKey + (accidental * (stepsPerOctave / 12.0)) }
	}

	performKeyToDegree { | degree, stepsPerOctave = 12 |
		var n = degree div: stepsPerOctave * this.size;
		var key = degree % stepsPerOctave;
		this.deprecated(thisMethod);
		^this.indexInBetween(key) + n
	}

	performNearestInList { | degree |
		this.deprecated(thisMethod);
		^this.at(this.indexIn(degree))
	}

	performNearestInScale { arg degree, stepsPerOctave=12; // collection is sorted
		var root = degree.trunc(stepsPerOctave);
		var key = degree % stepsPerOctave;
		this.deprecated(thisMethod);
		^key.nearestInList(this) + root
	}

	transposeKey { arg amount, octave=12;
		this.deprecated(thisMethod);
		^((this + amount) % octave).sort
	}
	mode { arg degree, octave=12;
		this.deprecated(thisMethod);
		^(rotate(this, degree.neg) - this.wrapAt(degree)) % octave
	}
}


+AbstractFunction {
	performDegreeToKey { arg scaleDegree, stepsPerOctave = 12, accidental = 0;
		this.deprecated(thisMethod);
		^this.value(scaleDegree, stepsPerOctave, accidental)
	}
}

+Scale {
	performDegreeToKey { | scaleDegree, stepsPerOctave, accidental = 0 |
		var baseKey;
		this.deprecated(thisMethod);
		stepsPerOctave = stepsPerOctave ? tuning.stepsPerOctave;
		baseKey = (stepsPerOctave * (scaleDegree div: this.size)) + this.wrapAt(scaleDegree);
		^if(accidental == 0) { baseKey } { baseKey + (accidental * (stepsPerOctave / 12.0)) }
	}

	performKeyToDegree { | degree, stepsPerOctave = 12 |
		this.deprecated(thisMethod);
		^degrees.performKeyToDegree(degree, stepsPerOctave)
	}

	performNearestInList { | degree |
		this.deprecated(thisMethod);
		^degrees.performNearestInList(degree)
	}

	performNearestInScale { | degree, stepsPerOctave=12 | // collection is sorted
		this.deprecated(thisMethod);
		^degrees.performNearestInScale(degree, stepsPerOctave)
	}
}

+ScaleStream {
	performDegreeToKey { | degree, stepsPerOctave, accidental = 0 |
		this.deprecated(thisMethod);
		^this.chooseScale(degree).performDegreeToKey(degree, stepsPerOctave, accidental);
	}

	performKeyToDegree { | degree, stepsPerOctave = 12 |
		this.deprecated(thisMethod);
		^this.chooseScale(degree).performKeyToDegree(degree, stepsPerOctave)
	}

	performNearestInList { | degree |
		this.deprecated(thisMethod);
		^this.chooseScale(degree).performNearestInList(degree)
	}

	performNearestInScale { | degree, stepsPerOctave=12 | // collection is sorted
		this.deprecated(thisMethod);
		^this.chooseScale(degree).performNearestInScale(degree, stepsPerOctave)
	}
}
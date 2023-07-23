#include "lib_sound.h"

#include "box_common.h"

#include "toy_memory.h"
#include "toy_drive_system.h"

static int nativeLoadSound(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadSound\n");
		return -1;
	}

	//get the filename
	Toy_Literal fileLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal fileLiteralIdn = fileLiteral;
	if (TOY_IS_IDENTIFIER(fileLiteral) && Toy_parseIdentifierToValue(interpreter, &fileLiteral)) {
		Toy_freeLiteral(fileLiteralIdn);
	}

	if (!TOY_IS_STRING(fileLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to loadSound\n");
		Toy_freeLiteral(fileLiteral);
		return -1;
	}

	//get the path
	Toy_Literal pathLiteral = Toy_getDrivePathLiteral(interpreter, &fileLiteral);

	if (TOY_IS_NULL(pathLiteral)) {
		Toy_freeLiteral(fileLiteral);
		Toy_freeLiteral(pathLiteral);
		return -1;
	}

	//load the file
	Mix_Chunk* sound = Mix_LoadWAV(Toy_toCString( TOY_AS_STRING(pathLiteral) ));

	if (sound == NULL) {
		interpreter->errorOutput("Failed to load the sound file: ");
		Toy_printLiteralCustom(fileLiteral, interpreter->errorOutput);
		interpreter->errorOutput("\n");

		Toy_freeLiteral(fileLiteral);
		Toy_freeLiteral(pathLiteral);

		return -1;
	}

    //return the sound
    Toy_Literal soundLiteral = TOY_TO_OPAQUE_LITERAL(sound, TOY_OPAQUE_TAG_SOUND);

	Toy_pushLiteralArray(&interpreter->stack, soundLiteral);

	Toy_freeLiteral(fileLiteral);
	Toy_freeLiteral(pathLiteral);
	Toy_freeLiteral(soundLiteral);

	return 1;
}

static int nativeFreeSound(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeSound\n");
		return -1;
	}

	//get the sound
	Toy_Literal soundLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal soundLiteralIdn = soundLiteral;
	if (TOY_IS_IDENTIFIER(soundLiteral) && Toy_parseIdentifierToValue(interpreter, &soundLiteral)) {
		Toy_freeLiteral(soundLiteralIdn);
	}

	if (!TOY_IS_OPAQUE(soundLiteral) || TOY_GET_OPAQUE_TAG(soundLiteral) != TOY_OPAQUE_TAG_SOUND) {
		interpreter->errorOutput("Incorrect argument type passed to freeSound\n");
		Toy_freeLiteral(soundLiteral);
		return -1;
	}

	//free the pointer
	Mix_Chunk* sound = TOY_AS_OPAQUE(soundLiteral);
	Mix_FreeChunk(sound);

	//cleanup
	Toy_freeLiteral(soundLiteral);

	return 0;
}

static int nativePlaySound(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to playSound\n");
		return -1;
	}

	//get the sound
	Toy_Literal soundLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal soundLiteralIdn = soundLiteral;
	if (TOY_IS_IDENTIFIER(soundLiteral) && Toy_parseIdentifierToValue(interpreter, &soundLiteral)) {
		Toy_freeLiteral(soundLiteralIdn);
	}

	if (!TOY_IS_OPAQUE(soundLiteral) || TOY_GET_OPAQUE_TAG(soundLiteral) != TOY_OPAQUE_TAG_SOUND) {
		interpreter->errorOutput("Incorrect argument type passed to playSound\n");
		Toy_freeLiteral(soundLiteral);
		return -1;
	}

	//play the pointer
	Mix_Chunk* sound = TOY_AS_OPAQUE(soundLiteral);

	Mix_PlayChannel(-1, sound, 0);
	//TODO: add repeat argument
	//TODO: add channel system

	//cleanup
	Toy_freeLiteral(soundLiteral);

	return 0;
}

static int nativeGetSoundVolume(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to getSoundVolume\n");
		return -1;
	}

	//get the sound
	Toy_Literal soundLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal soundLiteralIdn = soundLiteral;
	if (TOY_IS_IDENTIFIER(soundLiteral) && Toy_parseIdentifierToValue(interpreter, &soundLiteral)) {
		Toy_freeLiteral(soundLiteralIdn);
	}

	if (!TOY_IS_OPAQUE(soundLiteral) || TOY_GET_OPAQUE_TAG(soundLiteral) != TOY_OPAQUE_TAG_SOUND) {
		interpreter->errorOutput("Incorrect argument type passed to getSoundVolume\n");
		Toy_freeLiteral(soundLiteral);
		return -1;
	}

	//get the volume of the chunk
	Mix_Chunk* sound = TOY_AS_OPAQUE(soundLiteral);
	Toy_Literal resultLiteral = TOY_TO_INTEGER_LITERAL(Mix_VolumeChunk(sound, -1));

	//return the value
	Toy_pushLiteralArray(&interpreter->stack, resultLiteral);

	Toy_freeLiteral(resultLiteral);
	Toy_freeLiteral(soundLiteral);

	return 1;
}

static int nativeSetSoundVolume(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 2) {
		interpreter->errorOutput("Incorrect number of arguments passed to setSoundVolume\n");
		return -1;
	}

	//get the sound
	Toy_Literal volumeLiteral = Toy_popLiteralArray(arguments);
	Toy_Literal soundLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal soundLiteralIdn = soundLiteral;
	if (TOY_IS_IDENTIFIER(soundLiteral) && Toy_parseIdentifierToValue(interpreter, &soundLiteral)) {
		Toy_freeLiteral(soundLiteralIdn);
	}

	Toy_Literal volumeLiteralIdn = volumeLiteral;
	if (TOY_IS_IDENTIFIER(volumeLiteral) && Toy_parseIdentifierToValue(interpreter, &volumeLiteral)) {
		Toy_freeLiteral(volumeLiteralIdn);
	}

	if (!TOY_IS_OPAQUE(soundLiteral) || !TOY_IS_INTEGER(volumeLiteral) || TOY_GET_OPAQUE_TAG(soundLiteral) != TOY_OPAQUE_TAG_SOUND) {
		interpreter->errorOutput("Incorrect argument type passed to setSoundVolume\n");
		Toy_freeLiteral(soundLiteral);
		Toy_freeLiteral(volumeLiteral);
		return -1;
	}

	//set the volume of the chunk
	Mix_Chunk* sound = TOY_AS_OPAQUE(soundLiteral);
	Mix_VolumeChunk(sound, TOY_AS_INTEGER(volumeLiteral));

	Toy_freeLiteral(soundLiteral);
	Toy_freeLiteral(volumeLiteral);

	return 0;
}

//call the hook
typedef struct Natives {
	char* name;
	Toy_NativeFn fn;
} Natives;

int Box_hookSound(Toy_Interpreter* interpreter, Toy_Literal identifier, Toy_Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"loadSound", nativeLoadSound},
		{"freeSound", nativeFreeSound},
		{"playSound", nativePlaySound},

		{"getSoundVolume", nativeGetSoundVolume},
		{"setSoundVolume", nativeSetSoundVolume},

		{NULL, NULL}
	};

	//store the library in an aliased dictionary
	if (!TOY_IS_NULL(alias)) {
		//make sure the name isn't taken
		if (Toy_isDelcaredScopeVariable(interpreter->scope, alias)) {
			interpreter->errorOutput("Can't override an existing variable\n");
			Toy_freeLiteral(alias);
			return false;
		}

		//create the dictionary to load up with functions
		Toy_LiteralDictionary* dictionary = TOY_ALLOCATE(Toy_LiteralDictionary, 1);
		Toy_initLiteralDictionary(dictionary);

		//load the dict with functions
		for (int i = 0; natives[i].name; i++) {
			Toy_Literal name = TOY_TO_STRING_LITERAL(Toy_createRefString(natives[i].name));
			Toy_Literal func = TOY_TO_FUNCTION_NATIVE_LITERAL(natives[i].fn);

			Toy_setLiteralDictionary(dictionary, name, func);

			Toy_freeLiteral(name);
			Toy_freeLiteral(func);
		}

		//build the type
		Toy_Literal type = TOY_TO_TYPE_LITERAL(TOY_LITERAL_DICTIONARY, true);
		Toy_Literal strType = TOY_TO_TYPE_LITERAL(TOY_LITERAL_STRING, true);
		Toy_Literal fnType = TOY_TO_TYPE_LITERAL(TOY_LITERAL_FUNCTION_NATIVE, true);
		TOY_TYPE_PUSH_SUBTYPE(&type, strType);
		TOY_TYPE_PUSH_SUBTYPE(&type, fnType);

		//set scope
		Toy_Literal dict = TOY_TO_DICTIONARY_LITERAL(dictionary);
		Toy_declareScopeVariable(interpreter->scope, alias, type);
		Toy_setScopeVariable(interpreter->scope, alias, dict, false);

		//cleanup
		Toy_freeLiteral(dict);
		Toy_freeLiteral(type);
		return 0;
	}

	//default
	for (int i = 0; natives[i].name; i++) {
		Toy_injectNativeFn(interpreter, natives[i].name, natives[i].fn);
	}

	return 0;
}

#include "lib_music.h"

#include "box_common.h"
#include "box_engine.h"

#include "toy_memory.h"
#include "toy_drive_system.h"

static int nativeLoadMusic(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadMusic\n");
		return -1;
	}

	//get the filename
	Toy_Literal fileLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal fileLiteralIdn = fileLiteral;
	if (TOY_IS_IDENTIFIER(fileLiteral) && Toy_parseIdentifierToValue(interpreter, &fileLiteral)) {
		Toy_freeLiteral(fileLiteralIdn);
	}

	if (!TOY_IS_STRING(fileLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to loadMusic\n");
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
	if (engine.music != NULL) {
		Mix_FreeMusic(engine.music);
	}

	engine.music = Mix_LoadMUS(Toy_toCString( TOY_AS_STRING(pathLiteral) ));

	if (engine.music == NULL) {
		interpreter->errorOutput("Failed to load the music file: ");
		Toy_printLiteralCustom(fileLiteral, interpreter->errorOutput);
		interpreter->errorOutput("\n");

		Toy_freeLiteral(fileLiteral);
		Toy_freeLiteral(pathLiteral);

		return -1;
	}

	Toy_freeLiteral(fileLiteral);
	Toy_freeLiteral(pathLiteral);

	return 0;
}

static int nativefreeMusic(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 0) {
		interpreter->errorOutput("Incorrect number of arguments passed to freeMusic\n");
		return -1;
	}

	//free the file
	if (engine.music != NULL) {
		Mix_FreeMusic(engine.music);
		engine.music = NULL;
	}

	return 0;
}

static int nativePlayMusic(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 0) {
		interpreter->errorOutput("Incorrect number of arguments passed to playMusic\n");
		return -1;
	}

	//free the file
	if (engine.music == NULL) {
		interpreter->errorOutput("Could not play music, no music file loaded\n");
		return -1;
	}

	Mix_PlayMusic(engine.music, -1); //NOTE: you can have more than one music file loaded, but for now one is enough

	return 0;
}

static int nativeStopMusic(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 0) {
		interpreter->errorOutput("Incorrect number of arguments passed to stopMusic\n");
		return -1;
	}

	Mix_HaltMusic();

	return 0;
}

static int nativeCheckMusicPlaying(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 0) {
		interpreter->errorOutput("Incorrect number of arguments passed to checkMusicPlaying\n");
		return -1;
	}

	//generate the boolean
	Toy_Literal resultLiteral = TOY_TO_BOOLEAN_LITERAL(Mix_PlayingMusic() != 0);

	//return the value
	Toy_pushLiteralArray(&interpreter->stack, resultLiteral);

	return 1;
}

static int nativePauseMusic(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 0) {
		interpreter->errorOutput("Incorrect number of arguments passed to pauseMusic\n");
		return -1;
	}

	Mix_PauseMusic();

	return 0;
}

static int nativeUnpauseMusic(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 0) {
		interpreter->errorOutput("Incorrect number of arguments passed to unpauseMusic\n");
		return -1;
	}

	Mix_ResumeMusic();

	return 0;
}

static int nativeCheckMusicPaused(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 0) {
		interpreter->errorOutput("Incorrect number of arguments passed to checkMusicPaused\n");
		return -1;
	}

	//generate the boolean
	Toy_Literal resultLiteral = TOY_TO_BOOLEAN_LITERAL(Mix_PausedMusic() != 0);

	//return the value
	Toy_pushLiteralArray(&interpreter->stack, resultLiteral);

	return 1;
}

static int nativeGetMusicVolume(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 0) {
		interpreter->errorOutput("Incorrect number of arguments passed to getMusicVolume\n");
		return -1;
	}

	//generate the value
	Toy_Literal resultLiteral = TOY_TO_INTEGER_LITERAL(Mix_VolumeMusic(-1));

	//return the value
	Toy_pushLiteralArray(&interpreter->stack, resultLiteral);

	Toy_freeLiteral(resultLiteral);

	return 1;
}

static int nativeSetMusicVolume(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	//checks
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to setMusicVolume\n");
		return -1;
	}

	//get the volume
	Toy_Literal volumeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal volumeLiteralIdn = volumeLiteral;
	if (TOY_IS_IDENTIFIER(volumeLiteral) && Toy_parseIdentifierToValue(interpreter, &volumeLiteral)) {
		Toy_freeLiteral(volumeLiteralIdn);
	}

	if (!TOY_IS_INTEGER(volumeLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to setMusicVolume\n");
		Toy_freeLiteral(volumeLiteral);
		return -1;
	}

	//set the volume
	Mix_VolumeMusic(TOY_AS_INTEGER(volumeLiteral));

	//cleanup
	Toy_freeLiteral(volumeLiteral);

	return 0;
}

//call the hook
typedef struct Natives {
	char* name;
	Toy_NativeFn fn;
} Natives;

int Box_hookMusic(Toy_Interpreter* interpreter, Toy_Literal identifier, Toy_Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"loadMusic", nativeLoadMusic},
		{"freeMusic", nativefreeMusic},
		{"playMusic", nativePlayMusic},
		{"stopMusic", nativeStopMusic},
		{"checkMusicPlaying", nativeCheckMusicPlaying},

		{"pauseMusic", nativePauseMusic},
		{"unpauseMusic", nativeUnpauseMusic},
		{"checkMusicPaused", nativeCheckMusicPaused},

		{"getMusicVolume", nativeGetMusicVolume},
		{"setMusicVolume", nativeSetMusicVolume},

		{NULL, NULL}
	};

	//store the library in an aliased dictionary
	if (!TOY_IS_NULL(alias)) {
		//make sure the name isn't taken
		if (Toy_isDeclaredScopeVariable(interpreter->scope, alias)) {
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

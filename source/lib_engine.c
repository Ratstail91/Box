#include "lib_engine.h"

#include "box_engine.h"

#include "repl_tools.h"
#include "drive_system.h"

#include "toy_memory.h"
#include "toy_literal_array.h"

#include "toy_console_colors.h"

#include <stdio.h>

//errors here should be fatal
static void fatalError(char* message) {
	fprintf(stderr, TOY_CC_ERROR "%s" TOY_CC_RESET, message);
	exit(-1);
}

//native functions to be called
static int nativeInitWindow(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (engine.window != NULL) {
		fatalError("Can't re-initialize the window\n");
	}

	if (arguments->count != 4) {
		fatalError("Incorrect number of arguments passed to initWindow\n");
	}

	//extract the arguments
	Toy_Literal fscreen = Toy_popLiteralArray(arguments);
	Toy_Literal screenHeight = Toy_popLiteralArray(arguments);
	Toy_Literal screenWidth = Toy_popLiteralArray(arguments);
	Toy_Literal caption = Toy_popLiteralArray(arguments);

	Toy_Literal captionIdn = caption;
	if (TOY_IS_IDENTIFIER(caption) && Toy_parseIdentifierToValue(interpreter, &caption)) {
		Toy_freeLiteral(captionIdn);
	}

	Toy_Literal screenWidthIdn = screenWidth;
	if (TOY_IS_IDENTIFIER(screenWidth) && Toy_parseIdentifierToValue(interpreter, &screenWidth)) {
		Toy_freeLiteral(screenWidthIdn);
	}

	Toy_Literal screenHeightIdn = screenHeight;
	if (TOY_IS_IDENTIFIER(screenHeight) && Toy_parseIdentifierToValue(interpreter, &screenHeight)) {
		Toy_freeLiteral(screenHeightIdn);
	}

	Toy_Literal fscreenIdn = fscreen;
	if (TOY_IS_IDENTIFIER(fscreen) && Toy_parseIdentifierToValue(interpreter, &fscreen)) {
		Toy_freeLiteral(fscreenIdn);
	}

	//check argument types
	if (!TOY_IS_STRING(caption) || !TOY_IS_INTEGER(screenWidth) || !TOY_IS_INTEGER(screenHeight) || !TOY_IS_BOOLEAN(fscreen)) {
		fatalError("Incorrect argument type passed to initWindow\n");
	}

	//init the window
	engine.window = SDL_CreateWindow(
		Toy_toCString(TOY_AS_STRING(caption)),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		engine.screenWidth = TOY_AS_INTEGER(screenWidth),
		engine.screenHeight = TOY_AS_INTEGER(screenHeight),
		TOY_IS_TRUTHY(fscreen) ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE
	);

	if (engine.window == NULL) {
		fatalError("Failed to initialize the window\n");
	}

	//init the renderer
	// SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	engine.renderer = SDL_CreateRenderer(engine.window, -1, SDL_RENDERER_ACCELERATED);

	if (engine.renderer == NULL) {
		fatalError("Failed to initialize the renderer\n");
	}

	SDL_RendererInfo rendererInfo;
	SDL_GetRendererInfo(engine.renderer, &rendererInfo);

	//printf("Renderer: %s (HW %s)\n", rendererInfo.name, rendererInfo.flags & SDL_RENDERER_ACCELERATED ? "yes" : "no");

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_RenderSetLogicalSize(engine.renderer, engine.screenWidth, engine.screenHeight);

	//only run with a window
	engine.running = true;

	Toy_freeLiteral(caption);
	Toy_freeLiteral(screenWidth);
	Toy_freeLiteral(screenHeight);
	Toy_freeLiteral(fscreen);

	return 0;
}

static int nativeLoadRootNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to loadRootNode\n");
		return -1;
	}

	//extract the arguments
	Toy_Literal drivePathLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal drivePathLiteralIdn = drivePathLiteral;
	if (TOY_IS_IDENTIFIER(drivePathLiteral) && Toy_parseIdentifierToValue(interpreter, &drivePathLiteral)) {
		Toy_freeLiteral(drivePathLiteralIdn);
	}

	//check argument types
	if (!TOY_IS_STRING(drivePathLiteral)) {
		interpreter->errorOutput("Incorrect argument type passed to loadRootNode\n");
		Toy_freeLiteral(drivePathLiteral);
		return -1;
	}

	Toy_Literal filePathLiteral = Toy_getDrivePathLiteral(interpreter, &drivePathLiteral);

	Toy_freeLiteral(drivePathLiteral); //not needed anymore

	if (!TOY_IS_STRING(filePathLiteral)) {
		Toy_freeLiteral(filePathLiteral);
		return -1;
	}

	//set the signal that a new node is needed
	engine.nextRootNodeFilename = Toy_copyLiteral(filePathLiteral);

	Toy_freeLiteral(filePathLiteral);

	return 0;
}

static int nativeGetRootNode(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 0) {
		interpreter->errorOutput("Incorrect number of arguments passed to getRootNode\n");
		return -1;
	}

	if (engine.rootNode == NULL) {
		interpreter->errorOutput("Can't access root node until after initialization\n");
		return -1;
	}

	Toy_Literal resultLiteral = TOY_TO_OPAQUE_LITERAL(engine.rootNode, BOX_OPAQUE_TAG_NODE);

	Toy_pushLiteralArray(&interpreter->stack, resultLiteral);

	return 1;
}

//utility functions
static int nativeSetRenderTarget(Toy_Interpreter* interpreter, Toy_LiteralArray* arguments) {
	if (arguments->count != 1) {
		interpreter->errorOutput("Incorrect number of arguments passed to setRenderTarget\n");
		return -1;
	}

	//get the target node OR null value
	Toy_Literal nodeLiteral = Toy_popLiteralArray(arguments);

	Toy_Literal nodeLiteralIdn = nodeLiteral;
	if (TOY_IS_IDENTIFIER(nodeLiteral) && Toy_parseIdentifierToValue(interpreter, &nodeLiteral)) {
		Toy_freeLiteral(nodeLiteralIdn);
	}

	//null value is allowed - indicates setting the screen as the render target
	if (!TOY_IS_NULL(nodeLiteral) && (!TOY_IS_OPAQUE(nodeLiteral) || TOY_GET_OPAQUE_TAG(nodeLiteral) != BOX_OPAQUE_TAG_NODE) ) {
		interpreter->errorOutput("Incorrect argument type passed to setRenderTarget\n");
		Toy_freeLiteral(nodeLiteral);
	}

	if (TOY_IS_NULL(nodeLiteral)) {
		SDL_SetRenderTarget(engine.renderer, NULL);
	}

	else {
		SDL_SetRenderTarget(engine.renderer, ((Box_Node*)TOY_AS_OPAQUE(nodeLiteral))->texture);
	}

	Toy_freeLiteral(nodeLiteral);

	return 0;
}


//call the hook
typedef struct Natives {
	char* name;
	Toy_NativeFn fn;
} Natives;

int Box_hookEngine(Toy_Interpreter* interpreter, Toy_Literal identifier, Toy_Literal alias) {
	//build the natives list
	Natives natives[] = {
		{"initWindow", nativeInitWindow},
		{"loadRootNode", nativeLoadRootNode},
		{"getRootNode", nativeGetRootNode},
		{"setRenderTarget", nativeSetRenderTarget},
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

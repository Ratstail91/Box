#include "box_engine.h"

#include "lib_about.h"
#include "lib_standard.h"
#include "lib_random.h"
#include "lib_runner.h"
#include "lib_engine.h"
#include "lib_node.h"
#include "lib_input.h"
#include "lib_music.h"
#include "lib_sound.h"

#include "repl_tools.h"

#include "toy_memory.h"
#include "toy_lexer.h"
#include "toy_parser.h"
#include "toy_compiler.h"
#include "toy_interpreter.h"
#include "toy_literal_array.h"
#include "toy_literal_dictionary.h"

#include "toy_console_colors.h"
#include "toy_drive_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//define the extern engine object
Box_Engine engine;

//errors here should be fatal
static void fatalError(char* message) {
	fprintf(stderr, TOY_CC_ERROR "%s\n" TOY_CC_RESET, message);
	exit(-1);
}

//exposed functions
void Box_initEngine(const char* initScript) {
	//clear
	engine.rootNode = NULL;
	engine.nextRootNodeFilename = TOY_TO_NULL_LITERAL;
	engine.running = false;
	engine.window = NULL;
	engine.renderer = NULL;
	engine.music = NULL;

	//init SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
		fatalError("Failed to initialize SDL2");
	}

	//init SDL_image
	int imageFlags = IMG_INIT_PNG | IMG_INIT_JPG;
	if (IMG_Init(imageFlags) != imageFlags) {
		fatalError("Failed to initialize SDL2_image");
	}

	//init SDL_mixer
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
		fatalError("Failed to initialize SDL2_mixer");
	}

	//init SDL_ttf
	if (TTF_Init() != 0) {
		fatalError("Failed to initialize SDL2_ttf");
	}

	//init events
	Toy_initLiteralDictionary(&engine.symKeyDownEvents);
	Toy_initLiteralDictionary(&engine.symKeyUpEvents);

	//init Toy
	Toy_initInterpreter(&engine.interpreter);
	Toy_injectNativeHook(&engine.interpreter, "about", Toy_hookAbout);
	Toy_injectNativeHook(&engine.interpreter, "standard", Toy_hookStandard);
	Toy_injectNativeHook(&engine.interpreter, "random", Toy_hookRandom);
	Toy_injectNativeHook(&engine.interpreter, "runner", Toy_hookRunner);
	Toy_injectNativeHook(&engine.interpreter, "engine", Box_hookEngine);
	Toy_injectNativeHook(&engine.interpreter, "node", Box_hookNode);
	Toy_injectNativeHook(&engine.interpreter, "input", Box_hookInput);
	Toy_injectNativeHook(&engine.interpreter, "music", Box_hookMusic);
	Toy_injectNativeHook(&engine.interpreter, "sound", Box_hookSound);

	//load the initScript with the drive path system
	Toy_Literal scriptLiteral = TOY_TO_STRING_LITERAL(Toy_createRefString(initScript));
	Toy_Literal driveLiteral = Toy_getDrivePathLiteral(&engine.interpreter, &scriptLiteral); //only takes the interpreter for the error function

	if (TOY_IS_NULL(driveLiteral)) {
		Toy_printLiteral(scriptLiteral);

		Toy_freeLiteral(scriptLiteral);
		Toy_freeLiteral(driveLiteral);

		fatalError("Couldn't read null drive literal");
	}

	size_t size = 0;
	const unsigned char* source = Toy_readFile(Toy_toCString(TOY_AS_STRING(driveLiteral)), &size);

	if (!source) {
		Toy_printLiteral(driveLiteral);

		Toy_freeLiteral(scriptLiteral);
		Toy_freeLiteral(driveLiteral);

		fatalError("Couldn't read the given file");
	}

	Toy_freeLiteral(scriptLiteral);
	Toy_freeLiteral(driveLiteral);

	//compile the source to bytecode
	const unsigned char* tb = Toy_compileString((const char*)source, &size);
	free((void*)source);

	//BUGFIX: make an inner-interpreter for `init.toy` to remove globals
	Toy_Interpreter inner;

	//init the inner interpreter hooks manually
	inner.hooks = engine.interpreter.hooks;
	Toy_setInterpreterPrint(&inner, engine.interpreter.printOutput);
	Toy_setInterpreterAssert(&inner, engine.interpreter.assertOutput);
	Toy_setInterpreterError(&inner, engine.interpreter.errorOutput);
	inner.scope = Toy_pushScope(NULL);

	//run the inner interpreter
	Toy_runInterpreter(&inner, tb, size);

	//manual cleanup
	Toy_freeLiteralArray(&inner.stack);
	Toy_freeLiteralArray(&inner.literalCache);
	while(inner.scope != NULL) {
		inner.scope = Toy_popScope(inner.scope);
	}
}

void Box_freeEngine() {
	//clear existing root node
	if (engine.rootNode != NULL) {
		Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onFree", NULL);
		Box_freeNode(engine.rootNode);
		engine.rootNode = NULL;
	}

	if (!TOY_IS_NULL(engine.nextRootNodeFilename)) {
		Toy_freeLiteral(engine.nextRootNodeFilename);
	}

	Toy_freeInterpreter(&engine.interpreter);

	//free events
	Toy_freeLiteralDictionary(&engine.symKeyDownEvents);
	Toy_freeLiteralDictionary(&engine.symKeyUpEvents);

	//free the music
	if (engine.music != NULL) {
		Mix_HaltMusic();
		Mix_FreeMusic(engine.music);
	}

	//free SDL libs
	TTF_Quit();
	IMG_Quit();
	Mix_Quit();

	//free SDL
	SDL_DestroyRenderer(engine.renderer);
	SDL_DestroyWindow(engine.window);
	SDL_Quit();

	engine.renderer = NULL;
	engine.window = NULL;
}

static inline void execLoadRootNode() {
	//if a new root node is NOT needed, skip out
	if (TOY_IS_NULL(engine.nextRootNodeFilename)) {
		return;
	}

	//free the existing root node
	if (engine.rootNode != NULL) {
		Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onFree", NULL);
		Box_freeNode(engine.rootNode);
		engine.rootNode = NULL;
	}

	//compile the new root node
	size_t size = 0;
	const unsigned char* source = Toy_readFile(Toy_toCString(TOY_AS_STRING(engine.nextRootNodeFilename)), &size);
	const unsigned char* tb = Toy_compileString((const char*)source, &size);
	free((void*)source);

	//allocate the new root node
	engine.rootNode = TOY_ALLOCATE(Box_Node, 1);

	//BUGFIX: make an inner-interpreter
	Toy_Interpreter inner;

	//init the inner interpreter manually
	Toy_initLiteralArray(&inner.literalCache);
	inner.scope = Toy_pushScope(engine.interpreter.scope);
	inner.bytecode = tb;
	inner.length = (int)size;
	inner.count = 0;
	inner.codeStart = -1;
	inner.depth = engine.interpreter.depth + 1;
	inner.panic = false;
	Toy_initLiteralArray(&inner.stack);
	inner.hooks = engine.interpreter.hooks;
	Toy_setInterpreterPrint(&inner, engine.interpreter.printOutput);
	Toy_setInterpreterAssert(&inner, engine.interpreter.assertOutput);
	Toy_setInterpreterError(&inner, engine.interpreter.errorOutput);

	Box_initNode(engine.rootNode, &inner, tb, size);

	//immediately call onLoad() after running the script - for loading other nodes
	Box_callNode(engine.rootNode, &inner, "onLoad", NULL);

	//cache the scope for later freeing
	engine.rootNode->scope = inner.scope;

	//manual cleanup
	Toy_freeLiteralArray(&inner.stack);
	Toy_freeLiteralArray(&inner.literalCache);

	//cleanup
	Toy_freeLiteral(engine.nextRootNodeFilename);
	engine.nextRootNodeFilename = TOY_TO_NULL_LITERAL;

	//init the new node-tree
	Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onInit", NULL);
}

static inline void execEvents() {
	Toy_LiteralArray args; //save some allocation by reusing this
	Toy_initLiteralArray(&args);

	//poll all events
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			//quit
			case SDL_QUIT: {
				engine.running = false;
			}
			break;

			//window events are handled internally
			case SDL_WINDOWEVENT: {
				switch(event.window.event) {
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						//TODO: toy onWindowResized, setLogicalWindowSize, getLogicalWindowSize
						// engine.screenWidth = event.window.data1;
						// engine.screenHeight = event.window.data2;
						// SDL_RenderSetLogicalSize(engine.renderer, engine.screenWidth, engine.screenHeight);
					break;
				}
			}
			break;

			//input
			case SDL_KEYDOWN: {
				//bugfix: ignore repeat messages
				if (event.key.repeat) {
					break;
				}

				//determine the given keycode
				Toy_Literal keycodeLiteral = TOY_TO_INTEGER_LITERAL( (int)(event.key.keysym.sym) );
				if (!Toy_existsLiteralDictionary(&engine.symKeyDownEvents, keycodeLiteral)) {
					Toy_freeLiteral(keycodeLiteral);
					break;
				}

				//get the event name
				Toy_Literal eventLiteral = Toy_getLiteralDictionary(&engine.symKeyDownEvents, keycodeLiteral);

				//call the function
				Toy_pushLiteralArray(&args, eventLiteral);
				Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onKeyDown", &args);
				Toy_freeLiteral(Toy_popLiteralArray(&args));

				//push to the event list
				Toy_freeLiteral(eventLiteral);
				Toy_freeLiteral(keycodeLiteral);
			}
			break;

			case SDL_KEYUP: {
				//bugfix: ignore repeat messages
				if (event.key.repeat) {
					break;
				}

				//determine the given keycode
				Toy_Literal keycodeLiteral = TOY_TO_INTEGER_LITERAL( (int)(event.key.keysym.sym) );
				if (!Toy_existsLiteralDictionary(&engine.symKeyUpEvents, keycodeLiteral)) {
					Toy_freeLiteral(keycodeLiteral);
					break;
				}

				//get the event name
				Toy_Literal eventLiteral = Toy_getLiteralDictionary(&engine.symKeyUpEvents, keycodeLiteral);

				//call the function
				Toy_pushLiteralArray(&args, eventLiteral);
				Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onKeyUp", &args);
				Toy_freeLiteral(Toy_popLiteralArray(&args));

				//push to the event list
				Toy_freeLiteral(eventLiteral);
				Toy_freeLiteral(keycodeLiteral);
			}
			break;

			//mouse motion
			case SDL_MOUSEMOTION: {
				Toy_Literal mouseX = TOY_TO_INTEGER_LITERAL( (int)(event.motion.x) );
				Toy_Literal mouseY = TOY_TO_INTEGER_LITERAL( (int)(event.motion.y) );
				Toy_Literal mouseXRel = TOY_TO_INTEGER_LITERAL( (int)(event.motion.xrel) );
				Toy_Literal mouseYRel = TOY_TO_INTEGER_LITERAL( (int)(event.motion.yrel) );

				Toy_pushLiteralArray(&args, mouseX);
				Toy_pushLiteralArray(&args, mouseY);
				Toy_pushLiteralArray(&args, mouseXRel);
				Toy_pushLiteralArray(&args, mouseYRel);

				Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onMouseMotion", &args);

				Toy_freeLiteral(mouseX);
				Toy_freeLiteral(mouseY);
				Toy_freeLiteral(mouseXRel);
				Toy_freeLiteral(mouseYRel);

				//hack: manual free
				for(int i = 0; i < args.count; i++) {
					Toy_freeLiteral(args.literals[i]);
				}
				args.count = 0;
			}
			break;

			//mouse button down
			case SDL_MOUSEBUTTONDOWN: {
				Toy_Literal mouseX = TOY_TO_INTEGER_LITERAL( (int)(event.button.x) );
				Toy_Literal mouseY = TOY_TO_INTEGER_LITERAL( (int)(event.button.y) );
				Toy_Literal mouseButton;

				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("left"));
						break;

					case SDL_BUTTON_MIDDLE:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("middle"));
						break;

					case SDL_BUTTON_RIGHT:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("right"));
						break;

					case SDL_BUTTON_X1:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("x1"));
						break;

					case SDL_BUTTON_X2:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("x2"));
						break;

					default:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("unknown"));
						break;
				}

				Toy_pushLiteralArray(&args, mouseX);
				Toy_pushLiteralArray(&args, mouseY);
				Toy_pushLiteralArray(&args, mouseButton);

				Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onMouseButtonDown", &args);

				Toy_freeLiteral(mouseX);
				Toy_freeLiteral(mouseY);
				Toy_freeLiteral(mouseButton);

				//hack: manual free
				for(int i = 0; i < args.count; i++) {
					Toy_freeLiteral(args.literals[i]);
				}
				args.count = 0;
			}
			break;

			//mouse button up
			case SDL_MOUSEBUTTONUP: {
				Toy_Literal mouseX = TOY_TO_INTEGER_LITERAL( (int)(event.button.x) );
				Toy_Literal mouseY = TOY_TO_INTEGER_LITERAL( (int)(event.button.y) );
				Toy_Literal mouseButton;

				switch (event.button.button) {
					case SDL_BUTTON_LEFT:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("left"));
						break;

					case SDL_BUTTON_MIDDLE:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("middle"));
						break;

					case SDL_BUTTON_RIGHT:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("right"));
						break;

					case SDL_BUTTON_X1:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("x1"));
						break;

					case SDL_BUTTON_X2:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("x2"));
						break;

					default:
						mouseButton = TOY_TO_STRING_LITERAL(Toy_createRefString("unknown"));
						break;
				}

				Toy_pushLiteralArray(&args, mouseX);
				Toy_pushLiteralArray(&args, mouseY);
				Toy_pushLiteralArray(&args, mouseButton);

				Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onMouseButtonUp", &args);

				Toy_freeLiteral(mouseX);
				Toy_freeLiteral(mouseY);
				Toy_freeLiteral(mouseButton);

				//hack: manual free
				for(int i = 0; i < args.count; i++) {
					Toy_freeLiteral(args.literals[i]);
				}
				args.count = 0;
			}
			break;

			//mouse wheel
			case SDL_MOUSEWHEEL: {
				Toy_Literal mouseX = TOY_TO_INTEGER_LITERAL( (int)(event.wheel.x) );
				Toy_Literal mouseY = TOY_TO_INTEGER_LITERAL( (int)(event.wheel.y) );
				Toy_pushLiteralArray(&args, mouseX);
				Toy_pushLiteralArray(&args, mouseY);

				Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onMouseWheel", &args);

				Toy_freeLiteral(mouseX);
				Toy_freeLiteral(mouseY);

				//hack: manual free
				for(int i = 0; i < args.count; i++) {
					Toy_freeLiteral(args.literals[i]);
				}
				args.count = 0;
			}
			break;
		}
	}

	Toy_freeLiteralArray(&args);
}

static inline void execStep() {
	if (engine.rootNode != NULL) {
		//move nodes first, so collisions can be checked in code
		Box_movePositionByMotionRecursiveNode(engine.rootNode);

		//steps
		Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onStep", NULL);
	}
}

static inline void execUpdate(int deltaTime) {
	if (engine.rootNode != NULL) {
		//create the args
		Toy_Literal deltaLiteral = TOY_TO_INTEGER_LITERAL(deltaTime);

		Toy_LiteralArray args;
		Toy_initLiteralArray(&args);
		Toy_pushLiteralArray(&args, deltaLiteral);

		Toy_freeLiteral(deltaLiteral);

		//updates
		Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onUpdate", &args);

		//free
		Toy_freeLiteralArray(&args);
	}
}

//the heart of the engine
void Box_execEngine() {
	if (!engine.running) {
		fatalError("Can't execute the engine (did you forget to initialize the screen?)");
	}

	//set up time
	engine.realTime = SDL_GetTicks();
	engine.simTime = engine.realTime;
	engine.deltaTime = 0;
	const int fixedStep = 1000 / 60;

	Dbg_Timer dbgTimer;
	Dbg_FPSCounter fps;

	Dbg_initTimer(&dbgTimer);
	Dbg_initFPSCounter(&fps);

	while (engine.running) {
		Dbg_tickFPSCounter(&fps);

		Dbg_clearConsole();
		Dbg_printTimerLog(&dbgTimer);
		Dbg_printFPSCounter(&fps);

		Dbg_startTimer(&dbgTimer, "execLoadRootNode()");
		execLoadRootNode();
		Dbg_stopTimer(&dbgTimer);

		//calc the time values
		const int lastRealTime = engine.realTime;
		engine.realTime = SDL_GetTicks();
		engine.deltaTime = engine.realTime - lastRealTime;

		Dbg_startTimer(&dbgTimer, "onFrameStart()");
		Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onFrameStart", NULL);
		Dbg_stopTimer(&dbgTimer);

		//execute events
		Dbg_startTimer(&dbgTimer, "execEvents()");
		execEvents();
		Dbg_stopTimer(&dbgTimer);

		//execute update
		Dbg_startTimer(&dbgTimer, "execUpdate() (variable-delta)");
		execUpdate(engine.deltaTime);
		Dbg_stopTimer(&dbgTimer);

		//execute fixed steps
		Dbg_startTimer(&dbgTimer, "execStep() (fixed-delta)");
		//while not enough time has passed
		while(engine.simTime < engine.realTime) {
			//simulate the world
			execStep();

			//calc the time simulation
			engine.simTime += fixedStep;
		}
		Dbg_stopTimer(&dbgTimer);

		//render the world
		Dbg_startTimer(&dbgTimer, "screen clear");
		SDL_SetRenderDrawColor(engine.renderer, 0, 0, 0, 255); //NOTE: This line can be disabled later
		SDL_RenderClear(engine.renderer); //NOTE: This line can be disabled later
		Dbg_stopTimer(&dbgTimer);

		Dbg_startTimer(&dbgTimer, "onDraw()");
		Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onDraw", NULL);
		Dbg_stopTimer(&dbgTimer);

		Dbg_startTimer(&dbgTimer, "screen render");
		SDL_RenderPresent(engine.renderer);
		Dbg_stopTimer(&dbgTimer);

		Dbg_startTimer(&dbgTimer, "onFrameEnd()");
		Box_callRecursiveNode(engine.rootNode, &engine.interpreter, "onFrameEnd", NULL);
		Dbg_stopTimer(&dbgTimer);

		SDL_Delay(10);
	}

	Dbg_freeTimer(&dbgTimer);
	Dbg_freeFPSCounter(&fps);
}

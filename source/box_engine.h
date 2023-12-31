#pragma once

#include "box_common.h"
#include "box_node.h"

#include "toy_interpreter.h"
#include "toy_literal_array.h"
#include "toy_literal_dictionary.h"

//the base engine object, which represents the state of the game
typedef struct Box_private_engine {
	//engine stuff
	Box_Node* rootNode;
	Toy_Literal nextRootNodeFilename;
	int simTime;
	int realTime; //also used as deltaTick
	int deltaTime;
	bool running;

	//Toy stuff
	Toy_Interpreter interpreter;

	//SDL stuff
	SDL_Window* window;
	SDL_Renderer* renderer;
	int screenWidth;
	int screenHeight;

	//SDL_mixer stuff
	Mix_Music* music;

	//input syms mapped to events
	Toy_LiteralDictionary symKeyDownEvents; //keysym -> event names
	Toy_LiteralDictionary symKeyUpEvents; //keysym -> event names
} Box_Engine;

//extern singleton - used by various libraries
extern Box_Engine engine;

//APIs for running the engine in main()
BOX_API void Box_initEngine(const char* initScript);
BOX_API void Box_execEngine();
BOX_API void Box_freeEngine();


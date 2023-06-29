#pragma once

#include "box_common.h"

#include "toy_literal_dictionary.h"
#include "toy_interpreter.h"

#define OPAQUE_TAG_NODE 1001

//forward declare
typedef struct Box_private_node Box_Node;

//the node object, which forms a tree
typedef struct Box_private_node {
	//BUGFIX: hold the node's root scope so it can be popped
	Toy_Scope* scope; //used by nativeLoadNode

	//toy functions, stored in a dict for flexibility
	Toy_LiteralDictionary* functions;

	//cache the parent pointer for fast access
	Box_Node* parent;

	//use Toy's memory model
	Box_Node** children;
	int capacity;
	int count; //includes tombstones
	int childCount;

	//rendering-specific features
	SDL_Texture* texture;
	SDL_Rect rect; //rendered rect
	int frames; //horizontal-strip based animations
	int currentFrame;

	//position & motion, relative to my parent
	int positionX;
	int positionY;
	int motionX;
	int motionY;
	float scaleX;
	float scaleY;

	//sorting layer
	int layer;
} Box_Node;

BOX_API void Box_initNode(Box_Node* node, Toy_Interpreter* interpreter, const unsigned char* tb, size_t size); //run bytecode, then grab all top-level function literals
BOX_API void Box_pushNode(Box_Node* node, Box_Node* child); //push to the array (prune tombstones when expanding/copying)
BOX_API void Box_freeNode(Box_Node* node); //free this node and all children

BOX_API Box_Node* Box_getChildNode(Box_Node* node, int index); //NOTE: indexes are no longer valid after sorting
BOX_API void Box_freeChildNode(Box_Node* node, int index);

BOX_API void Box_sortChildrenNode(Box_Node* node, Toy_Interpreter* interpreter, Toy_Literal fnCompare);

BOX_API Toy_Literal Box_callNodeLiteral(Box_Node* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args);
BOX_API Toy_Literal Box_callNode(Box_Node* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args); //call "fnName" on this node, and only this node, if it exists

//for calling various lifecycle functions
BOX_API void Box_callRecursiveNodeLiteral(Box_Node* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args);
BOX_API void Box_callRecursiveNode(Box_Node* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args); //call "fnName" on this node, and all children, if it exists

BOX_API int Box_getChildCountNode(Box_Node* node);

BOX_API int Box_loadTextureNode(Box_Node* node, const char* fname);
BOX_API void Box_freeTextureNode(Box_Node* node);

BOX_API void Box_setRectNode(Box_Node* node, SDL_Rect rect);
BOX_API SDL_Rect Box_getRectNode(Box_Node* node);

BOX_API void Box_setFramesNode(Box_Node* node, int frames);
BOX_API int Box_getFramesNode(Box_Node* node);
BOX_API void Box_setCurrentFrameNode(Box_Node* node, int currentFrame);
BOX_API int Box_getCurrentFrameNode(Box_Node* node);
BOX_API void Box_incrementCurrentFrame(Box_Node* node);

//position in the world
BOX_API void Box_setPositionXNode(Box_Node* node, int x);
BOX_API void Box_setPositionYNode(Box_Node* node, int y);
BOX_API void Box_setMotionXNode(Box_Node* node, int x);
BOX_API void Box_setMotionYNode(Box_Node* node, int y);
BOX_API void Box_setScaleXNode(Box_Node* node, float sx);
BOX_API void Box_setScaleYNode(Box_Node* node, float sy);

BOX_API int Box_getPositionXNode(Box_Node* node);
BOX_API int Box_getPositionYNode(Box_Node* node);
BOX_API int Box_getMotionXNode(Box_Node* node);
BOX_API int Box_getMotionYNode(Box_Node* node);
BOX_API float Box_getScaleXNode(Box_Node* node);
BOX_API float Box_getScaleYNode(Box_Node* node);

BOX_API int Box_getWorldPositionXNode(Box_Node* node);
BOX_API int Box_getWorldPositionYNode(Box_Node* node);
BOX_API int Box_getWorldMotionXNode(Box_Node* node);
BOX_API int Box_getWorldMotionYNode(Box_Node* node);
BOX_API float Box_getWorldScaleXNode(Box_Node* node);
BOX_API float Box_getWorldScaleYNode(Box_Node* node);

BOX_API void Box_movePositionByMotionNode(Box_Node* node);
BOX_API void Box_movePositionByMotionRecursiveNode(Box_Node* node);

//sorting layer
BOX_API void Box_setLayerNode(Box_Node* node, int layer);
BOX_API int Box_getLayerNode(Box_Node* node);

//utilities
BOX_API void Box_setTextNode(Box_Node* node, TTF_Font* font, const char* text, SDL_Color color);

BOX_API void Box_drawNode(Box_Node* node, SDL_Rect dest);

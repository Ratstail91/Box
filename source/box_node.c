#include "box_node.h"
#include "box_engine.h"

#include "toy_memory.h"

void Box_initNode(Box_Node* node, Toy_Interpreter* interpreter, const unsigned char* tb, size_t size) {
	//init
	node->scope = NULL;
	node->functions = TOY_ALLOCATE(Toy_LiteralDictionary, 1);
	node->parent = NULL;
	node->children = NULL;
	node->capacity = 0;
	node->count = 0;
	node->childCount = 0;
	node->texture = NULL;
	node->rect = ((SDL_Rect) { 0, 0, 0, 0 });
	node->frames = 0;
	node->currentFrame = 0;
	node->positionX = 0;
	node->positionY = 0;
	node->motionX = 0;
	node->motionY = 0;
	node->scaleX = 1.0f;
	node->scaleY = 1.0f;
	node->layer = 0;

	Toy_initLiteralDictionary(node->functions);

	//skip empty nodes
	if (tb == NULL) {
		return;
	}

	//run bytecode
	Toy_runInterpreter(interpreter, tb, size);

	//grab all top-level functions from the dirty interpreter
	Toy_LiteralDictionary* variablesPtr = &interpreter->scope->variables;

	for (int i = 0; i < variablesPtr->capacity; i++) {
		//skip empties and tombstones
		if (TOY_IS_NULL(variablesPtr->entries[i].key)) {
			continue;
		}

		//if this variable is a function
		Toy_private_dictionary_entry* entry = &variablesPtr->entries[i];
		if (TOY_IS_FUNCTION(entry->value)) {
			//save a copy
			Toy_setLiteralDictionary(node->functions, entry->key, entry->value);
		}
	}
}

void Box_pushNode(Box_Node* node, Box_Node* child) {
	//push to the array
	if (node->count + 1 > node->capacity) {
		int oldCapacity = node->capacity;

		node->capacity = TOY_GROW_CAPACITY(oldCapacity);
		node->children = TOY_GROW_ARRAY(Box_Node*, node->children, oldCapacity, node->capacity);
	}

	//assign
	node->children[node->count++] = child;

	//reverse-assign
	child->parent = node;

	//count
	node->childCount++;
}

void Box_freeNode(Box_Node* node) {
	if (node == NULL) {
		return; //NO-OP
	}

	//free this node's children
	for (int i = 0; i < node->count; i++) {
		Box_freeNode(node->children[i]);
	}

	//free the pointer array to the children
	TOY_FREE_ARRAY(Box_Node*, node->children, node->capacity);

	if (node->functions != NULL) {
		Toy_freeLiteralDictionary(node->functions);
		TOY_FREE(Toy_LiteralDictionary, node->functions);
	}

	if (node->scope != NULL) {
		Toy_popScope(node->scope);
	}

	if (node->texture != NULL) {
		Box_freeTextureNode(node);
	}

	//free this node's memory
	TOY_FREE(Box_Node, node);
}

Box_Node* Box_getChildNode(Box_Node* node, int index) {
	if (index < 0 || index > node->count) {
		return NULL;
	}

	return node->children[index];
}

void Box_freeChildNode(Box_Node* node, int index) {
	//get the child node
	Box_Node* childNode = node->children[index];

	//free the node
	if (childNode != NULL) {
		Box_freeNode(childNode);
		node->childCount--;
	}

	node->children[index] = NULL;
}

static void swapUtil(Box_Node** lhs, Box_Node** rhs) {
	Box_Node* tmp = *lhs;
	*lhs = *rhs;
	*rhs = tmp;
}

//copied from lib_standard.c
static void recursiveLiteralQuicksortUtil(Toy_Interpreter* interpreter, Box_Node** ptr, int count, Toy_Literal fnCompare) {
	//base case
	if (count <= 1) {
		return;
	}

	int runner = 0;

	//iterate through the array
	for (int checker = 0; checker < count - 1; checker++) {
		//if node is null, it is always "sorted" to the end
		while (ptr[checker] == NULL && count > 0) {
			swapUtil(&ptr[checker], &ptr[count - 1]);
			count--;
		}

		//base case
		if (count < 2) {
			return;
		}

		//check for sorting layers (lower layers MUST come first)
		if (ptr[checker]->layer < ptr[runner]->layer) {
			swapUtil(&ptr[checker], &ptr[runner++]);
			continue;
		}

		Toy_LiteralArray arguments;
		Toy_LiteralArray returns;

		Toy_initLiteralArray(&arguments);
		Toy_initLiteralArray(&returns);

		Toy_pushLiteralArray(&arguments, TOY_TO_OPAQUE_LITERAL(ptr[checker], BOX_OPAQUE_TAG_NODE));
		Toy_pushLiteralArray(&arguments, TOY_TO_OPAQUE_LITERAL(ptr[count - 1], BOX_OPAQUE_TAG_NODE));

		Toy_callLiteralFn(interpreter, fnCompare, &arguments, &returns);

		Toy_Literal lessThan = Toy_popLiteralArray(&returns);

		Toy_freeLiteralArray(&arguments);
		Toy_freeLiteralArray(&returns);

		if (TOY_IS_TRUTHY(lessThan)) {
			swapUtil(&ptr[runner++], &ptr[checker]);
		}

		Toy_freeLiteral(lessThan);
	}

	//"shift everything up" so the pivot is in the middle
	swapUtil(&ptr[runner], &ptr[count - 1]);

	//recurse on each end
	recursiveLiteralQuicksortUtil(interpreter, &ptr[0], runner, fnCompare);
	recursiveLiteralQuicksortUtil(interpreter, &ptr[runner + 1], count - runner - 1, fnCompare);
}

BOX_API void Box_sortChildrenNode(Box_Node* node, Toy_Interpreter* interpreter, Toy_Literal fnCompare) {
	//check that this node's children aren't already sorted
	bool sorted = true;
	for (int checker = 0; checker < node->count - 1 && sorted; checker++) {
		//NULL (tombstone) is always considered unsorted
		if (node->children[checker] == NULL || node->children[checker + 1] == NULL) {
			sorted = false;
			break;
		}

		//check for sorting layers (lower layers MUST come first)
		if (node->children[checker]->layer > node->children[checker + 1]->layer) {
			sorted = false;
			break;
		}

		Toy_LiteralArray arguments;
		Toy_LiteralArray returns;

		Toy_initLiteralArray(&arguments);
		Toy_initLiteralArray(&returns);

		Toy_pushLiteralArray(&arguments, TOY_TO_OPAQUE_LITERAL(node->children[checker], BOX_OPAQUE_TAG_NODE));
		Toy_pushLiteralArray(&arguments, TOY_TO_OPAQUE_LITERAL(node->children[checker + 1], BOX_OPAQUE_TAG_NODE));

		Toy_callLiteralFn(interpreter, fnCompare, &arguments, &returns);

		Toy_Literal lessThan = Toy_popLiteralArray(&returns);

		Toy_freeLiteralArray(&arguments);
		Toy_freeLiteralArray(&returns);

		if (!TOY_IS_TRUTHY(lessThan)) {
			sorted = false;
		}

		Toy_freeLiteral(lessThan);
	}

	//sort the children
	if (!sorted) {
		recursiveLiteralQuicksortUtil(interpreter, node->children, node->count, fnCompare);
	}

	//re-count the newly-sorted children
	for (int i = node->count - 1; node->children[i] == NULL; i--) {
		node->count--;
	}
}

Toy_Literal Box_callNodeLiteral(Box_Node* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args) {
	Toy_Literal ret = TOY_TO_NULL_LITERAL;

	//if this fn exists
	if (Toy_existsLiteralDictionary(node->functions, key)) {
		Toy_Literal fn = Toy_getLiteralDictionary(node->functions, key);
		Toy_Literal n = TOY_TO_OPAQUE_LITERAL(node, BOX_OPAQUE_TAG_NODE);

		Toy_LiteralArray arguments;
		Toy_LiteralArray returns;
		Toy_initLiteralArray(&arguments);
		Toy_initLiteralArray(&returns);

		//feed the arguments in
		Toy_pushLiteralArray(&arguments, n);

		if (args) {
			for (int i = 0; i < args->count; i++) {
				Toy_pushLiteralArray(&arguments, args->literals[i]);
			}
		}

		Toy_callLiteralFn(interpreter, fn, &arguments, &returns);

		ret = Toy_popLiteralArray(&returns);

		Toy_freeLiteralArray(&arguments);
		Toy_freeLiteralArray(&returns);

		Toy_freeLiteral(n);
		Toy_freeLiteral(fn);
	}

	return ret;
}

Toy_Literal Box_callNode(Box_Node* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args) {
	//call "fnName" on this node, and all children, if it exists
	Toy_Literal key = TOY_TO_IDENTIFIER_LITERAL(Toy_createRefString(fnName));

	Toy_Literal ret = Box_callNodeLiteral(node, interpreter, key, args);

	Toy_freeLiteral(key);

	return ret;
}

void Box_callRecursiveNodeLiteral(Box_Node* node, Toy_Interpreter* interpreter, Toy_Literal key, Toy_LiteralArray* args) {
	//if this fn exists
	if (Toy_existsLiteralDictionary(node->functions, key)) {
		Toy_Literal fn = Toy_getLiteralDictionary(node->functions, key);
		Toy_Literal n = TOY_TO_OPAQUE_LITERAL(node, BOX_OPAQUE_TAG_NODE);

		Toy_LiteralArray arguments;
		Toy_LiteralArray returns;
		Toy_initLiteralArray(&arguments);
		Toy_initLiteralArray(&returns);

		//feed the arguments in
		Toy_pushLiteralArray(&arguments, n);

		if (args) {
			for (int i = 0; i < args->count; i++) {
				Toy_pushLiteralArray(&arguments, args->literals[i]);
			}
		}

		Toy_callLiteralFn(interpreter, fn, &arguments, &returns);

		Toy_freeLiteralArray(&arguments);
		Toy_freeLiteralArray(&returns);

		Toy_freeLiteral(n);
		Toy_freeLiteral(fn);
	}

	//recurse to the (non-tombstone) children
	for (int i = 0; i < node->count; i++) {
		if (node->children[i] != NULL) {
			Box_callRecursiveNodeLiteral(node->children[i], interpreter, key, args);
		}
	}
}

void Box_callRecursiveNode(Box_Node* node, Toy_Interpreter* interpreter, const char* fnName, Toy_LiteralArray* args) {
	//call "fnName" on this node, and all children, if it exists
	Toy_Literal key = TOY_TO_IDENTIFIER_LITERAL(Toy_createRefString(fnName));

	Box_callRecursiveNodeLiteral(node, interpreter, key, args);

	Toy_freeLiteral(key);
}

int Box_getChildCountNode(Box_Node* node) {
	return node->childCount;
}

BOX_API int Box_createTextureNode(Box_Node* node, int width, int height) {
	node->texture = SDL_CreateTexture(engine.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);

	if (node->texture == NULL) {
		return -1;
	}

	//get the rect
	int w, h;
	SDL_QueryTexture(node->texture, NULL, NULL, &w, &h);
	SDL_Rect r = { 0, 0, w, h };
	Box_setRectNode(node, r);
	Box_setFramesNode(node, 1); //default

	return 0;
}

int Box_loadTextureNode(Box_Node* node, const char* fname) {
	SDL_Surface* surface = IMG_Load(fname);

	if (surface == NULL) {
		return -1;
	}

	node->texture = SDL_CreateTextureFromSurface(engine.renderer, surface);

	if (node->texture == NULL) {
		return -2;
	}

	SDL_FreeSurface(surface);

	int w, h;
	SDL_QueryTexture(node->texture, NULL, NULL, &w, &h);
	SDL_Rect r = { 0, 0, w, h };
	Box_setRectNode(node, r);
	Box_setFramesNode(node, 1); //default

	return 0;
}

void Box_freeTextureNode(Box_Node* node) {
	if (node->texture != NULL) {
		SDL_DestroyTexture(node->texture);
		node->texture = NULL;
	}
}

void Box_setRectNode(Box_Node* node, SDL_Rect rect) {
	node->rect = rect;
}

SDL_Rect Box_getRectNode(Box_Node* node) {
	return node->rect;
}

void Box_setFramesNode(Box_Node* node, int frames) {
	node->frames = frames;
	node->currentFrame = 0; //just in case
}

int Box_getFramesNode(Box_Node* node) {
	return node->frames;
}

void Box_setCurrentFrameNode(Box_Node* node, int currentFrame) {
	node->currentFrame = currentFrame;
}

int Box_getCurrentFrameNode(Box_Node* node) {
	return node->currentFrame;
}

void Box_incrementCurrentFrame(Box_Node* node) {
	node->currentFrame++;
	if (node->currentFrame >= node->frames) {
		node->currentFrame = 0;
	}
}

void Box_setPositionXNode(Box_Node* node, int x) {
	node->positionX = x;
}

void Box_setPositionYNode(Box_Node* node, int y) {
	node->positionY = y;
}

void Box_setMotionXNode(Box_Node* node, int x) {
	node->motionX = x;
}
void Box_setMotionYNode(Box_Node* node, int y) {
	node->motionY = y;
}

void Box_setScaleXNode(Box_Node* node, float sx) {
	node->scaleX = sx;
}

void Box_setScaleYNode(Box_Node* node, float sy) {
	node->scaleY = sy;
}

int Box_getPositionXNode(Box_Node* node) {
	return node->positionX;
}

int Box_getPositionYNode(Box_Node* node) {
	return node->positionY;
}

int Box_getMotionXNode(Box_Node* node) {
	return node->motionX;
}

int Box_getMotionYNode(Box_Node* node) {
	return node->motionY;
}

float Box_getScaleXNode(Box_Node* node) {
	return node->scaleX;
}

float Box_getScaleYNode(Box_Node* node) {
	return node->scaleY;
}

int Box_getWorldPositionXNode(Box_Node* node) {
	int result = 0;
	while(node != NULL) {
		result += node->positionX;
		node = node->parent;
	}
	return result;
}

int Box_getWorldPositionYNode(Box_Node* node) {
	int result = 0;
	while(node != NULL) {
		result += node->positionY;
		node = node->parent;
	}
	return result;
}

int Box_getWorldMotionXNode(Box_Node* node) {
	int result = 0;
	while(node != NULL) {
		result += node->motionX;
		node = node->parent;
	}
	return result;
}

int Box_getWorldMotionYNode(Box_Node* node) {
	int result = 0;
	while(node != NULL) {
		result += node->motionY;
		node = node->parent;
	}
	return result;
}

float Box_getWorldScaleXNode(Box_Node* node) {
	float result = 1;
	while(node != NULL) {
		result *= node->scaleX;
		node = node->parent;
	}
	return result;
}

float Box_getWorldScaleYNode(Box_Node* node) {
	float result = 1;
	while(node != NULL) {
		result *= node->scaleY;
		node = node->parent;
	}
	return result;
}

void Box_setLayerNode(Box_Node* node, int layer) {
	node->layer = layer;
}

int Box_getLayerNode(Box_Node* node) {
	return node->layer;
}

void Box_movePositionByMotionNode(Box_Node* node) {
	node->positionX += node->motionX;
	node->positionY += node->motionY;
}

void Box_movePositionByMotionRecursiveNode(Box_Node* node) {
	Box_movePositionByMotionNode(node);

	//recurse to the (non-tombstone) children
	for (int i = 0; i < node->count; i++) {
		if (node->children[i] != NULL) {
			Box_movePositionByMotionRecursiveNode(node->children[i]);
		}
	}
}

void Box_setTextNode(Box_Node* node, TTF_Font* font, const char* text, SDL_Color color) {
	SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);

	node->texture = SDL_CreateTextureFromSurface(engine.renderer, surface);

	node->rect = (SDL_Rect){ .x = 0, .y = 0, .w = surface->w, .h = surface->h };
	node->frames = 1;
	node->currentFrame = 0;

	SDL_FreeSurface(surface);
}


void Box_drawNode(Box_Node* node, SDL_Rect dest) {
	if (!node->texture) return;
	SDL_Rect src = node->rect;
	src.x += src.w * node->currentFrame;
	SDL_RenderCopy(engine.renderer, node->texture, &src, &dest);
}

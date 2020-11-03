#include <stdlib.h>

#include "config/node-visitor.h"
#include "utils.h"

struct cd_node_visitor {
	size_t *stack;
	size_t stack_len;
	size_t stack_alloc;
	struct config_data *config;
	unsigned init: 1;
};

void node_visitor_init(struct cd_node_visitor **visitor, struct config_data *config)
{
	*visitor = (struct cd_node_visitor *) malloc(sizeof(struct cd_node_visitor));
	if (!*visitor)
		FATAL(MEM_ALLOC_FAILED);

	if (config->parent)
		BUG("node_visitor must be initialized with root node");

	(*visitor)->stack_len = 0;
	(*visitor)->stack_alloc = 16;
	(*visitor)->stack = (size_t *)malloc((*visitor)->stack_alloc * sizeof(size_t));
	(*visitor)->config = config;
	(*visitor)->init = 0;
}

void node_visitor_release(struct cd_node_visitor *visitor)
{
	free(visitor->stack);
	free(visitor);
}

static void push_stack(struct cd_node_visitor *visitor, size_t index)
{
	if (visitor->stack_len == visitor->stack_alloc) {
		visitor->stack_alloc += 16;
		visitor->stack = (size_t *) realloc(visitor->stack, visitor->stack_alloc);
		if (!visitor->stack)
			FATAL(MEM_ALLOC_FAILED);
	}

	visitor->stack[visitor->stack_len] = index;
	visitor->stack_len++;
}

static size_t pop_stack(struct cd_node_visitor *visitor)
{
	if (!visitor->stack_len)
		BUG("stack underflow in config_data traversal");

	return visitor->stack[--visitor->stack_len];
}

int node_visitor_next(struct cd_node_visitor *visitor, struct config_data **next)
{
	struct config_data *current = visitor->config;

	// when function is called for the first time, return the root node
	if (!visitor->init) {
		visitor->init = 1;
		*next = current;
		return 0;
	}

	if (current->subsections_len) {
		// traverse into first subsection
		push_stack(visitor, 0);
		current = current->subsections[0];
	} else {
		do {
			// traverse to parent
			current = current->parent;
			if (!current)
				return 1;

			size_t index = pop_stack(visitor);
			index++;

			// does the parent have any subsections left to traverse?
			if (index < current->subsections_len) {
				// traverse next subsection
				push_stack(visitor, index);
				current = current->subsections[index];

				break;
			}
		} while (1);
	}

	*next = current;
	visitor->config = current;
	return 0;
}

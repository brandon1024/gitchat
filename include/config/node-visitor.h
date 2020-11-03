#ifndef GIT_CHAT_INCLUDE_CONFIG_NODE_VISITOR_H
#define GIT_CHAT_INCLUDE_CONFIG_NODE_VISITOR_H

#include "config/config-data.h"

struct cd_node_visitor;

/**
 * Initialize a config_data node visitor. `visitor` is updated with a context
 * handle. `config` must be the root config_data node.
 * */
void node_visitor_init(struct cd_node_visitor **visitor, struct config_data *config);

/**
 * Release visitor resources.
 * */
void node_visitor_release(struct cd_node_visitor *visitor);

/**
 * Retrieve the next node in the config_data tree, returning zero if successful
 * or non-zero if traversal is complete. `next` is updated with the next node
 * in the tree.
 *
 * Manipulating the config_data tree while a node traveral is in progress may
 * result in undefined behaviour.
 * */
int node_visitor_next(struct cd_node_visitor *visitor, struct config_data **next);

#endif //GIT_CHAT_INCLUDE_CONFIG_NODE_VISITOR_H

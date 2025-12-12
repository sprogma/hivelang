#ifndef TYPE_H
#define TYPE_H


#include "context.hpp"
#include "dynamic.hpp"
#include "code.hpp"


size_t get_type_size(struct context *ctx, type_id tid);
int type_is_integer(struct context *ctx, type_id tid);
type_id get_base_type(struct context *ctx, const char *name);
type_id get_complex_type(struct context *ctx, enum type_type type, type_id base);
type_id get_expr_type(struct context *ctx, struct expression *e);

#endif

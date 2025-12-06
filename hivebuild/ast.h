#ifndef AST
#define AST

#include "api.h"

#include "inttypes.h"


size_t size_of_type(struct type *t);
struct program *parse(const char *s);
struct type *get_expr_type(struct program *p, struct expression *e);
int is_lvalue(struct program *p, struct expression *e);
struct type *get_base_type(char *name);
struct type *get_complex_type(enum type_type type, struct type *base);

#endif

#ifndef EXPORT_H
#define EXPORT_H


#include "ast.h"


int export_program_text(struct program *program);
int export_program_json(struct program *program, const char *filename);
int export_program_bytes(struct program *program, const char *filename);


#endif

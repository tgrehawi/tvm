#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "tvmtype.h"
#include "tvmprog.h"

#define STARTCAP 16
#define EXPANDFACTOR 1.5f

#define chkpntr(p) if (p == NULL) fprintf(stderr, "Could not allocate memory.\n")

void paddi(program *p, instr i, uint *cap) {
	if (p->length == *cap) {
		*cap *= EXPANDFACTOR;
		p->instrs = realloc(p->instrs, *cap * sizeof(instr));
		chkpntr(p->instrs);
	}
	p->instrs[p->length++] = i;
}

char nextc(FILE *file, char *c, uint *line, uint *column) {
	*c = fgetc(file);
	if (*c == '`') {
		while ((*c = fgetc(file)) != '\n');
	}
	if (*c == '\n') {
		(*line) ++;
		*column = 0;
	}
	(*column) ++;
	return *c;
}

int fgeti(FILE *file, char *c, int *i, uint *line, uint *column) {
	int sign = 1;
		#define nextc() nextc(file, c, line, column)
	if (*c == '-') {
		sign = -1;
		nextc();
	}
	else if (*c == '+') {
		nextc();
	}
	if (isdigit(*c)) {
		*i = *c - '0';
		while (isdigit(nextc())) {
			*i *= 10;
			*i += *c - '0';
		}
		*i *= sign;
		return 0;
	}
	else {
		return -1;
	}
		#undef nextc
}

ploadret pload(program *_p, FILE *file) {
	program p = {NULL, 0};
	uint cap = STARTCAP;
	p.instrs = malloc(cap * sizeof(instr));
	chkpntr(p.instrs);
	uint line = 1;
	uint column = 0;
	char c;
		#define nextc() nextc(file, &c, &line, &column)
		#define nextnospace() while (isspace(nextc()))
		#define ploadret(type) ((ploadret) {type, line, column, c})
		#define paddi(i) paddi(&p, i, &cap)
		#define rerror(type) return (pfree(p), ploadret(type))
		#define fgeti(i) if (fgeti(file, &c, i, &line, &column) < 0) rerror(INVALID_INTEGER)
	instr i;
	nextc();
	while (c != EOF) {
		if (isspace(c))
			nextnospace();
		i.name = c;
		switch (i.name) {
			//these take an argument
			case PUSHC:
				nextnospace();
				if (c == '\\') {
					switch(nextc()) {
						case 'n': c = '\n'; break;
						case 'r': c = '\r'; break;
						case 't': c = '\t'; break;
						case 's': c = ' '; break;
						default:
							rerror(INVALID_ESCAPE);
					}
				}
				i.arg = c;
				nextc();
				break;
			case PUSHI:
				nextnospace();
				fgeti(&i.arg);
				break;
			//these take no argument
			case POP:
			case PCHAR:
			case PINT:
			case IADD:
			case ISUB:
			case IMUL:
			case IDIV:
			case IMOD:
				nextnospace();
				i.arg = 0;
				break;
			default:
				rerror(INVALID_INSTRUCTION);
		}
		// printf("%d:%d %c '%c' (%d)\n", line, column, i.name, i.arg, i.arg);
		paddi(i);
	}
	*_p = p;
	return ploadret(SUCCESS);
		#undef nextc
		#undef nextnospace
		#undef ploadret
		#undef paddi
		#undef rerror
		#undef fgeti
}


int prun(progstate *state, program p) {
		#define check(call, msg) if ((call) != 0) return (printf("\n** Error: %s **\n", msg), -1)
		#define ppush(i) check(ppush(state, i), "STACK OVERFLOW")
		#define ppop(i) check(ppop(state, i), "STACK UNDERFLOW")
		#define ppeek(i) check(ppeek(state, i), "STACK EMPTY")
	instr *pp = p.instrs;
	int a, b, c;
	while (pp - p.instrs < p.length) {
		switch (pp->name) {
			case PUSHI:
			case PUSHC:
				ppush(pp->arg);
				break;
			case POP:
				ppop(NULL);
				break;
			case PCHAR:
				ppeek(&c);
				putchar(c);
				break;
			case PINT:
				ppeek(&c);
				printf("%d", c);
				break;
			case IADD:
				ppop(&b);
				ppop(&a);
				ppush(a + b);
				break;
			case ISUB:
				ppop(&b);
				ppop(&a);
				ppush(a - b);
				break;
			case IMUL:
				ppop(&b);
				ppop(&a);
				ppush(a * b);
				break;
			case IDIV:
				ppop(&b);
				ppop(&a);
				ppush(a / b);
				break;
			case IMOD:
				ppop(&b);
				ppop(&a);
				ppush(a % b);
				break;
		}
		pp ++;
	}
	return 0;
		#undef check
		#undef ppush
		#undef ppop
		#undef ppeek
}

void pfree(program p) {
	free(p.instrs);
	p.length = 0;
}

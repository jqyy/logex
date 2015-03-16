#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAXIDENT 26

enum {
	TOK_FALSE = MAXIDENT + 1,
	TOK_TRUE,
	TOK_AND,
	TOK_OR,
	TOK_XOR,
	TOK_NOT,
	TOK_LPAREN,
	TOK_RPAREN,
};

#define isident(x) (0 <= (x) && (x) < MAXIDENT)

typedef struct {
	char *vector;
	int len;
} TruthTab;

typedef struct {
	char ident[MAXIDENT];
	char ident_cnt;
	char token;
	FILE *inputf;
	TruthTab *result;
} ParseState;

int fatal(const char *msg)
{
	fputs(msg, stderr);
	exit(EXIT_FAILURE);
	return 0;
}

void *ensure_memory(void *alloc)
{
	if (!alloc)
		fatal("out of memory\n");
	return alloc;
}

/************************************************
 * Scanner {                                    *
 ************************************************/

int get_id(ParseState *ps, int ch)
{
	int i;

	for (i = 0; i < ps->ident_cnt; i++)
		if (ps->ident[i] == ch)
			return i;
	assert(i == ps->ident_cnt && i < MAXIDENT);
	ps->ident[i] = ch;
	return ps->ident_cnt++;
}

int next_token(ParseState *ps)
{
	int ch;

entry:
	ch = fgetc(ps->inputf);
	switch (ch) {
		case ' ': case '\t': case '\r': case '\n':
			goto entry;
		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y':
		case 'Z':
			return ps->token = get_id(ps, ch);
		case '0':
			return ps->token = TOK_FALSE;
		case '1':
			return ps->token = TOK_TRUE;
		case '+': case '|':
			return ps->token = TOK_OR;
		case '*': case '&':
			return ps->token = TOK_AND;
		case '^':
			return ps->token = TOK_XOR;
		case '~': case '!':
			return ps->token = TOK_NOT;
		case '(':
			return ps->token = TOK_LPAREN;
		case ')':
			return ps->token = TOK_RPAREN;
		default:
			return ps->token = MAXIDENT;
	}
}

/************************************************
 * Scanner }                                    *
 ************************************************/

/************************************************
 * Truth Table Operations {                     *
 ************************************************/

#define MAX_TRUTHTAB_LEN (1<<MAXIDENT)

TruthTab *new_truth_tab(int len)
{
	TruthTab *t;

	assert(len < MAX_TRUTHTAB_LEN);
	t = ensure_memory(malloc(sizeof *t));
	t->vector = ensure_memory(calloc(len, 1));
	t->len = len;
	return t;
}

void free_truth_tab(TruthTab *t)
{
	free(t->vector);
	free(t);
}

#if 0
int expand_truth_tab(TruthTab *t)
{
	assert(t->len * 2 < MAX_TRUTHTAB_LEN);
	t->vector = ensure_memory(realloc(t->vector, t->len * 2));
	memcpy(t->vector + t->len, t->vector, t->len);
	return t->len *= 2;
}
#endif

TruthTab *constant(int k)
{
	TruthTab *t;

	t = ensure_memory(new_truth_tab(1));
	t->vector[0] = k;
	return t;
}

TruthTab *single_var(int ident)
{
	TruthTab *t;

	t = ensure_memory(new_truth_tab(1 << (ident+1)));
	memset(t->vector + (1<<ident), 1, 1<<ident);
	return t;
}

void inverse(TruthTab *t)
{
	int i;
	
	for (i = 0; i < t->len; i++) {
		t->vector[i] ^= 1;
	}
}

/* Caution: orignal lhs and rhs are invalid after calling this. */
TruthTab *binary_op(int op, TruthTab *lhs, TruthTab *rhs)
{
	int i;
	int rhsmask;

	if (lhs->len < rhs->len) {
		TruthTab *tmp = lhs;
		lhs = rhs;
		rhs = tmp;
	}
	rhsmask = rhs->len - 1;
	for (i = 0; i < lhs->len; i++) {
		switch (op) {
			case TOK_AND:
				lhs->vector[i] &= rhs->vector[i & rhsmask];
				break;
			case TOK_OR:
				lhs->vector[i] |= rhs->vector[i & rhsmask];
				break;
			case TOK_XOR:
				lhs->vector[i] ^= rhs->vector[i & rhsmask];
				break;
		}
	}
	free_truth_tab(rhs);
	return lhs;
}

/************************************************
 * Truth Table Operations }                     *
 ************************************************/

/************************************************
 * Parser {                                     *
 ************************************************/

TruthTab *parse_expr(ParseState *);
TruthTab *parse_term(ParseState *);
TruthTab *parse_factor(ParseState *);

void parse_expect(ParseState *ps, int token)
{
	if (ps->token != token) {
		fatal("syntax error\n");
	}
	next_token(ps);
}

void parse(ParseState *ps, FILE *inputf)
{
	ps->ident_cnt = 0;
	ps->inputf = inputf;
	next_token(ps);
	ps->result = parse_expr(ps);
	parse_expect(ps, MAXIDENT);
	assert(feof(ps->inputf));
}

/* expr -> term { OR term } */
TruthTab *parse_expr(ParseState *ps)
{
	TruthTab *lhs, *rhs;

	lhs = parse_term(ps);
	while (ps->token == TOK_OR) {
		parse_expect(ps, TOK_OR);
		rhs = parse_term(ps);
		lhs = binary_op(TOK_OR, lhs, rhs);
	}
	return lhs;
}

/* term -> factor { ([AND] | XOR) factor } */
TruthTab *parse_term(ParseState *ps)
{
	TruthTab *lhs, *rhs;

	lhs = parse_factor(ps);
	for (;;) {
		if (ps->token == TOK_AND) /* optional AND */
			parse_expect(ps, TOK_AND);
		else if (ps->token == TOK_XOR) {
			parse_expect(ps, TOK_XOR);
			rhs = parse_factor(ps);
			lhs = binary_op(TOK_XOR, lhs, rhs);
		}
		if (isident(ps->token)
			|| ps->token == TOK_TRUE
			|| ps->token == TOK_FALSE
			|| ps->token == TOK_NOT
			|| ps->token == TOK_LPAREN) {
			rhs = parse_factor(ps);
			lhs = binary_op(TOK_AND, lhs, rhs);
		} else {
			return lhs;
		}
	}
}

/* factor -> ID
 *        -> TRUE|FALSE
 *        -> LPAREN expr RPAREN
 *        -> { NOT } factor
 */
TruthTab *parse_factor(ParseState *ps)
{
	TruthTab *t;
	int not_cnt = 0;

	while (ps->token == TOK_NOT) {
		parse_expect(ps, TOK_NOT);
		not_cnt++;
	}
	if (isident(ps->token)) {
		t = single_var(ps->token);
		parse_expect(ps, ps->token);
	} else if (ps->token == TOK_FALSE) {
		t = constant(0);
		parse_expect(ps, TOK_FALSE);
	} else if (ps->token == TOK_TRUE) {
		t = constant(1);
		parse_expect(ps, TOK_TRUE);
	} else {
		parse_expect(ps, TOK_LPAREN);
		t = parse_expr(ps);
		parse_expect(ps, TOK_RPAREN);
	}
	if (not_cnt & 1) {
		inverse(t);
	}
	return t;
}

/************************************************
 * Parser }                                     *
 ************************************************/

void print_result(const ParseState *ps)
{
	int i;
	int line;

	/* print identifiers */
	for (i = 0; i < ps->ident_cnt; i++) {
		putchar(ps->ident[i]);
		putchar(' ');
	}
	printf("| result\n");
	/* rule */
	for (i = 0; i < ps->ident_cnt * 2; i++)
		putchar('-');
	printf("+-------\n");
	/* print lines */
	for (line = 0; line < ps->result->len; line++) {
		char *vector = ps->result->vector;

		for (i = 1; i < ps->result->len; i <<= 1) {
			putchar((line & i) ? '1' : '0');
			putchar(' ');
		}
		printf("| %6d\n", vector[line]);
	}
}

int main()
{
	ParseState ps;

	parse(&ps, stdin);
	print_result(&ps);
	free_truth_tab(ps.result);
	return 0;
}

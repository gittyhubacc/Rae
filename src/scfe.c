#include "scfe.h"
#include <stddef.h>
#include <stdio.h>

static int tkn_arr_eq(scfe_tkn_arr left, scfe_tkn_arr right)
{
        if (left.len != right.len) {
                return 0;
        }
        if (left.addr == right.addr) {
                return 1;
        }
        for (int i = 0; i < left.len; i++) {
                if (left.addr[i].type != right.addr[i].type) {
                        return 0;
                }
        }
        return 1;
}

static char hitmap[0xFF];
static scfe_tkn_arr tkn_arr_un(arena *a, scfe_tkn_arr left, scfe_tkn_arr right)
{ // tkn_arr_union
        mfmemset(hitmap, 0, 0xFF);

        scfe_tkn_arr res;
        res.len = 0;
        res.addr = make(a, scfe_tkn, left.len + right.len);
        for (int i = 0; i < left.len; i++) {
                if (!hitmap[left.addr[i].type]) {
                        res.addr[res.len++] = left.addr[i];
                        hitmap[left.addr[i].type] = 1;
                }
        }
        for (int i = 0; i < right.len; i++) {
                if (!hitmap[right.addr[i].type]) {
                        res.addr[res.len++] = right.addr[i];
                }
        }
        return res;
}

static int tkn_arr_member(scfe_tkn_arr s, scfe_tkn t)
{
        for (int i = 0; i < s.len; i++) {
                if (s.addr[i].type == t.type) {
                        return 1;
                }
        }
        return 0;
}

static void special_sauce(arena *a, scfe_pda_state *s, int type,
                          scfe_tkn_arr lookahead)
{
        scfe_pda_id *data;
        list *cursor = s->ids;
        while (cursor) {
                data = cursor->data;
                if (data->head.type == type && data->body.len == 1) {
                        data->look_ahead =
                            tkn_arr_un(a, data->look_ahead, lookahead);
                        special_sauce(a, s, data->body.addr[0].type,
                                      data->look_ahead);
                }
                cursor = cursor->next;
        }
}

// *copies* p into s->ids
static int append_if_absent(arena *a, scfe_pda_state *s, scfe_pda_id *p)
{
        scfe_pda_id *data;
        list *cursor = s->ids;
        while (cursor) {
                data = cursor->data;
                // printf("\tcmp: %i = %i, %i = %i, tknarreq = %i\n",
                // data->head.type, p->head.type, data->i, p->i,
                // tkn_arr_eq(data->body, p->body));
                if (data->head.type == p->head.type && data->i == p->i &&
                    tkn_arr_eq(data->body, p->body)) {
                        if (!tkn_arr_eq(data->look_ahead, p->look_ahead)) {
                                /*printf("\tlookalike: diff\n");
                                for (int a = 0; a < data->look_ahead.len; a++) {
                                        printf("\tdata[%i]: %i\n", a,
                                               data->look_ahead.addr[a].type);
                                }
                                for (int a = 0; a < p->look_ahead.len; a++) {
                                        printf("\tp[%i]: %i\n", a,
                                               p->look_ahead.addr[a].type);
                                }
                                */
                                data->look_ahead = tkn_arr_un(
                                    a, data->look_ahead, p->look_ahead);
                                /*
                                for (int a = 0; a < data->look_ahead.len; a++) {
                                        printf("\tres[%i]: %i\n", a,
                                               data->look_ahead.addr[a].type);
                                }
                                */
                                if (data->body.len == 1) {
                                        special_sauce(a, s,
                                                      data->body.addr[0].type,
                                                      data->look_ahead);
                                }
                        }
                        return 0;
                }
                cursor = cursor->next;
        }

        list *node = make(a, list, 1);
        node->data = make(a, scfe_pda_id, 1);
        *(scfe_pda_id *)node->data = *p;
        node->next = NULL;
        list_append(&s->ids, node);

        return 1;
}

static scfe_tkn_arr first_terminal(arena *a, scfe_grammar *g, scfe_tkn t)
{
        scfe_tkn_arr tmp;
        scfe_tkn_arr terminals = {.addr = 0, .len = 0};

        for (int i = 0; i < g->nonterminals.len; i++) {
                if (g->nonterminals.addr[i].head.type != t.type) {
                        continue;
                }

                // if first symbol is terminal, add to terminals
                if (g->nonterminals.addr[i].body.addr[0].type <
                    g->terminals.len) {
                        tmp.addr = make(a, scfe_tkn, 1);
                        tmp.addr[0] = g->nonterminals.addr[i].body.addr[0];
                        tmp.len = 1;
                        terminals = tkn_arr_un(a, terminals, tmp);
                } else if (g->nonterminals.addr[i].body.addr[0].type !=
                           t.type) {
                        tmp = first_terminal(
                            a, g, g->nonterminals.addr[i].body.addr[0]);
                        terminals = tkn_arr_un(a, terminals, tmp);
                }
        }

        return terminals;
}

static int append_pda_id_closure(arena *a, scfe_pda_state *s, scfe_grammar *g,
                                 scfe_pda_id *p)
{
        /*
#ifdef SCFE_DEBUG
        printf("closing %i <-", p->head.type);
        for (int i = 0; i < p->body.len; i++) {
                if (i == p->i) {
                        printf("[");
                }
                printf(" %i", p->body.addr[i].type);
                if (i == p->i) {
                        printf("]");
                }
        }
        printf(" [ ");
        for (int i = 0; i < p->look_ahead.len; i++) {
                printf("%i ", p->look_ahead.addr[i].type);
        }
        printf(" ]\n");
#endif
*/
        scfe_pda_id *new;
        int updated = 0;
        for (int i = 0; i < g->nonterminals.len; i++) {
                if (g->nonterminals.addr[i].head.type ==
                    p->body.addr[p->i].type) {
                        new = make(a, scfe_pda_id, 1);
                        new->i = 0;
                        new->rule = i;
                        new->head = g->nonterminals.addr[i].head;
                        // new->body = g->nonterminals.addr[i].body;
                        new->body.len = g->nonterminals.addr[i].body.len;
                        new->body.addr = make(a, scfe_tkn, new->body.len);
                        // we do a deep copy here (we were doing a shallow)
                        // because we eventually write to this memory when
                        // shifting to give the rules we create parse tree nodes
                        // references to the actual tokenized string data... :)
                        for (int j = 0; j < new->body.len; j++) {
                                new->body.addr[j].type =
                                    g->nonterminals.addr[i].body.addr[j].type;
                        }
                        if (p->i + 1 >= p->body.len) {
                                // inherit the look ahead
                                new->look_ahead = p->look_ahead;
                        } else if (p->body.addr[p->i + 1].type >
                                   g->terminals.len) {
                                // the next symbol is a nonterminal
                                new->look_ahead = first_terminal(
                                    a, g, p->body.addr[p->i + 1]);
                        } else {
                                // the next symbol is a terminal
                                new->look_ahead.len = 1;
                                new->look_ahead.addr = make(a, scfe_tkn, 1);
                                new->look_ahead.addr[0] =
                                    p->body.addr[p->i + 1];
                        }
                        updated |= append_if_absent(a, s, new);
                }
        }
        return updated;
}

static scfe_pda_state close_pda_id_list(arena *a, scfe_grammar *g,
                                        scfe_pda_state *s)
{
        scfe_pda_id *p;
        list *cursor;
        int updated = 1;
        scfe_pda_state res = {.ids = NULL};

        // copy what's present
        cursor = s->ids;
        while (cursor) {
                p = cursor->data;
                append_if_absent(a, &res, p);
                cursor = cursor->next;
        }

        // then close
        while (updated) {
                updated = 0;
                cursor = res.ids;
                while (cursor) {
                        p = cursor->data;
                        updated |= append_pda_id_closure(a, &res, g, p);
                        cursor = cursor->next;
                }
        }
        return res;
}

static const scfe_tkn_arr tkn_epsilon = {.len = 0, .addr = 0};
static scfe_pda_state create_start_state(arena *a, scfe_grammar *g)
{
        int start_i = g->nonterminals.len - 1;
        scfe_pda_rule *start = g->nonterminals.addr + start_i;
        scfe_pda_id start_pda_id = {
            .i = 0,
            .rule = start_i,
            .head = start->head,
            .body = start->body,
            .look_ahead = tkn_epsilon,
        };
        list start_id_list = {
            .data = &start_pda_id,
            .next = 0,
            .last = 0,
        };
        scfe_pda_state basis_state = {
            .ids = &start_id_list,
        };
        return close_pda_id_list(a, g, &basis_state);
}

static int is_terminal(scfe_grammar *g, scfe_tkn t)
{ // if terminal or over
        return t.type <= g->terminals.len;
}

// TO DO: opportunity to save memory here, switching of states
static int shift(arena *a, scfe_parser *p, scfe_grammar *g, scfe_tkn t)
{
        scfe_pda_id *new;
        scfe_pda_id *data;
        scfe_pda_state *cur_state;
        scfe_pda_state *new_state;
        list *cursor;

#ifdef SCFE_DEBUG
        printf("shifting: %i (%.*s)\n", t.type, t.sdata.len, t.sdata.addr);
#endif

        new_state = make(a, scfe_pda_state, 1);
        cur_state = &p->state_stack[p->state_i - 1];
        cursor = cur_state->ids;
        while (cursor) {
                data = cursor->data;
                if (data->i >= data->body.len ||
                    data->body.addr[data->i].type != t.type) {
                        // the id didn't match the next input symbol
                        // it dies a dishonorable death
                        cursor = cursor->next;
                        continue;
                }
                new = make(a, scfe_pda_id, 1);
                new->i = data->i + 1;
                new->rule = data->rule;
                new->head = data->head;
                new->body = data->body;
                if (data->i < new->body.len) {
#ifdef SCFE_DEBUG
                        printf("!!!SNEAKY!!!, assinging '%.*s'\n", t.sdata.len,
                               t.sdata.addr);
#endif
                        // we save the data we actually found
                        // when this token was matched in shift
                        if (new->body.addr[data->i].sdata.addr) {
#ifdef SCFE_DEBUG
                                printf("!!!!!!!SUPER RARE! '%.*s'\n",
                                       new->body.addr[data->i].sdata.len,
                                       new->body.addr[data->i].sdata.addr);
#endif
                        }
                        new->body.addr[data->i].sdata = t.sdata;
                        new->body.addr[data->i].line_num = t.line_num;
                        new->body.addr[data->i].col_pos = t.col_pos;
                }
                new->look_ahead = data->look_ahead;
                append_if_absent(a, new_state, new);
                cursor = cursor->next;
        }

        int ids = 0;
        *new_state = close_pda_id_list(a, g, new_state);
        cursor = new_state->ids;
        while (cursor) {
                ids++;
                cursor = cursor->next;
        }
        if (ids < 1) {
                printf("unexpected token: %i '%.*s' on line %i column %i\n",
                       t.type, t.sdata.len, t.sdata.addr, t.line_num,
                       t.col_pos);
                return -1;
        }
        p->state_stack[p->state_i++] = *new_state;

        return 0;
}

static scfe_pda_id *reduce(arena *a, scfe_parser *p, scfe_grammar *g,
                           scfe_tkn next_t)
{
        scfe_pda_id *data;
        scfe_tkn_arr *body;
        list *cursor = p->state_stack[p->state_i - 1].ids;

        while (cursor) {
                data = cursor->data;
                body = &data->body;
                if (data->i < body->len ||
                    !tkn_arr_member(data->look_ahead, next_t)) {
                        cursor = cursor->next;
                        continue;
                }
#ifdef SCFE_DEBUG
                printf("reduce %i\n", data->head.type);
#endif

                p->state_i -= data->body.len;

                shift(a, p, g, data->head);

                return data;
        }
        return 0;
}

static void print_state(list *state)
{
        scfe_pda_id *data;
        list *cursor = state;
        printf("-----\n");
        while (cursor) {
                data = cursor->data;
                printf("%i -> [%i] ", data->head.type, data->i);
                for (int i = 0; i < data->body.len; i++) {
                        printf("%i,", data->body.addr[i].type);
                }
                printf(" [");
                for (int i = 0; i < data->look_ahead.len; i++) {
                        printf("%i,", data->look_ahead.addr[i].type);
                }
                printf("]\n");
                cursor = cursor->next;
        }
}

ptnode *scfe_parse(arena *a, arena tmp, scfe_grammar *g,
                        scfe_tkn_arr tokens)
{
        scfe_parser prsr;
        prsr.pt_i = 0;
        prsr.state_i = 0;
        prsr.state_stack[prsr.state_i++] = create_start_state(&tmp, g);
#ifdef SCFE_DEBUG
        printf("=====START DEBUG TOKEN LIST\n");
        for (int i = 0; i < tokens.len; i++) {
                printf("%i '%.*s'\n", tokens.addr[i].type,
                       tokens.addr[i].sdata.len, tokens.addr[i].sdata.addr);
        }
        printf("=====END DEBUG TOKEN LIST\n");
#endif

        scfe_pda_id *p;
        ptnode *n;
        int i = 0;
        scfe_pda_state *state;

        while ((state = &prsr.state_stack[prsr.state_i - 1])) {
#ifdef SCFE_DEBUG
                print_state(state->ids);
#endif
                if (i >= tokens.len) {
                        // parser accepted, return top of value stack
                        return prsr.pt_stack[prsr.pt_i - 1];
                }

                while ((p = reduce(&tmp, &prsr, g, tokens.addr[i]))) {
                        state = &prsr.state_stack[prsr.state_i - 1];
#ifdef SCFE_DEBUG
                        print_state(state->ids);
#endif
                        //      make node for the rule we're about to reduce
                        //      with
                        n = make(a, ptnode, 1);
                        n->parent = 0;
                        n->rule = p->rule;
                        n->label = p->head;
#ifdef SCFE_DEBUG
                        printf("making head: %i\n", n->label.type);
#endif
                        n->child_cnt = p->body.len;
                        n->children = make(a, ptnode, n->child_cnt);

                        int to_pop = 0;
                        int nt_count = 0;
                        for (int i = 0; i < n->child_cnt; i++) {
                                nt_count +=
                                    is_terminal(g, p->body.addr[i]) ? 0 : 1;
                        }
                        for (int i = 0; i < n->child_cnt; i++) {
                                if (is_terminal(g, p->body.addr[i])) {
                                        n->children[i].rule = -1;
                                        n->children[i].parent = n;
                                        n->children[i].label = p->body.addr[i];
                                        n->children[i].child_cnt = 0;
                                        n->children[i].children = 0;
#ifdef SCFE_DEBUG
                                        printf(
                                            "created leaf node: %i (%.*s) for "
                                            "child %i\n",
                                            p->body.addr[i].type,
                                            p->body.addr[i].sdata.len,
                                            p->body.addr[i].sdata.addr, i);
#endif
                                } else {
                                        n->children[i] =
                                            *prsr.pt_stack[prsr.pt_i -
                                                           (nt_count) +
                                                           (to_pop++)];
#ifdef SCFE_DEBUG
                                        printf("popped nonterminal node: %i "
                                               "for child %i\n",
                                               p->body.addr[i].type, i);
#endif
                                }
                        }
                        prsr.pt_i -= to_pop;
                        prsr.pt_stack[prsr.pt_i++] = n;
#ifdef SCFE_DEBUG
                        printf("popped %i, %i high\n", to_pop, prsr.pt_i);
#endif
                }

                if (shift(&tmp, &prsr, g, tokens.addr[i++]) != 0) {
                        printf("whoops!\n");
                        return NULL;
                }
        }
        return 0;
}

#define IS_WHITESPACE(c) (c == ' ' || c == '\t' || c == '\n' || c == 0)
static scfe_tkn emit_tkn(arena *a, int *i, string input, scfe_nfa_arr nfas)
{
        if (*i >= input.len) {
                return (scfe_tkn){.type = -1};
        }
        // send char to each accepting machine until they're all dead
        // emit the last one to survive
        int accepted = 1;
        int last_accepted = 0;
        int line_num = 0;
        int col_pos = 0;
        string tkn_data;
        tkn_data.len = 1;
        tkn_data.addr = input.addr + (*i);
        while (accepted) {
                accepted = 0;
                for (int n = 0; n < nfas.len; n++) {
                        if (scfe_nfa_membership(a, nfas.addr[n], tkn_data)) {
                                accepted = n + 1;
                                tkn_data.len++;
                                (*i)++;
                                break;
                        }
                }
                if (accepted) {
                        last_accepted = accepted;
                }
        }
        if (last_accepted > 0) {
                tkn_data.len--;
                return (scfe_tkn){
                    .type = last_accepted - 1,
                    .line_num = line_num,
                    .col_pos = col_pos,
                    .sdata = tkn_data,
                };
        }
        printf("unrecognizable token: '%.*s'\n", tkn_data.len, tkn_data.addr);
        return (scfe_tkn){.type = -1};
}

static scfe_tkn_arr scfe_lex(arena *a, arena tmp, string_arr regexs,
                             string input)
{
        scfe_nfa_arr nfas;
        scfe_nfa *nfa_arr = make(a, scfe_nfa, regexs.len);
        nfas.len = regexs.len;
        nfas.addr = nfa_arr;
        for (int i = 0; i < regexs.len; i++) {
                nfas.addr[i] = scfe_regex(a, tmp, regexs.addr[i]);
        }

        int i = 0;
        list *node;
        list *tkns = NULL;
        int tkn_cnt = 0;

        int col_pos = 1;
        int line_num = 1;
        while (i < input.len) {
                if (IS_WHITESPACE(input.addr[i])) {
                        while (IS_WHITESPACE(input.addr[i])) {
                                if (input.addr[i] == '\n') {
                                        col_pos = 1;
                                        line_num++;
                                }
                                col_pos++;
                                i++;
                        }
                        continue;
                }
		if (input.addr[i] == '#') {
			while (input.addr[i] != '\n') {
				i++;
			}
			col_pos = 1;
			line_num++;
			continue;
		}
                node = make(a, list, 1);
                node->data = make(a, scfe_tkn, 1);
                int icopy = i;
                *((scfe_tkn *)node->data) = emit_tkn(a, &i, input, nfas);
                int idiff = i - icopy;
                ((scfe_tkn *)node->data)->line_num = line_num;
                ((scfe_tkn *)node->data)->col_pos = col_pos + idiff;
                if (((scfe_tkn *)node->data)->type < 0) {
                        return (scfe_tkn_arr){.len = -1, .addr = 0};
                }
                list_append(&tkns, node);
                tkn_cnt++;
        }

        scfe_tkn_arr res;
        res.len = 0;
        res.addr = make(a, scfe_tkn, tkn_cnt + 1);
        node = tkns;
        while (node) {
                res.addr[res.len++] = *((scfe_tkn *)node->data);
                node = node->next;
        }
        res.addr[res.len].type = nfas.len;
        res.addr[res.len].sdata = S("OVER");
        res.len += 1;

        return res;
}

ptnode *scfe_parse_tree(arena *a, arena tmp, scfe_grammar *g, string input)
{
        scfe_tkn_arr tokens = scfe_lex(a, tmp, g->terminals, input);
        if (tokens.len < 0) {
                return NULL;
        }
        ptnode *node = scfe_parse(a, tmp, g, tokens);
        return node;
}

/*
 * nfa stuff
 */

typedef list *nfa_id;
static void nfa_id_append(nfa_id *head, nfa_id node)
{
        list_append(head, node);
}

static void nfa_id_append_if_absent(nfa_id *head, nfa_id node)
{
        nfa_id cursor = *head;
        while (cursor) {
                if (cursor->data == node->data) {
                        return;
                }
                cursor = cursor->next;
        }
        list_append(head, node);
}

static void print_nfa_id(nfa_id id)
{

        nfa_id cursor;
        cursor = id;
        if (!cursor) {
                printf("null boy\n");
                return;
        }
        printf("-> %li", (long)cursor->data);
        cursor = cursor->next;
        while (cursor) {
                printf(", %li", (long)cursor->data);
                cursor = cursor->next;
        }
        printf("\n");
}

#define NFA_EPSILON (0x02)
static nfa_id nfa_eclose(arena *a, scfe_nfa_delta d, nfa_id id)
{
        int state;
        list *node;
        nfa_id res = 0;
        nfa_id cursor = id;

        // add every state we're already in
        while (cursor) {
                state = (long)cursor->data;
                node = make(a, list, 1);
                node->data = (void *)(long)state;
                nfa_id_append_if_absent(&res, node);
                cursor = cursor->next;
        }

        // now close this list
        cursor = res;
        while (cursor) {
                state = (long)cursor->data;
                for (int i = 0; i < d.rule_cnt; i++) {
                        if (d.rules[i].state == state &&
                            d.rules[i].input == NFA_EPSILON &&
                            d.rules[i].dest != state) {
                                // printf("%i + %i -> %i\n", d.rules[i].state,
                                // d.rules[i].input, d.rules[i].dest);
                                node = make(a, list, 1);
                                // printf("%i - %lukib (%lu) - %p\n", j++,
                                // bytes_used((*a)) / kibibyte,
                                // bytes_used((*a)), node);
                                node->data = (void *)(long)d.rules[i].dest;
                                nfa_id_append_if_absent(&res, node);
                        }
                }
                cursor = cursor->next;
        }
        // printf("done\n");
        //  print_nfa_id(res);

        return res;
}

static nfa_id nfa_step(arena *a, scfe_nfa_delta d, nfa_id id, int input)
{
        int state;
        list *node;
        list *new = 0;
        nfa_id cursor = nfa_eclose(a, d, id);
        // printf("nfa_step(%c)\n", input);
        while (cursor) {
                state = (long)cursor->data;
                // printf("we were in %i\n", state);
                for (int i = 0; i < d.rule_cnt; i++) {
                        if (d.rules[i].state == state &&
                            d.rules[i].input == input) {
                                node = make(a, list, 1);
                                node->data = (void *)(long)d.rules[i].dest;
                                nfa_id_append(&new, node);
                                // printf("\tand we can move to %i\n",
                                // (int)node->data);
                        }
                }
                cursor = cursor->next;
        }
        return new;
}

int scfe_nfa_membership(arena *a, scfe_nfa n, string input)
{
        nfa_id id;
        nfa_id id_list = 0;

        id = make(a, list, 1);
        id->data = (void *)(long)n.start;
        nfa_id_append(&id_list, id);

        // print_nfa_id(id_list);
        for (int i = 0; i < input.len; i++) {
                id_list = nfa_step(a, n.delta, id_list, input.addr[i]);
                // print_nfa_id(id_list);
        }
        id = nfa_eclose(a, n.delta, id_list);
        while (id) {
                if (id->data == (void *)(long)n.accept) {
                        return 1;
                }
                id = id->next;
        }
        // printf("nope %i\n", n.accept);
        return 0;
}

scfe_nfa scfe_nfa_union(arena *a, scfe_nfa s, scfe_nfa r)
{
        scfe_nfa n;
        int scnt = s.delta.rule_cnt;
        int rcnt = r.delta.rule_cnt;

        n.start = 0;
        n.accept = (s.accept + 1) + (r.accept + 1) + 1;
        n.delta.rule_cnt = scnt + rcnt + 4;
        n.delta.rules = make(a, scfe_nfa_arrow, n.delta.rule_cnt);

        for (int i = 0; i < s.delta.rule_cnt; i++) {
                n.delta.rules[i] = s.delta.rules[i];
                n.delta.rules[i].state += 1;
                n.delta.rules[i].dest += 1;
        }

        for (int i = 0; i < r.delta.rule_cnt; i++) {
                n.delta.rules[i + scnt] = r.delta.rules[i];
                n.delta.rules[i + scnt].state += s.accept + 2;
                n.delta.rules[i + scnt].dest += s.accept + 2;
        }

        n.delta.rules[0 + scnt + rcnt].state = n.start;
        n.delta.rules[0 + scnt + rcnt].input = NFA_EPSILON;
        n.delta.rules[0 + scnt + rcnt].dest = s.start + 1;

        n.delta.rules[1 + scnt + rcnt].state = n.start;
        n.delta.rules[1 + scnt + rcnt].input = NFA_EPSILON;
        n.delta.rules[1 + scnt + rcnt].dest = r.start + s.accept + 2;

        n.delta.rules[2 + scnt + rcnt].state = s.accept + 1;
        n.delta.rules[2 + scnt + rcnt].input = NFA_EPSILON;
        n.delta.rules[2 + scnt + rcnt].dest = n.accept;

        n.delta.rules[3 + scnt + rcnt].state = r.accept + s.accept + 2;
        n.delta.rules[3 + scnt + rcnt].input = NFA_EPSILON;
        n.delta.rules[3 + scnt + rcnt].dest = n.accept;

        return n;
}

scfe_nfa scfe_nfa_concat(arena *a, scfe_nfa s, scfe_nfa r)
{
        scfe_nfa n;
        int scnt = s.delta.rule_cnt;
        int rcnt = r.delta.rule_cnt;

        n.start = s.start;
        n.accept = r.accept + s.accept + 1;
        n.delta.rule_cnt = scnt + rcnt + 1;
        n.delta.rules = make(a, scfe_nfa_arrow, n.delta.rule_cnt);

        for (int i = 0; i < scnt; i++) {
                n.delta.rules[i] = s.delta.rules[i];
        }

        for (int i = 0; i < rcnt; i++) {
                n.delta.rules[i + scnt] = r.delta.rules[i];
                n.delta.rules[i + scnt].state += s.accept + 1;
                n.delta.rules[i + scnt].dest += s.accept + 1;
        }

        n.delta.rules[scnt + rcnt].state = s.accept;
        n.delta.rules[scnt + rcnt].input = NFA_EPSILON;
        n.delta.rules[scnt + rcnt].dest = r.start + s.accept + 1;

        return n;
}

scfe_nfa scfe_nfa_close(arena *a, scfe_nfa r)
{
        scfe_nfa n;
        int rcnt = r.delta.rule_cnt;

        n.start = 0;
        n.accept = (r.accept + 1) + 1;
        n.delta.rule_cnt = rcnt + 4;
        n.delta.rules = make(a, scfe_nfa_arrow, n.delta.rule_cnt);

        for (int i = 0; i < rcnt; i++) {
                n.delta.rules[i] = r.delta.rules[i];
                n.delta.rules[i].state += 1;
                n.delta.rules[i].dest += 1;
        }

        n.delta.rules[0 + rcnt].state = n.start;
        n.delta.rules[0 + rcnt].input = NFA_EPSILON;
        n.delta.rules[0 + rcnt].dest = r.start + 1;

        n.delta.rules[1 + rcnt].state = n.start;
        n.delta.rules[1 + rcnt].input = NFA_EPSILON;
        n.delta.rules[1 + rcnt].dest = n.accept;

        n.delta.rules[2 + rcnt].state = r.accept + 1;
        n.delta.rules[2 + rcnt].input = NFA_EPSILON;
        n.delta.rules[2 + rcnt].dest = r.start + 1;

        n.delta.rules[3 + rcnt].state = r.accept + 1;
        n.delta.rules[3 + rcnt].input = NFA_EPSILON;
        n.delta.rules[3 + rcnt].dest = n.accept;

        return n;
}

/*
 * regular expression stuff
 */
typedef enum re_tkn_type {
        TTOK_UNION,   // 0
        TTOK_CONCAT,  // 1
        TTOK_CLOSE,   // 2
        TTOK_EPSILON, // 3
        TTOK_GROUPL,  // 4
        TTOK_GROUPR,  // 5
        TTOK_RANGEL,  // 6
        TTOK_RANGER,  // 7
        TTOK_HYPHEN,  // 8
        TTOK_ESCAPE,  // 9
        TTOK_LITERAL, // 10
        TTOK_OVER,    // 11

        NTOK_START,        // 12
        NTOK_RANGE_TRIPLE, // 13
        NTOK_RANGE_LIST,   // 14
        NTOK_RANGE,        // 15
        NTOK_RE_P,         // 16
        NTOK_RE_S,         // 17
        NTOK_RE_T,         // 18
        NTOK_RE_Q          // 19
} re_tkn_type;

static re_tkn_type get_tkn_type(char c)
{
        switch (c) {
        case '+':
                return TTOK_UNION;
        case '.':
                return TTOK_CONCAT;
        case '*':
                return TTOK_CLOSE;
        case '~':
                return TTOK_EPSILON;
        case '(':
                return TTOK_GROUPL;
        case ')':
                return TTOK_GROUPR;
        case '[':
                return TTOK_RANGEL;
        case '-':
                return TTOK_HYPHEN;
        case ']':
                return TTOK_RANGER;
        case '\\':
                return TTOK_ESCAPE;
        default:
                return TTOK_LITERAL;
        }
}

static scfe_tkn_arr regex_lex(arena *a, arena tmp, string re_orig)
{
        // copy input
        string re;
        re.len = re_orig.len;
        re.addr = make(a, char, re.len);
        mfmemcpy(re.addr, re_orig.addr, re.len);

        int i = 0;
        int tok_cnt = 0;
        int implicit_concat = 0;
        int in_range = 0;
        scfe_tkn *tok;
        scfe_tkn_arr res;
        re_tkn_type tok_type;
        list *tokens = NULL;
        list *node;

        while (i < re.len) {
                tok_type = get_tkn_type(re.addr[i]);

                if (tok_type == TTOK_RANGEL) {
                        in_range = 1;
                } else if (tok_type == TTOK_RANGER) {
                        in_range = 0;
                } else if (tok_type == TTOK_LITERAL && implicit_concat &&
                           !in_range) {
                        node = make(&tmp, list, 1);

                        tok = make(&tmp, scfe_tkn, 1);
                        tok->type = TTOK_CONCAT;
                        tok->sdata = S(".");
                        tok_cnt++;

                        node->data = tok;
                        list_append(&tokens, node);
                }
                implicit_concat = tok_type == TTOK_LITERAL;

                node = make(&tmp, list, 1);

                tok = make(&tmp, scfe_tkn, 1);
                tok->type = tok_type;
                tok->sdata = (string){.len = 1, .addr = re.addr + (i++)};

                if (tok->type == TTOK_ESCAPE) {
                        tok->type = TTOK_LITERAL;
                        tok->sdata.addr = re.addr + (i++);
                }

                tok_cnt++;
                node->data = tok;
                list_append(&tokens, node);
        }

        res.len = 0;
        res.addr = make(a, scfe_tkn, tok_cnt + 1);
        node = tokens;
        while (node) {
                res.addr[res.len++] = *(scfe_tkn *)node->data;
                node = node->next;
        }
        res.addr[res.len].type = TTOK_OVER;
        res.addr[res.len].sdata = S("OVER");
        res.len += 1;

        return res;
}

static ptnode *regex_parse(arena *a, arena tmp, scfe_tkn_arr tokens)
{
        scfe_grammar grammar =
            {
                .terminals = {.len = 11},
                .nonterminals =
                    {
                        .len = 15,
                        .addr =
                            (scfe_pda_rule[]){
                                {
                                    .head = {.type = NTOK_RANGE_TRIPLE},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 3,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = TTOK_LITERAL},
                                                    {.type = TTOK_HYPHEN},
                                                    {.type = TTOK_LITERAL},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RANGE_LIST},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 1,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RANGE_TRIPLE},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RANGE_LIST},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 2,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RANGE_LIST},
                                                    {.type = NTOK_RANGE_TRIPLE},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RANGE},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 3,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = TTOK_RANGEL},
                                                    {.type = NTOK_RANGE_LIST},
                                                    {.type = TTOK_RANGER},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_P},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 1,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = TTOK_LITERAL},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_P},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 1,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = TTOK_EPSILON},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_P},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 1,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RANGE},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_P},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 3,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = TTOK_GROUPL},
                                                    {.type = NTOK_RE_Q},
                                                    {.type = TTOK_GROUPR},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_S},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 1,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RE_P},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_S},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 2,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RE_S},
                                                    {.type = TTOK_CLOSE},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_T},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 1,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RE_S},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_T},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 3,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RE_T},
                                                    {.type = TTOK_CONCAT},
                                                    {.type = NTOK_RE_S},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_Q},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 1,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RE_T},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_RE_Q},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 3,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RE_Q},
                                                    {.type = TTOK_UNION},
                                                    {.type = NTOK_RE_T},
                                                },
                                        },
                                },
                                {
                                    .head = {.type = NTOK_START},
                                    .body =
                                        (scfe_tkn_arr){
                                            .len = 2,
                                            .addr =
                                                (scfe_tkn[]){
                                                    {.type = NTOK_RE_Q},
                                                    {.type = TTOK_OVER},
                                                },
                                        },
                                },
                            },
                    },
            };
        return scfe_parse(a, tmp, &grammar, tokens);
}

static scfe_nfa create_range_nfa(arena *a, arena tmp, scfe_tkn l, scfe_tkn r)
{
        char start = l.sdata.addr[0];
        char stop = r.sdata.addr[0];
        scfe_nfa res = (scfe_nfa){
            .start = 0,
            .accept = 1,
            .delta =
                (scfe_nfa_delta){
                    .rule_cnt = 1,
                    .rules =
                        (scfe_nfa_arrow[]){
                            (scfe_nfa_arrow){
                                .state = 0,
                                .dest = 1,
                                .input = start,
                            },
                        },
                },
        };
        for (int i = start + 1; i <= stop; i++) {
                scfe_nfa next = (scfe_nfa){
                    .start = 0,
                    .accept = 1,
                    .delta =
                        (scfe_nfa_delta){
                            .rule_cnt = 1,
                            .rules =
                                (scfe_nfa_arrow[]){
                                    (scfe_nfa_arrow){
                                        .state = 0,
                                        .dest = 1,
                                        .input = i,
                                    },
                                },
                        },
                };
                res = scfe_nfa_union(a, res, next);
        }
        return res;
}

static scfe_nfa create_nfa(arena *a, arena tmp, ptnode *pt)
{
        // ez algo
        ptnode *child;
        scfe_nfa_arrow *arrows;
        if (pt->child_cnt == 1) {
                child = pt->children;
                switch (child->label.type) {
                case TTOK_LITERAL:
                        arrows = make(a, scfe_nfa_arrow, 1);
                        arrows[0] = (scfe_nfa_arrow){
                            .state = 0,
                            .dest = 1,
                            .input = child->label.sdata.addr[0],
                        };
                        return (scfe_nfa){
                            .start = 0,
                            .accept = 1,
                            .delta =
                                (scfe_nfa_delta){
                                    .rule_cnt = 1,
                                    .rules = arrows,
                                },
                        };
                case TTOK_EPSILON:
                        arrows = make(a, scfe_nfa_arrow, 1);
                        arrows[0] = (scfe_nfa_arrow){
                            .state = 0,
                            .dest = 1,
                            .input = NFA_EPSILON,
                        };
                        return (scfe_nfa){
                            .start = 0,
                            .accept = 1,
                            .delta = (scfe_nfa_delta){.rule_cnt = 1,
                                                      .rules = arrows},
                        };
                default:
                        return create_nfa(a, tmp, pt->children);
                }
        } else if (pt->child_cnt == 2) {
                if (pt->label.type == NTOK_RANGE_LIST) {
                        scfe_nfa heads = create_nfa(a, tmp, &pt->children[0]);
                        scfe_nfa tail = create_nfa(a, tmp, &pt->children[1]);
                        return scfe_nfa_union(a, heads, tail);
                } else {
                        child = &pt->children[1];
                        if (child->label.type == TTOK_CLOSE) {
                                scfe_nfa nfa = create_nfa(a, tmp, pt->children);
                                return scfe_nfa_close(a, nfa);
                        }
                        scfe_nfa left = create_nfa(a, tmp, pt->children);
                        scfe_nfa right = create_nfa(a, tmp, child);
                        return scfe_nfa_concat(a, left, right);
                }
        }
        child = &pt->children[1];
        switch (child->label.type) {
        case TTOK_HYPHEN:
                return create_range_nfa(a, tmp, pt->children[0].label,
                                        pt->children[2].label);
        case TTOK_CONCAT:
                return scfe_nfa_concat(a, create_nfa(a, tmp, &pt->children[0]),
                                       create_nfa(a, tmp, &pt->children[2]));
        case TTOK_UNION:
                return scfe_nfa_union(a, create_nfa(a, tmp, &pt->children[0]),
                                      create_nfa(a, tmp, &pt->children[2]));
        default:
                return create_nfa(a, tmp, child);
        }
}

static scfe_nfa regex_compile(arena *a, arena tmp, ptnode *pt)
{
        return create_nfa(a, tmp, pt);
}

scfe_nfa scfe_regex(arena *a, arena tmp, string re)
{
        scfe_tkn_arr tokens = regex_lex(a, tmp, re);
        ptnode *pt = regex_parse(a, tmp, tokens);
        scfe_nfa nfa = regex_compile(a, tmp, pt);
        return nfa;
}

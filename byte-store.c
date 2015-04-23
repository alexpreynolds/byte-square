#include "byte-store.h"

int
main(int argc, char** argv) 
{
    lookup_t *lookup = NULL;
    store_t* store = NULL;
    uint32_t pairs = 0;

    bs_init_globals();
    bs_init_command_line_options(argc, argv);

    lookup = bs_init_lookup(bs_global_args.lookup_fn);

    if (bs_global_args.store_create_flag) {
        store = bs_init_store(pairs);
        bs_delete_store(&store);
    }
    else if (bs_global_args.store_query_flag) {
    }

    bs_delete_lookup(&lookup);

    return EXIT_SUCCESS;
}

lookup_t*
bs_init_lookup(char* fn)
{
    lookup_t* l = NULL;
    FILE* lf = NULL;
    char buf[BUF_MAX_LEN];
    char chr_str[BUF_MAX_LEN];
    char start_str[BUF_MAX_LEN];
    char stop_str[BUF_MAX_LEN];
    char id_str[BUF_MAX_LEN];
    uint64_t start_val = 0;
    uint64_t stop_val = 0;

    l = malloc(sizeof(lookup_t));
    if (!l) {
        fprintf(stderr, "Error: Could not allocate space for lookup table!\n");
        exit(EXIT_FAILURE);
    }
    l->capacity = 0;
    l->nelems = 0;
    l->elems = NULL;

    lf = fopen(fn, "r");
    if (lf) {
        while (fgets(buf, BUF_MAX_LEN, lf)) {
            sscanf(buf, "%s\t%s\t%s\t%s\n", chr_str, start_str, stop_str, id_str);
            sscanf(start_str, "%" SCNu64, &start_val);
            sscanf(stop_str, "%" SCNu64, &stop_val);
            element_t* e = bs_init_element(chr_str, start_val, stop_val, id_str);
            bs_push_elem_to_lookup(e, &l);
        }
    }
    else {
        fprintf(stderr, "Error: Could not open or read from [%s]\n", fn);
        bs_print_usage(stderr);
        exit(EXIT_FAILURE);
    }
    fclose(lf);

    return l;
}

void 
bs_delete_lookup(lookup_t** l)
{
    for (uint32_t idx = 0; idx < (*l)->nelems; idx++) {
        bs_delete_element(&(*l)->elems[idx]);
    }
    free((*l)->elems);
    (*l)->elems = NULL;
    (*l)->nelems = 0;
    (*l)->capacity = 0;
    free(*l);
    *l = NULL;
}

element_t*
bs_init_element(char* chr, uint64_t start, uint64_t stop, char* id)
{
    element_t *e = NULL;

    e = malloc(sizeof(element_t));
    if (!e) {
        fprintf(stderr, "Error: Could not allocate space for element!\n");
        exit(EXIT_FAILURE);
    }
    e->chr = NULL;
    if (strlen(chr) > 0) {
        e->chr = malloc(sizeof(*chr) * strlen(chr) + 1);
        if (!e->chr) {
            fprintf(stderr, "Error: Could not allocate space for element chromosome!\n");
            exit(EXIT_FAILURE);
        }
        memcpy(e->chr, chr, strlen(chr) + 1);
    }
    e->start = start;
    e->stop = stop;
    e->id = NULL;
    if (strlen(id) > 0) {
        e->id = malloc(sizeof(*id) * strlen(id) + 1);
        if (!e->id) {
            fprintf(stderr,"Error: Could not allocate space for element id!\n");
            exit(EXIT_FAILURE);
        }
        memcpy(e->id, id, strlen(id) + 1);
    }
    
    return e;
}

void
bs_delete_element(element_t** e)
{
    free((*e)->chr);
    (*e)->chr = NULL;
    (*e)->start = 0;
    (*e)->stop = 0;
    free((*e)->id);
    (*e)->id = NULL;
    free(*e);
    *e = NULL;
}

void
bs_push_elem_to_lookup(element_t* e, lookup_t** l)
{
    if ((*l)->capacity == 0) {
        (*l)->capacity++;
        (*l)->elems = malloc(sizeof(element_t *));
    }
    else if ((*l)->nelems >= (*l)->capacity) {
        (*l)->capacity *= 2;
        element_t** new_elems = malloc(sizeof(element_t *) * (*l)->capacity);
        for (uint32_t idx = 0; idx < (*l)->nelems; idx++) {
            new_elems[idx] = bs_init_element((*l)->elems[idx]->chr,
                                             (*l)->elems[idx]->start,
                                             (*l)->elems[idx]->stop,
                                             (*l)->elems[idx]->id);
            bs_delete_element(&((*l)->elems[idx]));
        }   
        (*l)->elems = new_elems;     
    }
    uint32_t n = (*l)->nelems;
    (*l)->elems[n] = e;
    (*l)->nelems++;
}

store_t* 
bs_init_store(uint32_t pairs)
{
    store_t* s = NULL;

    s = malloc(sizeof(store_t));
    s->pairs = pairs;
    s->nbytes = pairs * (pairs + 1) / 2; /* upper diagonal matrix */
    s->data = malloc(sizeof(unsigned char) * s->nbytes);

    if (!s || !s->data) {
        fprintf(stderr, "Error: Could not allocate space for store\n");
        exit(EXIT_FAILURE);
    }

    return s;
}

void
bs_populate_store()
{
}

void
bs_delete_store(store_t** s)
{
    free((*s)->data);
    (*s)->data = NULL;
    (*s)->nbytes = 0;
    (*s)->pairs = 0;
    free(*s);
    *s = NULL;
}

void
bs_init_globals()
{
    bs_global_args.store_create_flag = kFalse;
    bs_global_args.store_query_flag = kFalse;
    bs_global_args.rng_seed_flag = kFalse;
    bs_global_args.rng_seed_value = 0;
    bs_global_args.lookup_fn[0] = '\0';
}

void 
bs_init_command_line_options(int argc, char** argv)
{
    /*
    fprintf(stderr, "argc:\t\t[%d]\n", argc);
    for (int argc_idx = 0; argc_idx < argc; argc_idx++)
        fprintf(stderr, "argv[%02d]:\t[%s]\n", argc_idx, argv[argc_idx]); 
    */

    int bs_client_long_index;
    int bs_client_opt = getopt_long(argc,
                                    argv,
                                    bs_client_opt_string,
                                    bs_client_long_options,
                                    &bs_client_long_index);

    opterr = 0;

    while (bs_client_opt != -1) {
        switch (bs_client_opt) {
        case 'c':
            bs_global_args.store_create_flag = kTrue;
            break;
        case 'q':
            bs_global_args.store_query_flag = kTrue;
            break;
        case 'l':
            memcpy(bs_global_args.lookup_fn, optarg, strlen(optarg) + 1);
            break;
        case 'd':
            bs_global_args.rng_seed_flag = kTrue;
            bs_global_args.rng_seed_value = atoi(optarg);
            break;
        case 'h':
        case '?':
            bs_print_usage(stdout);
            exit(EXIT_SUCCESS);
        default:
            break;
        }
        bs_client_opt = getopt_long(argc,
                                    argv,
                                    bs_client_opt_string,
                                    bs_client_long_options,
                                    &bs_client_long_index);
    }

    if (bs_global_args.store_create_flag && bs_global_args.store_query_flag) {
        fprintf(stderr, "Error: Cannot both create and query data store!\n");
        bs_print_usage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!bs_global_args.store_create_flag && !bs_global_args.store_query_flag) {
        fprintf(stderr, "Error: Must either create or query a data store!\n");
        bs_print_usage(stderr);
        exit(EXIT_FAILURE);
    }
}

void 
bs_print_usage(FILE* output_stream) 
{
    fprintf(output_stream,
            "\n" \
            " Usage: \n\n" \
            "\t Create data store:\n" \
            "\t\t %s --store-create --lookup=fn --store=fn\n\n" \
            "\t Query data store:\n" \
            "\t\t %s --store-query  --lookup=fn --store=fn --query=str\n\n" \
            " Notes:\n\n" \
            " - Lookup file is a sorted BED3 or BED4 file\n\n" \
            " - Query string is a numeric range specifing indices of interest from lookup\n" \
            "   table (e.g. \"17-83\" represents indices 17 through 83)\n\n",
            bs_name,
            bs_name);
}
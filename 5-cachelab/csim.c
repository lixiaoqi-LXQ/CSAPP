#include "cachelab.h"
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>

const char *helpinfo =
    "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
    "Options:\n"
    "  -h         Print this help message.\n"
    "  -v         Optional verbose flag.\n"
    "  -s <num>   Number of set index bits.\n"
    "  -E <num>   Number of lines per set.\n"
    "  -b <num>   Number of block offset bits.\n"
    "  -t <file>  Trace file.\n\n"
    "Examples:\n"
    "  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
    "  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";

#define MAX_LINE 100
#define __DEUBG__

typedef u_int8_t  Byte;
typedef u_int64_t Flag;
typedef u_int64_t Index;
typedef u_int64_t Offset;
typedef u_int64_t Address;

typedef struct
{
    bool valid;
    Flag flag;
    /* Byte *block; */
} Line;

typedef struct
{
    Line *lines;
    int  *recently_used;
    int   cur;
} Set;

typedef struct
{
    Flag   flag;
    Index  index;
    Offset offset;
} AddrParse;

typedef struct
{
    int hits;
    int misses;
    int evictions;
} Result;

u_int32_t E, s, b;
bool      verbose;

void      sanity_check();
AddrParse address_parse(Address addr);

Result emualte(FILE *f)
{
    // goal result
    Result r;
    r.hits = r.misses = r.evictions = 0;

    // input parse
    char    *p;
    char     line[MAX_LINE];
    Address  addr;
    unsigned size;

    // allocate space
    Set  *cache     = malloc(sizeof(Set) * pow(2, s));
    Line *line_mem  = malloc(sizeof(Line) * E * pow(2, s));
    int  *rused_mem = malloc(sizeof(int) * (E - 1) * pow(2, s));
    memset(rused_mem, -1, sizeof(int) * (E - 1) * pow(2, s));
    if (!cache || !line_mem || !rused_mem) {
        printf("no left space\n");
        exit(1);
    }
    for (int i = 0; i < pow(2, s); i++) {
        cache[i].lines         = line_mem + i * E;
        cache[i].recently_used = rused_mem + i * (E - 1);
        cache[i].cur           = 0;
        for (int j = 0; j < E; j++)
            cache[i].lines[j].valid = false;
    }

    while (1) {
        unsigned lru_idx;
        bool     hit    = false;
        bool     assign = false;
        if (!fgets(line, MAX_LINE, f))
            break;
        if (line[0] == 'I')
            continue;
        for (p = line + 2; *p != ','; p++)
            ;
        *p           = '\0';
        addr         = strtoull(line + 2, NULL, 16);
        size         = strtoul(p + 1, NULL, 10);

        AddrParse ap = address_parse(addr);
        /* if (ap.offset + size > pow(2, b)) {
         *     printf(" %c %x,%d\n", line[1], (unsigned)addr, size);
         *     assert(ap.offset + size <= pow(2, b));
         *     exit(1);
         * } */

        if (verbose) {
#ifdef __DEUBG__
            printf("%c %lx,%d [%x, %x, %x]\t", line[1], addr, size,
                   (unsigned)ap.flag, (unsigned)ap.index, (unsigned)ap.offset);
#else
            printf("%c %lx,%d ", line[1], (unsigned long)addr, size);
#endif
        }

        for (int i = 0; i < E; i++) {
            if (cache[ap.index].lines[i].valid == false)
                continue;
            if (cache[ap.index].lines[i].flag == ap.flag) {
                hit = true;
                r.hits++;
                if (verbose)
                    printf("hit ");
                lru_idx = i;
                break;
            }
        }
        if (!hit) {
            r.misses++;
            if (verbose)
                printf("miss ");
            // get the address into cache
            for (int i = 0; i < E; i++) {
                if (cache[ap.index].lines[i].valid == false) {
                    // set line
                    cache[ap.index].lines[i].valid = true;
                    cache[ap.index].lines[i].flag  = ap.flag;
                    assign                         = true;
                    lru_idx                        = i;
                    break;
                }
            }
            if (!assign) {
                // need eviction here
                r.evictions++;
                if (E != 1) {
                    // find a line to evict
                    for (int evict = 0;; evict++) {
                        int i = 0;
                        for (i = 0; i != E - 1; i++)
                            if (cache[ap.index].recently_used[i] == evict)
                                break;
                        if (i == E - 1) {
                            cache[ap.index].lines[evict].flag = ap.flag;
                            lru_idx                           = evict;
                            break;
                        }
                    }
                } else
                    cache[ap.index].lines[0].flag = ap.flag;
                if (verbose) {
                    printf("eviction ");
#ifdef __DEUBG__
                    for (int k = 0; k < E; ++k) {
                        printf("[%d]%lx ", k, cache[ap.index].lines[k].flag);
                    }
#endif
                }
            }
        }
        if (line[1] == 'M') {
            r.hits++;
            if (verbose)
                printf("hit");
        }

        // LRU update
        if (E != 1) {
            // we should avoid duplicate here: it's an error case
            bool owned = false;
            int  idx   = cache[ap.index].cur;
            for (int i = 0; i != E - 1; i++) {
                if (cache[ap.index].recently_used[i] == lru_idx) {
                    owned = true;
                    break;
                }
            }
            if (!owned) {
                cache[ap.index].recently_used[idx] = lru_idx;
                cache[ap.index].cur                = (idx + 1) % (E - 1);
            } else {
                int  old = (cache[ap.index].cur + (E - 1) - 1) % (E - 1);
                int *mem = malloc(sizeof(int) * (E - 1));
                assert(mem);
                memcpy(mem, cache[ap.index].recently_used,
                       sizeof(int) * (E - 1));
                for (int i = E - 3; i >= 0; i--) {
                    if (mem[old] == lru_idx) {
                        old = (old + (E - 1) - 1) % (E - 1);
                    }
                    cache[ap.index].recently_used[i] = mem[old];
                    old = (old + (E - 1) - 1) % (E - 1);
                }
                cache[ap.index].recently_used[E - 2] = lru_idx;
                cache[ap.index].cur                  = 0;

                free(mem);
            }
            /* int bef = (idx == 0 ? E - 2 : idx - 1);
             * // only update for critical case
             * if (cache[ap.index].recently_used[bef] != lru_idx) {
             *     cache[ap.index].recently_used[idx] = lru_idx;
             *     cache[ap.index].cur                = (idx + 1) % (E - 1);
             * } */
        }
        if (verbose) {
#ifdef __DEUBG__
            int idx = cache[ap.index].cur;
            printf("<");
            for (int i = 0; i != E - 1; i++) {
                printf("%d%s", cache[ap.index].recently_used[i],
                       (i == idx ? "*," : ","));
            }
            printf("\b>");
#endif
            printf("\n");
        }
    }

    free(line_mem);
    free(rused_mem);
    free(cache);
    return r;
}

int main(int argc, char *argv[])
{
    bool s1, s2, s3, s4;
    verbose = false;
    s1 = s2 = s3 = s4 = true;
    FILE *file;
    int   o;
    while ((o = getopt(argc, argv, "s:E:b:t:h::v::")) != -1) {
        switch (o) {
            case 's':
                s  = atoi(optarg);
                s1 = false;
                break;
            case 'E':
                E  = atoi(optarg);
                s2 = false;
                break;
            case 'b':
                b  = atoi(optarg);
                s3 = false;
                break;
            case 't':
                file = fopen(optarg, "r");
                s4   = false;
                if (!file) {
                    printf("%s: No such file or directory\n", optarg);
                    exit(1);
                }
                break;
            case 'v':
                verbose = true;
                break;
            case '?':
                printf("error input %c\n", optopt);
                printf("%s", helpinfo);
                exit(1);
            case 'h':
            default:
                printf("%s", helpinfo);
                exit(0);
        }
    }
    if (s1 || s2 || s3 || s4) {
        printf("./csim-ref: Missing required command line argument\n");
        printf("%s", helpinfo);
        exit(1);
    }
    /* printf("E = %u, s = %u, b = %u\n", E, s, b);
     * if (verbose) {
     *     printf("verbose mode\n");
     * } */
    sanity_check();
    Result r = emualte(file);

    printSummary(r.hits, r.misses, r.evictions);
    /* fclose(file); */
    return 0;
}

AddrParse address_parse(Address addr)
{
    AddrParse ap;
    ap.offset = addr & ((1 << b) - 1);
    ap.index  = (addr >> b) & ((1 << s) - 1);
    // no check for 1 on MSB
    ap.flag   = (addr >> (b + s));
    return ap;
}

void sanity_check()
{
    assert(b <= 5);
    assert(s <= 5);
}

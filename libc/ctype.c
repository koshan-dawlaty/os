/* this is used for acpica */
static unsigned short ctype_table[384];

#define _ISupper 0x01
#define _ISlower 0x02
#define _ISalpha (_ISupper | _ISlower)
#define _ISdigit 0x04
#define _ISspace 0x08

static unsigned short ctype_table[384];

static unsigned short *ctype_ptr = ctype_table + 128;

const short unsigned int **__ctype_b_loc(void)
{
    return (const short unsigned int **)(&ctype_ptr);
}

void init_ctype(void)
{
    for (int i = 0; i < 384; i++)
        ctype_table[i] = 0;

    for (int c = 'A'; c <= 'Z'; c++)
        ctype_ptr[c] |= _ISupper;

    for (int c = 'a'; c <= 'z'; c++)
        ctype_ptr[c] |= _ISlower;

    for (int c = '0'; c <= '9'; c++)
        ctype_ptr[c] |= _ISdigit;

    ctype_ptr[' '] |= _ISspace;
    ctype_ptr['\t'] |= _ISspace;
    ctype_ptr['\n'] |= _ISspace;
}

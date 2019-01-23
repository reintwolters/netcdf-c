/*
  Copyright 2018, UCAR/Unidata
  See COPYRIGHT file for copying and redistribution conditions.
*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "netcdf.h"
#include "ncfilter.h"

#define PARAMS_ID 32768

/* The C standard apparently defines all floating point constants as double;
   we rely on that in this code. Update: apparently not true when
   -ansi flag is used.
*/
#define DBLVAL 12345678.12345678

#define MAXPARAMS 32
#define NPARAMS 16 /* # of unsigned ints in params */

static unsigned int baseline[NPARAMS];
	
/* Expected contents of baseline:
id = 32768
params = 4294967279, 23, 4294967271, 27, 77, 93, 1145389056, 3287505826, 1097305129, 1, 2147483647, 4294967295U, 4294967295U
*/

static const char* spec = 
"32768, -17b, 23ub, -25S, 27US, 77, 93U, 789f, 12345678.12345678d, -9223372036854775807L, 18446744073709551615UL, 2147483647, -2147483648, 4294967295U";

/* Test support for the conversions */
/* Not sure if this kind of casting via union is legal C99 */
static union {
    unsigned int ui;
    float f;
} uf;

static union {
    unsigned int ui[2];
    double d;
} ud;

static union {
    unsigned int ui[2];
    unsigned long long ull;
    long long ll;
} ul;

static int nerrs = 0;

static int parsefilterspec(const char* spec, unsigned int* idp, size_t* nparamsp, unsigned int** paramsp);

static void
mismatch(size_t i, unsigned int *params, const char* tag)
{
    fprintf(stderr,"mismatch: %s [%d] baseline=%u params=%u\n",tag,(int)i,baseline[i],params[i]);
    fflush(stderr);
    nerrs++;
}

static void
mismatch2(size_t i, unsigned int *params, const char* tag)
{
    fprintf(stderr,"mismatch2: %s [%ld-%ld] baseline=%u,%u params=%u,%u\n",
	tag,(long)i,(long)(i+1),baseline[i],baseline[i+1],params[i],params[i+1]);
    fflush(stderr);
    nerrs++;
}

static void
insert(int index, void* src, size_t size)
{
    void* dst = &baseline[index];
    memcpy(dst,src,size);
}

static void
buildbaseline(void)
{
    unsigned int val4;
    unsigned long long val8;
    float float4;
    double float8;

    val4 = ((unsigned int)-17) & 0xff;
    insert(0,&val4,sizeof(val4)); /* 0 signed int*/
    val4 = (unsigned int)23;
    insert(1,&val4,sizeof(val4)); /* 1 unsigned int*/
    val4 = ((unsigned int)-25) & 0xffff;
    insert(2,&val4,sizeof(val4)); /* 3 signed int*/
    val4 = (unsigned int)27;
    insert(3,&val4,sizeof(val4)); /* 4 unsigned int*/
    val4 = (unsigned int)77;
    insert(4,&val4,sizeof(val4)); /* 4 signed int*/
    val4 = (unsigned int)93;
    insert(5,&val4,sizeof(val4)); /* 5 unsigned int*/
    float4 = 789.0f;
    insert(6,&float4,sizeof(float4)); /* 6 float */
    float8 = DBLVAL;
    insert(7,&float8,sizeof(float8)); /* 7 double */
    val8 = -9223372036854775807L;
    insert(9,&val8,sizeof(val8)); /* 9 signed long long */
    val8 = 18446744073709551615UL;
    insert(11,&val8,sizeof(val8)); /* 11 unsigned long long */
    val4 = 2147483647;
    insert(13,&val4,sizeof(val4)); /* 13 signed int */
    val4 = (-2147483647)-1;
    insert(14,&val4,sizeof(val4)); /* 14 signed int */
    val4 = 4294967295U;
    insert(15,&val4,sizeof(val4)); /* 15 unsigned int */
}

/**************************************************/
int
main(int argc, char **argv)
{
    int stat = 0;
    unsigned int id = 0;
    size_t i,nparams = 0;
    unsigned int* params = NULL;

    printf("\nTesting filter parser.\n");

    buildbaseline(); /* Build our comparison vector */

    stat = parsefilterspec(spec,&id,&nparams,&params);
    if(stat) {
	fprintf(stderr,"NC_parsefilterspec failed\n");
	exit(1);
    }
    if(id != PARAMS_ID)
        fprintf(stderr,"mismatch: id: expected=%u actual=%u\n",PARAMS_ID,id);
    for(i=0;i<nparams;i++) {
	if(baseline[i] != params[i])
	    mismatch(i,params,"N.A.");
    }
    /* Now some specialized tests */
    uf.ui = params[6];
    if(uf.f != (float)789.0)
	mismatch(6,params,"uf.f");
    ud.ui[0] = params[7];
    ud.ui[1] = params[8];
#ifdef WORD_BIGENDIAN
    byteswap8((unsigned char*)&ud.d);
#endif
    if(ud.d != (double)DBLVAL)
	mismatch2(7,params,"ud.d");
    ul.ui[0] = params[9];
    ul.ui[1] = params[10];
#ifdef WORD_BIGENDIAN
    byteswap8((unsigned char*)&ul.ll);
#endif
    if(ul.ll != -9223372036854775807LL)
	mismatch2(9,params,"ul.ll");
    ul.ui[0] = params[11];
    ul.ui[1] = params[12];
#ifdef WORD_BIGENDIAN
    byteswap8((unsigned char*)&ul.ull);
#endif
    if(ul.ull != 18446744073709551615ULL)
	mismatch2(11,params,"ul.ull");

    if (params)
       free(params);

    if (!nerrs)
       printf("SUCCESS!!\n");

    return (nerrs > 0 ? 1 : 0);
}

#ifdef WORD_BIGENDIAN
/* Byte swap an 8-byte integer in place */
static void
byteswap8(unsigned char* mem)
{
    unsigned char c;
    c = mem[0];
    mem[0] = mem[7];
    mem[7] = c;
    c = mem[1];
    mem[1] = mem[6];
    mem[6] = c;
    c = mem[2];
    mem[2] = mem[5];
    mem[5] = c;
    c = mem[3];
    mem[3] = mem[4];
    mem[4] = c;
}
#endif



/* Look at q0 and q1) to determine type */
static int
gettype(const int q0, const int q1, int* isunsignedp)
{
    int type = 0;
    int isunsigned = 0;
    char typechar;
    
    isunsigned = (q0 == 'u' || q0 == 'U');
    if(q1 == '\0')
	typechar = q0; /* we were given only a single char */
    else if(isunsigned)
	typechar = q1; /* we have something like Ux as the tag */
    else
	typechar = q1; /* look at last char for tag */
    switch (typechar) {
    case 'f': case 'F': case '.': type = 'f'; break; /* float */
    case 'd': case 'D': type = 'd'; break; /* double */
    case 'b': case 'B': type = 'b'; break; /* byte */
    case 's': case 'S': type = 's'; break; /* short */
    case 'l': case 'L': type = 'l'; break; /* long long */
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': type = 'i'; break;
    case 'u': case 'U': type = 'i'; isunsigned = 1; break; /* unsigned int */
    case '\0': type = 'i'; break;
    default: break;
    }
    if(isunsignedp) *isunsignedp = isunsigned;
    return type;
}

static int
parsefilterspec(const char* spec, unsigned int* idp, size_t* nparamsp, unsigned int** paramsp)
{
    int stat = NC_NOERR;
    int sstat; /* for scanf */
    char* p;
    char* sdata = NULL;
    unsigned int id;
    size_t count; /* no. of comma delimited params */
    size_t nparams; /* final no. of unsigned ints */
    size_t len;
    int i;
    unsigned int* ulist = NULL;
    unsigned char mem[8]; /* to convert to network byte order */

    if(spec == NULL || strlen(spec) == 0) goto fail;
    sdata = strdup(spec);

    /* Count number of parameters + id and delimit */
    p=sdata;
    for(count=0;;count++) {
        char* q = strchr(p,',');
	if(q == NULL) break;
	*q++ = '\0';
	p = q;
    }
    count++; /* for final piece */

    if(count == 0)
	goto fail; /* no id and no parameters */

    /* Extract the filter id */
    p = sdata;
    sstat = sscanf(p,"%u",&id);
    if(sstat != 1) goto fail;
    /* skip past the filter id */
    p = p + strlen(p) + 1;
    count--;

    /* Allocate the max needed space; *2 in case the params are all doubles */
    ulist = (unsigned int*)malloc(sizeof(unsigned int)*(count)*2);
    if(ulist == NULL) goto fail;

    /* walk and convert */
    nparams = 0; /* actual count */
    for(i=0;i<count;i++) { /* step thru param strings */
	unsigned long long val64u;
	unsigned int val32u;
	double vald;
	float valf;
	unsigned int *vector;
	int isunsigned = 0;
	int isnegative = 0;
	int type = 0;
	char* q;

	len = strlen(p);
	/* skip leading white space */
	while(strchr(" 	",*p) != NULL) {p++; len--;}
	/* Get leading sign character, if any */
	if(*p == '-') isnegative = 1;
        /* Get trailing type tag characters */
	switch (len) {
	case 0:
	    goto fail; /* empty parameter */
	case 1:
	case 2:
	    q = (p + len) - 1; /* point to last char */
	    type = gettype(*q,'\0',&isunsigned);
	    break;
	default: /* > 2 => we might have a two letter tag */
	    q = (p + len) - 2;
	    type = gettype(*q,*(q+1),&isunsigned);
	    break;
	}

	/* Now parse */
	switch (type) {
	case 'b':
	case 's':
	case 'i':
 	    /* special case for a positive integer;for back compatibility.*/
	    if(!isnegative)
	        sstat = sscanf(p,"%u",&val32u);
	    else
                sstat = sscanf(p,"%d",(int*)&val32u);
	    if(sstat != 1) goto fail;
	    switch(type) {
	    case 'b': val32u = (val32u & 0xFF); break;
	    case 's': val32u = (val32u & 0xFFFF); break;
	    }
	    ulist[nparams++] = val32u;
	    break;

	case 'f':
	    sstat = sscanf(p,"%lf",&vald);
	    if(sstat != 1) goto fail;
	    valf = (float)vald;
	    ulist[nparams++] = *(unsigned int*)&valf;
	    break;

	case 'd':
	    sstat = sscanf(p,"%lf",&vald);
	    if(sstat != 1) goto fail;
	    /* convert to network byte order */
	    memcpy(mem,&vald,sizeof(mem));
#ifdef WORDS_BIGENDIAN
	    byteswap8(mem);  /* convert big endian to little endian */
#endif
	    vector = (unsigned int*)mem;
	    ulist[nparams++] = vector[0];
	    ulist[nparams++] = vector[1];
	    break;
	case 'l': /* long long */
	    if(isunsigned)
	        sstat = sscanf(p,"%llu",&val64u);
	    else
                sstat = sscanf(p,"%lld",(long long*)&val64u);
	    if(sstat != 1) goto fail;
	    /* convert to network byte order */
	    memcpy(mem,&val64u,sizeof(mem));
#ifdef WORDS_BIGENDIAN	    
	    byteswap8(mem);  /* convert big endian to little endian */
#endif
	    vector = (unsigned int*)mem;
	    ulist[nparams++] = vector[0];
	    ulist[nparams++] = vector[1];
	    break;
	default:
	    goto fail;
	}
        p = p + strlen(p) + 1; /* move to next param */
    }
    /* Now return results */
    if(idp) *idp = id;
    if(nparamsp) *nparamsp = nparams;
    if(paramsp) {
       *paramsp = ulist;
       ulist = NULL; /* avoid duplicate free */
    }
done:
    if(sdata) free(sdata);
    if(ulist) free(ulist);
    return stat;
fail:
    stat = NC_EFILTER;
    goto done;
}

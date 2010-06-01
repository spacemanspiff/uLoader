#ifndef __UTILS_H__
#define __UTILS_H__

#define round_up(x,n)   (-(-(x) & -(n)))

void lower_caps ( char * s );

int is_ext ( const char *a, const char *b ); // compara extensiones
#endif

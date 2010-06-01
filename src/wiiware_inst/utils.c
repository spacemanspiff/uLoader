/*
	Copyright (C) 2010 Hermes.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "utils.h"

void lower_caps ( char * s )
{
        while ( *s ) {
                if ( *s>='A' && *s<='Z' )
                        *s += 0x20;
                s++;
        }
}

int is_ext ( const char *a, const char *b ) // compara extensiones
{
        int n,m;
        m=0;
        n=0;
        while ( a[n] ) {
                if ( a[n]=='.' )
                        m=n;
                n++;
        }

        n=0;
        while ( b[n]!=0 ) {
                unsigned char t1,t2;
                t1=a[m];
                t2=b[n];
                if ( t1>='A' && t1<='Z' )
                        t1 += 32;
                if ( t2>='A' && t2<='Z' )
                        t2 += 32;
                if ( t1!=t2 )
                        return 0;
                m++;
                n++;
        }
        if ( a[m]==0 )
                return 1;
        return 0;
}


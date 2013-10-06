/* rtp_helper.c
*
* Has some standard functions. !Red!
*
*
* Copyright (C) 2013 Tox project All Rights Reserved.
*
* This file is part of Tox.
*
* Tox is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Tox is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Tox. If not, see <http://www.gnu.org/licenses/>.
*
*/


#include "rtp_helper.h"
#include "network.h"

#include <arpa/inet.h> /* Fixes implicit function warning. */
#include <assert.h>

static int _seed = -1; /* Not initiated */

int t_setipport ( const char* _ip, unsigned short _port, void* _dest )
{
    if ( !_dest ) {
        return FAILURE;
    }

    IP_Port* _dest_c = ( IP_Port* ) _dest;
    ip_init(&_dest_c->ip, 0);

    IP_Port _ipv6_garbage;

    if ( !addr_resolve(_ip, &_dest_c->ip, &_ipv6_garbage.ip) )
        return FAILURE;

    _dest_c->port = htons ( _port );

    return SUCCESS;
}

uint32_t t_random ( uint32_t _max )
{
    if ( _seed < 0 ) {
        srand ( _time );
        _seed++;
    }

    if ( _max <= 0 ) {
        return ( unsigned ) rand();
    } else {
        return ( unsigned ) rand() % _max;
    }
}

void t_memcpy ( uint8_t* _dest, const uint8_t* _source, size_t _size )
{
    /*
     * Using countdown to zero method
     * It's faster than for(_it = 0; _it < _size; _it++);
     */
    size_t _it = _size;

    do {
        _it--;
        _dest[_it] = _source[_it];
    } while ( _it );

}

uint8_t* t_memset ( uint8_t* _dest, uint8_t _valu, size_t _size )
{
    /*
     * Again using countdown to zero method
     */
    size_t _it = _size;

    do {
        _it--;
        _dest[_it] = _valu;
    } while ( _it );

    return _dest;
}

size_t t_memlen ( const uint8_t* _valu)
{
    const uint8_t* _it;
    size_t _retu = 0;

    for ( _it = _valu; *_it; ++_it ) ++_retu;

    return _retu;
}

uint8_t* t_strallcpy ( const uint8_t* _source ) /* string alloc and copy */
{
    if ( !_source )
        return NULL;

    size_t _length = t_memlen(_source) + 1; /* make space for null character */

    uint8_t* _dest = malloc( sizeof ( uint8_t ) * _length );

    t_memcpy(_dest, _source, _length);

    return _dest;
}

size_t t_strfind ( const uint8_t* _str, const uint8_t* _substr )
{
    size_t _pos = 0;
    size_t _it, _delit = 0;

    for ( _it = 0; _str[_it] != '\0'; _it++ ){
        if ( _str[_it] == _substr[_delit] ){
            _pos = _it;
            while ( _str[_it] == _substr[_delit] && _str[_it] != '\0' ){
                _it ++;
                _delit++;

                if ( _substr[_delit] == '\0' ){
                    return _pos;
                }
            }
            _delit = 0;
            _pos = 0;
        }
    }
    return _pos;
}

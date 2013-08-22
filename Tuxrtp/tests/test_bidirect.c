#include "../rtp_impl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>
#include <assert.h>

#include "test_helper.h"

int print_help( const char* name )
{
    char* _help = malloc( 300 );
    memset(_help, '\0', 300);

    strcat(_help, " Usage: ");
    strcat(_help, name);
    strcat(_help, "\n -d IP ( destination )\n"
                    " -p PORT ( dest Port )\n"
                    " -l PORT ( listen Port ) \n");

    puts ( _help );

    free(_help);
    return FAILURE;
}

int main ( int argc, char* argv[] )
{
    int status;
    IP_Port     Ip_port;
    const char* ip, *psend, *plisten;
    uint16_t    port_send, port_listen;
    const char* test_bytes = "0123456789012345678901234567890123456789012345678901234567890123456789"
                             "0123456789012345678901234567890123456789012345678901234567890123456789"
                             "0123456789012345678901234567890123456789012345678901234567890123456789"
                             "0123456789012345678901234567890123456789012345678901234567890123456789";


    rtp_session_t* _m_session;
    rtp_msg_t     *_m_msg_R, *_m_msg_S;
    arg_t* _list = parse_args ( argc, argv );

    ip = find_arg_duble(_list, "-d");
    psend = find_arg_duble(_list, "-p");
    plisten = find_arg_duble(_list, "-l");

    if ( !ip || !plisten || !psend )
        return print_help(argv[0]);

    port_send = atoi(psend);
    port_listen = atoi(plisten);

    IP_Port local, remote;

    /*
     * This is the Local ip. We initiate networking on
     * this value for it's the local one. To make stuff simpler we receive over this value
     * and send on the other one ( see remote )
     */
    local.ip.i = htonl(INADDR_ANY);
    local.port = port_listen;
    status = init_networking(local.ip, port_listen);


    /*
     * Now this is the remote. It's used by rtp_session_t to determine the receivers ip etc.
     */
    set_ip_port ( ip, port_send, &remote );
    _m_session = rtp_init_session(-1);
    rtp_add_receiver( _m_session, &remote );

    /* Now let's start our main loop in both recv and send mode */

    for ( ;; )
    {
        /*
         * This part checks for received messages and if gotten one
         * display 'Received msg!' indicator and free message
         */
        _m_msg_R = rtp_recv_msg ( _m_session );

        if ( _m_msg_R ) {
            puts ( "Received msg!" );
            rtp_free_msg(_m_session, _m_msg_R);
        }
        /* -------------------- */

        /*
         * This one makes a test msg and sends that message to the 'remote'
         */
        _m_msg_S = rtp_msg_new ( _m_session, test_bytes, 280 ) ;
        rtp_send_msg ( _m_session, _m_msg_S );
        usleep ( 10000 );
        /* -------------------- */
    }

    return SUCCESS;
}

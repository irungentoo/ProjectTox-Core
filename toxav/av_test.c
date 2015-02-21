#include "toxav.h"
#include "../toxcore/tox.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#if defined(_WIN32) || defined(__WIN32__) || defined (WIN32)
#define c_sleep(x) Sleep(1*x)
#else
#include <unistd.h>
#define c_sleep(x) usleep(1000*x)
#endif

typedef struct {
    bool incoming;
    bool ringing;
    bool ended;
    bool errored;
    bool sending;
    bool paused;
} CallControl;


/** 
 * Callbacks 
 */
void t_toxav_call_cb(ToxAV *av, uint32_t friend_number, bool audio_enabled, bool video_enabled, void *user_data)
{
    printf("Handling CALL callback\n");
    ((CallControl*)user_data)->incoming = true;
}
void t_toxav_call_state_cb(ToxAV *av, uint32_t friend_number, TOXAV_CALL_STATE state, void *user_data)
{
    printf("Handling CALL STATE callback: ");
    
    if (((CallControl*)user_data)->paused)
        ((CallControl*)user_data)->paused = false;
    
    switch (state)
    {            
        case TOXAV_CALL_STATE_NOT_SENDING: {
            printf("Not sending");
            ((CallControl*)user_data)->sending = false;
        } break;
        
        case TOXAV_CALL_STATE_SENDING_A: {
            printf("Sending Audio");
            ((CallControl*)user_data)->sending = true;
        } break;
        
        case TOXAV_CALL_STATE_SENDING_V: {
            printf("Sending Video");
            ((CallControl*)user_data)->sending = true;
        } break;
        
        case TOXAV_CALL_STATE_SENDING_AV: {
            printf("Sending Both");
            ((CallControl*)user_data)->sending = true;
        } break;
        
        case TOXAV_CALL_STATE_PAUSED: {
            printf("Paused");
            ((CallControl*)user_data)->paused = true;
            ((CallControl*)user_data)->sending = false;
        } break;
        
        case TOXAV_CALL_STATE_END: {
            printf("Ended");
            ((CallControl*)user_data)->ended = true;
            ((CallControl*)user_data)->sending = false;
        } break;
        
        case TOXAV_CALL_STATE_ERROR: {
            printf("Error");
            ((CallControl*)user_data)->errored = true;
            ((CallControl*)user_data)->sending = false;
        } break;
    }
    
    printf("\n");
}
void t_toxav_receive_video_frame_cb(ToxAV *av, uint32_t friend_number,
                                    uint16_t width, uint16_t height,
                                    uint8_t const *planes[], int32_t const stride[],
                                    void *user_data)
{
    printf("Handling VIDEO FRAME callback\n");
}
void t_toxav_receive_audio_frame_cb(ToxAV *av, uint32_t friend_number,
                                    int16_t const *pcm,
                                    size_t sample_count,
                                    uint8_t channels,
                                    uint32_t sampling_rate,
                                    void *user_data)
{
    printf("Handling AUDIO FRAME callback\n");
}
void t_accept_friend_request_cb(Tox *m, const uint8_t *public_key, const uint8_t *data, uint16_t length, void *userdata)
{
    if (length == 7 && memcmp("gentoo", data, 7) == 0) {
        tox_add_friend_norequest(m, public_key);
    }
}


/**
 */
void prepare(Tox* Bsn, Tox* Alice, Tox* Bob)
{
    long long unsigned int cur_time = time(NULL);
    
    uint32_t to_compare = 974536;
    uint8_t address[TOX_FRIEND_ADDRESS_SIZE];
    
    tox_callback_friend_request(Alice, t_accept_friend_request_cb, &to_compare);
    tox_get_address(Alice, address);
    
    assert(tox_add_friend(Bob, address, (uint8_t *)"gentoo", 7) >= 0);
    
    uint8_t off = 1;
    
    while (1) {
        tox_do(Bsn);
        tox_do(Alice);
        tox_do(Bob);
        
        if (tox_isconnected(Bsn) && tox_isconnected(Alice) && tox_isconnected(Bob) && off) {
            printf("Toxes are online, took %llu seconds\n", time(NULL) - cur_time);
            off = 0;
        }
        
        if (tox_get_friend_connection_status(Alice, 0) == 1 && tox_get_friend_connection_status(Bob, 0) == 1)
            break;
        
        c_sleep(20);
    }
    
    printf("All set after %llu seconds!\n", time(NULL) - cur_time);
}
void prepareAV(ToxAV* AliceAV, void* AliceUD, ToxAV* BobAV, void* BobUD)
{
    /* Alice */
    toxav_callback_call(AliceAV, t_toxav_call_cb, AliceUD);
    toxav_callback_call_state(AliceAV, t_toxav_call_state_cb, AliceUD);
    toxav_callback_receive_video_frame(AliceAV, t_toxav_receive_video_frame_cb, AliceUD);
    toxav_callback_receive_audio_frame(AliceAV, t_toxav_receive_audio_frame_cb, AliceUD);
    
    /* Bob */
    toxav_callback_call(BobAV, t_toxav_call_cb, BobUD);
    toxav_callback_call_state(BobAV, t_toxav_call_state_cb, BobUD);
    toxav_callback_receive_video_frame(BobAV, t_toxav_receive_video_frame_cb, BobUD);
    toxav_callback_receive_audio_frame(BobAV, t_toxav_receive_audio_frame_cb, BobUD);
}
void iterate(Tox* Bsn, ToxAV* AliceAV, ToxAV* BobAV)
{
    tox_do(Bsn);
    tox_do(toxav_get_tox(AliceAV));
    tox_do(toxav_get_tox(BobAV));
    
    toxav_iteration(AliceAV);
    toxav_iteration(BobAV);
    
    c_sleep(20);
}


int main (int argc, char** argv)
{
    Tox *Bsn = tox_new(0);
    Tox *Alice = tox_new(0);
    Tox *Bob = tox_new(0);
    
    assert(Bsn && Alice && Bob);
    
    prepare(Bsn, Alice, Bob);
    
    
    ToxAV *AliceAV, *BobAV;
    CallControl AliceCC, BobCC;
    
    {
        TOXAV_ERR_NEW rc;
        AliceAV = toxav_new(Alice, &rc);
        assert(rc == TOXAV_ERR_NEW_OK);
        
        BobAV = toxav_new(Bob, &rc);
        assert(rc == TOXAV_ERR_NEW_OK);
        
        prepareAV(AliceAV, &AliceCC, BobAV, &BobCC);
        printf("Created 2 instances of ToxAV\n");
    }
    

#define REGULAR_CALL_FLOW(A_BR, V_BR) \
    { \
        memset(&AliceCC, 0, sizeof(CallControl)); \
        memset(&BobCC, 0, sizeof(CallControl)); \
        \
        TOXAV_ERR_CALL rc; \
        toxav_call(AliceAV, 0, A_BR, V_BR, &rc); \
        \
        if (rc != TOXAV_ERR_CALL_OK) { \
            printf("toxav_call failed: %d\n", rc); \
            exit(1); \
        } \
        \
        \
        long long unsigned int start_time = time(NULL); \
        \
        \
        while (!AliceCC.ended || !BobCC.ended) { \
            \
            if (BobCC.incoming) { \
                TOXAV_ERR_ANSWER rc; \
                toxav_answer(BobAV, 0, 48, 4000, &rc); \
                \
                if (rc != TOXAV_ERR_ANSWER_OK) { \
                    printf("toxav_answer failed: %d\n", rc); \
                    exit(1); \
                } \
                BobCC.incoming = false; \
                BobCC.sending = true; /* There is no more start callback when answering */\
            } \
            else if (AliceCC.sending && BobCC.sending) { \
                /* TODO rtp */ \
                \
                if (time(NULL) - start_time == 5) { \
                    \
                    TOXAV_ERR_CALL_CONTROL rc; \
                    toxav_call_control(AliceAV, 0, TOXAV_CALL_CONTROL_CANCEL, &rc); \
                    AliceCC.ended = true; /* There is no more end callback when hanging up */\
                    \
                    if (rc != TOXAV_ERR_CALL_CONTROL_OK) { \
                        printf("toxav_call_control failed: %d\n", rc); \
                        exit(1); \
                    } \
                } \
            } \
             \
            iterate(Bsn, AliceAV, BobAV); \
        } \
        printf("Success!\n");\
    }
    
    printf("\nTrying regular call (Audio and Video)...\n");
//     REGULAR_CALL_FLOW(48, 4000);
    
    printf("\nTrying regular call (Audio only)...\n");
//     REGULAR_CALL_FLOW(48, 0);
    
    printf("\nTrying regular call (Video only)...\n");
//     REGULAR_CALL_FLOW(0, 4000);
    
#undef REGULAR_CALL_FLOW
    
    { /* Alice calls; Bob rejects */
        printf("\nTrying reject flow...\n");
        
        memset(&AliceCC, 0, sizeof(CallControl));
        memset(&BobCC, 0, sizeof(CallControl));
        
        {
            TOXAV_ERR_CALL rc;
            toxav_call(AliceAV, 0, 48, 0, &rc);
            
            if (rc != TOXAV_ERR_CALL_OK) {
                printf("toxav_call failed: %d\n", rc);
                exit(1);
            }
        }
        
        while (!BobCC.incoming)
            iterate(Bsn, AliceAV, BobAV);
        
        /* Reject */
        {
            TOXAV_ERR_CALL_CONTROL rc;
            toxav_call_control(BobAV, 0, TOXAV_CALL_CONTROL_CANCEL, &rc);
            
            if (rc != TOXAV_ERR_CALL_CONTROL_OK) {
                printf("toxav_call_control failed: %d\n", rc);
                exit(1);
            }
        }
        
        while (!AliceCC.ended)
            iterate(Bsn, AliceAV, BobAV);
        
        printf("Success!\n");
    }
    
    { /* Alice calls; Alice cancels while ringing */
        printf("\nTrying cancel (while ringing) flow...\n");
        
        memset(&AliceCC, 0, sizeof(CallControl));
        memset(&BobCC, 0, sizeof(CallControl));
        
        {
            TOXAV_ERR_CALL rc;
            toxav_call(AliceAV, 0, 48, 0, &rc);
            
            if (rc != TOXAV_ERR_CALL_OK) {
                printf("toxav_call failed: %d\n", rc);
                exit(1);
            }
        }
        
        while (!BobCC.incoming)
            iterate(Bsn, AliceAV, BobAV);
        
        /* Cancel */
        {
            TOXAV_ERR_CALL_CONTROL rc;
            toxav_call_control(AliceAV, 0, TOXAV_CALL_CONTROL_CANCEL, &rc);
            
            if (rc != TOXAV_ERR_CALL_CONTROL_OK) {
                printf("toxav_call_control failed: %d\n", rc);
                exit(1);
            }
        }
        
        /* Alice will not receive end state */
        while (!BobCC.ended)
            iterate(Bsn, AliceAV, BobAV);
        
        printf("Success!\n");
    }
    
    printf("\nTest successful!\n");
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <crypto/cryptodev.h>

#define DATA_SIZE           (8*1024)

#define AES_BLOCK_SIZE      16
#define AES_ECB_KEY_SIZE    32
#define AES_CBC_KEY_SIZE    16
#define AES_CTR_KEY_SIZE    16
#define AES_GCM_KEY_SIZE    16
#define AES_GCM_AUTH_SIZE   31

#define DES_BLOCK_SIZE      8
#define DES_CBC_KEY_SIZE    8
#define DES3_CBC_KEY_SIZE   24

static int aes_gcm_stop   = 0;
static int aes_cbc_stop   = 0;
static int aes_ecb_stop   = 0;
static int aes_ctr_stop   = 0;
static int des_cbc_stop   = 0;
static int des3_cbc_stop  = 0;
static int sleep_sec      = 1;
static int debug          = 0;
static int perf           = 0;
static int switch_pattern = 0;
static int key_from_efuse = 0;

static unsigned char EFUSE_KEY_HDR[8]  = {0x65, 0x6B, 0x65, 0x79, 0x00, 0x00, 0x00, 0x00};  ///< byte[0...3] as magic tag, byte[4] as key offset (word unit)

/******************************************************************
 NA51055 efuse key section 640bit => 20 word
 =============================================================
 key#   offset  word[0]  word[1]  word[2]  word[3]
 =============================================================
 key#0  [ 0]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
 key#1  [ 4]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
 key#2  [ 8]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
 key#3  [12]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
 key#4  [16]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
*******************************************************************/

static void show_menu(void)
{
    printf("\n");
    printf(" (1)AES_GCM        start/stop\n");
    printf(" (2)AES_CBC        start/stop\n");
    printf(" (3)AES_ECB        start/stop\n");
    printf(" (4)AES_CTR        start/stop\n");
    printf(" (5)3DES_CBC       start/stop\n");
    printf(" (6)DES_CBC        start/stop\n");
    printf(" (D)Debug          On/Off\n");
    printf(" (P)Performance    On/Off\n");
    printf(" (S)Switch Pattern On/Off\n");
    printf(" (E)Key From eFuse On/Off\n");
    printf(" (Q)Exit\n");
    fflush(stdout);
}

static void *aes_ecb_thread_main(void *arg)
{
    int               cfd = -1;
    uint8_t           *plaintext  = NULL;
    uint8_t           *ciphertext = NULL;
    uint8_t           key[AES_ECB_KEY_SIZE];
    struct session_op sess;
    struct crypt_op   cryp;
    struct timeval    t_start;
    struct timeval    t_end;
    unsigned long     diff_us;

    printf("AES_ECB thread start!\n");

    /* Open the crypto device */
    cfd = open("/dev/crypto", O_RDWR, 0);
    if (cfd < 0) {
        perror("open(/dev/crypto)");
        goto exit;
    }

    plaintext = malloc(DATA_SIZE);
    if (!plaintext) {
        perror("fail to allocate plaintext buffer\n");
        goto close_cfd;
    }

    ciphertext = malloc(DATA_SIZE);
    if (!ciphertext) {
        perror("fail to allocate ciphertext buffer\n");
        goto free_mem;
    }

    memset(&sess,        0, sizeof(sess));
    memset(&cryp,        0, sizeof(cryp));

    /* prepare test data */
    memset(key,       0x33, sizeof(key));
    memset(plaintext, 0x13, DATA_SIZE);

    if (key_from_efuse) {
        memcpy(key, EFUSE_KEY_HDR, sizeof(EFUSE_KEY_HDR));
        key[4] = 0x00;  ///< key offset = 0
    }

    /* get crypto session */
    sess.cipher = CRYPTO_AES_ECB;
    sess.keylen = AES_ECB_KEY_SIZE;
    sess.key    = key;
    if (ioctl(cfd, CIOCGSESSION, &sess)) {
        perror("ioctl(CIOCGSESSION)");
        goto free_mem;
    }

    while (aes_ecb_stop == 0) {
        /* Encryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = plaintext;
        cryp.dst = ciphertext;
        cryp.iv  = NULL;
        cryp.op  = COP_ENCRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("AES_ECB size:%u encryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        if (debug) {
            printf("AES_ECB  Plaintext[0]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[0], plaintext[1], plaintext[2], plaintext[3],
                   plaintext[4], plaintext[5], plaintext[6], plaintext[7]);
            printf("AES_ECB  Plaintext[8]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[8],  plaintext[9],  plaintext[10], plaintext[11],
                   plaintext[12], plaintext[13], plaintext[14], plaintext[15]);

            printf("AES_ECB  Ciphertext[0] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[0], ciphertext[1], ciphertext[2], ciphertext[3],
                   ciphertext[4], ciphertext[5], ciphertext[6], ciphertext[7]);
            printf("AES_ECB  Ciphertext[8] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[8],  ciphertext[9],  ciphertext[10], ciphertext[11],
                   ciphertext[12], ciphertext[13], ciphertext[14], ciphertext[15]);
        }

        /* Decryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = ciphertext;
        cryp.dst = ciphertext;
        cryp.iv  = NULL;
        cryp.op  = COP_DECRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("AES_ECB size:%u decryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        /* Verify the result */
        if (memcmp(plaintext, ciphertext, DATA_SIZE) != 0) {
            printf("AES_ECB  test failed!\n");
            goto free_session;
        }
        else if (debug)
            printf("AES_ECB  test passed!\n");

        if (switch_pattern) {
            memset(plaintext, (random()&0xff), random()%DATA_SIZE);
        }

        if (sleep_sec)
            sleep(sleep_sec);
    }

free_session:
    /* finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
        perror("ioctl(CIOCFSESSION)");
    }

free_mem:
    if (plaintext)
        free(plaintext);

    if (ciphertext)
        free(ciphertext);

close_cfd:
    /* Close file descriptor */
    if (close(cfd))
        perror("close(cfd)");

exit:
    printf("AES_ECB thread exit!\n");

    pthread_exit(NULL);
}

static void *aes_cbc_thread_main(void *arg)
{
    int               cfd = -1;
    uint8_t           *plaintext  = NULL;
    uint8_t           *ciphertext = NULL;
    uint8_t           iv[AES_BLOCK_SIZE];
    uint8_t           key[AES_CBC_KEY_SIZE];
    struct session_op sess;
    struct crypt_op   cryp;
    struct timeval    t_start;
    struct timeval    t_end;
    unsigned long     diff_us;

    printf("AES_CBC thread start!\n");

    /* Open the crypto device */
    cfd = open("/dev/crypto", O_RDWR, 0);
    if (cfd < 0) {
        perror("open(/dev/crypto)");
        goto exit;
    }

    plaintext = malloc(DATA_SIZE);
    if (!plaintext) {
        perror("fail to allocate plaintext buffer\n");
        goto close_cfd;
    }

    ciphertext = malloc(DATA_SIZE);
    if (!ciphertext) {
        perror("fail to allocate ciphertext buffer\n");
        goto free_mem;
    }

    memset(&sess,        0, sizeof(sess));
    memset(&cryp,        0, sizeof(cryp));

    /* prepare test data */
    memset(key,       0x44, sizeof(key));
    memset(iv,        0x04, sizeof(iv));
    memset(plaintext, 0x14, DATA_SIZE);

    if (key_from_efuse) {
        memcpy(key, EFUSE_KEY_HDR, sizeof(EFUSE_KEY_HDR));
        key[4] = 0x04;  ///< key offset = 4
    }

    /* get crypto session */
    sess.cipher = CRYPTO_AES_CBC;
    sess.keylen = AES_CBC_KEY_SIZE;
    sess.key    = key;
    if (ioctl(cfd, CIOCGSESSION, &sess)) {
        perror("ioctl(CIOCGSESSION)");
        goto free_mem;
    }

    while (aes_cbc_stop == 0) {
        /* Encryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = plaintext;
        cryp.dst = ciphertext;
        cryp.iv  = iv;
        cryp.op  = COP_ENCRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("AES_CBC size:%u encryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        if (debug) {
            printf("AES_CBC  Plaintext[0]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[0], plaintext[1], plaintext[2], plaintext[3],
                   plaintext[4], plaintext[5], plaintext[6], plaintext[7]);
            printf("AES_CBC  Plaintext[8]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[8],  plaintext[9],  plaintext[10], plaintext[11],
                   plaintext[12], plaintext[13], plaintext[14], plaintext[15]);

            printf("AES_CBC  Ciphertext[0] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[0], ciphertext[1], ciphertext[2], ciphertext[3],
                   ciphertext[4], ciphertext[5], ciphertext[6], ciphertext[7]);
            printf("AES_CBC  Ciphertext[8] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[8],  ciphertext[9],  ciphertext[10], ciphertext[11],
                   ciphertext[12], ciphertext[13], ciphertext[14], ciphertext[15]);
        }

        /* Decryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = ciphertext;
        cryp.dst = ciphertext;
        cryp.iv  = iv;
        cryp.op  = COP_DECRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("AES_CBC size:%u decryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        /* Verify the result */
        if (memcmp(plaintext, ciphertext, DATA_SIZE) != 0) {
            printf("AES_CBC  test failed!\n");
            goto free_session;
        }
        else if (debug)
            printf("AES_CBC  test passed!\n");

        if (switch_pattern) {
            memset(plaintext, (random()&0xff), random()%DATA_SIZE);
        }

        if (sleep_sec)
            sleep(sleep_sec);
    }

free_session:
    /* finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
        perror("ioctl(CIOCFSESSION)");
    }

free_mem:
    if (plaintext)
        free(plaintext);

    if (ciphertext)
        free(ciphertext);

close_cfd:
    /* Close file descriptor */
    if (close(cfd))
        perror("close(cfd)");

exit:
    printf("AES_CBC thread exit!\n");
    pthread_exit(NULL);
}

static void *aes_gcm_thread_main(void *arg)
{
    int                  cfd = -1;
    uint8_t              *plaintext  = NULL;
    uint8_t              *ciphertext = NULL;
    uint8_t              *auth       = NULL;
    uint8_t              iv[AES_BLOCK_SIZE];
    uint8_t              key[AES_GCM_KEY_SIZE];
    struct session_op    sess;
    struct crypt_auth_op cao;
    struct timeval       t_start;
    struct timeval       t_end;
    unsigned long        diff_us;

    printf("AES_GCM thread start!\n");

    /* Open the crypto device */
    cfd = open("/dev/crypto", O_RDWR, 0);
    if (cfd < 0) {
        perror("open(/dev/crypto)");
        goto exit;
    }

    plaintext = malloc(DATA_SIZE);
    if (!plaintext) {
        perror("fail to allocate plaintext buffer\n");
        goto close_cfd;
    }

    ciphertext = malloc(DATA_SIZE+16);  ///< AuthTag is 16 bytes
    if (!ciphertext) {
        perror("fail to allocate ciphertext buffer\n");
        goto free_mem;
    }

    auth = malloc(AES_GCM_AUTH_SIZE);
    if (!auth) {
        perror("fail to allocate auth buffer\n");
        goto free_mem;
    }

    memset(&sess, 0, sizeof(sess));
    memset(&cao,  0, sizeof(cao));

    /* prepare test data */
    memset(key,       0x55, sizeof(key));
    memset(iv,        0x05, sizeof(iv));
    memset(auth,      0xf5, AES_GCM_AUTH_SIZE);
    memset(plaintext, 0x15, DATA_SIZE);

    if (key_from_efuse) {
        memcpy(key, EFUSE_KEY_HDR, sizeof(EFUSE_KEY_HDR));
        key[4] = 0x08;  ///< key offset = 8
    }

    /* get crypto session */
    sess.cipher = CRYPTO_AES_GCM;
    sess.keylen = AES_GCM_KEY_SIZE;
    sess.key    = key;
    if (ioctl(cfd, CIOCGSESSION, &sess)) {
        perror("ioctl(CIOCGSESSION)");
        goto free_mem;
    }

    while (aes_gcm_stop == 0) {
        /* Encryption */
        cao.ses      = sess.ses;
        cao.auth_src = auth;
        cao.auth_len = AES_GCM_AUTH_SIZE;
        cao.len      = DATA_SIZE;
        cao.src      = plaintext;
        cao.dst      = ciphertext;
        cao.iv       = iv;
        cao.iv_len   = 12;
        cao.op       = COP_ENCRYPT;
        cao.flags    = 0;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCAUTHCRYPT, &cao)) {
            perror("ioctl(CIOCAUTHCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("AES_GCM size:%u encryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        if (debug) {
            printf("AES_GCM  Plaintext[0]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[0], plaintext[1], plaintext[2], plaintext[3],
                   plaintext[4], plaintext[5], plaintext[6], plaintext[7]);
            printf("AES_GCM  Plaintext[8]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[8],  plaintext[9],  plaintext[10], plaintext[11],
                   plaintext[12], plaintext[13], plaintext[14], plaintext[15]);

            printf("AES_GCM  Ciphertext[0] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[0], ciphertext[1], ciphertext[2], ciphertext[3],
                   ciphertext[4], ciphertext[5], ciphertext[6], ciphertext[7]);
            printf("AES_GCM  Ciphertext[8] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[8],  ciphertext[9],  ciphertext[10], ciphertext[11],
                   ciphertext[12], ciphertext[13], ciphertext[14], ciphertext[15]);
        }

        /* Decryption */
        cao.ses      = sess.ses;
        cao.auth_src = auth;
        cao.auth_len = AES_GCM_AUTH_SIZE;
        cao.len      = DATA_SIZE+16;
        cao.src      = ciphertext;
        cao.dst      = ciphertext;
        cao.iv       = iv;
        cao.iv_len   = 12;
        cao.op       = COP_DECRYPT;
        cao.flags    = 0;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCAUTHCRYPT, &cao)) {
            perror("ioctl(CIOCAUTHCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("AES_GCM size:%u decryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        /* Verify the result */
        if (memcmp(plaintext, ciphertext, DATA_SIZE) != 0) {
            printf("AES_GCM  test failed!\n");
            goto free_session;
        }
        else if (debug)
            printf("AES_GCM  test passed!\n");

        if (switch_pattern) {
            memset(plaintext, (random()&0xff), random()%DATA_SIZE);
        }

        if (sleep_sec)
            sleep(sleep_sec);
    }

free_session:
    /* finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
        perror("ioctl(CIOCFSESSION)");
    }

free_mem:
    if (plaintext)
        free(plaintext);

    if (ciphertext)
        free(ciphertext);

    if (auth)
        free(auth);

close_cfd:
    /* Close file descriptor */
    if (close(cfd))
        perror("close(cfd)");

exit:
    printf("AES_GCM thread exit!\n");
    pthread_exit(NULL);
}

static void *aes_ctr_thread_main(void *arg)
{
    int               cfd = -1;
    uint8_t           *plaintext  = NULL;
    uint8_t           *ciphertext = NULL;
    uint8_t           iv[AES_BLOCK_SIZE];
    uint8_t           key[AES_CTR_KEY_SIZE];
    struct session_op sess;
    struct crypt_op   cryp;
    struct timeval    t_start;
    struct timeval    t_end;
    unsigned long     diff_us;

    printf("AES_CTR thread start!\n");

    /* Open the crypto device */
    cfd = open("/dev/crypto", O_RDWR, 0);
    if (cfd < 0) {
        perror("open(/dev/crypto)");
        goto exit;
    }

    plaintext = malloc(DATA_SIZE);
    if (!plaintext) {
        perror("fail to allocate plaintext buffer\n");
        goto close_cfd;
    }

    ciphertext = malloc(DATA_SIZE);
    if (!ciphertext) {
        perror("fail to allocate ciphertext buffer\n");
        goto free_mem;
    }

    memset(&sess,        0, sizeof(sess));
    memset(&cryp,        0, sizeof(cryp));

    /* prepare test data */
    memset(key,       0x66, sizeof(key));
    memset(iv,        0x06, sizeof(iv));
    memset(plaintext, 0x16, DATA_SIZE);

    if (key_from_efuse) {
        memcpy(key, EFUSE_KEY_HDR, sizeof(EFUSE_KEY_HDR));
        key[4] = 0x0c;  ///< key offset = 12
    }

    /* get crypto session */
    sess.cipher = CRYPTO_AES_CTR;
    sess.keylen = AES_CTR_KEY_SIZE;
    sess.key    = key;
    if (ioctl(cfd, CIOCGSESSION, &sess)) {
        perror("ioctl(CIOCGSESSION)");
        goto free_mem;
    }

    while (aes_ctr_stop == 0) {
        /* Encryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = plaintext;
        cryp.dst = ciphertext;
        cryp.iv  = iv;
        cryp.op  = COP_ENCRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("AES_CTR size:%u encryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        if (debug) {
            printf("AES_CTR  Plaintext[0]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[0], plaintext[1], plaintext[2], plaintext[3],
                   plaintext[4], plaintext[5], plaintext[6], plaintext[7]);
            printf("AES_CTR  Plaintext[8]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[8],  plaintext[9],  plaintext[10], plaintext[11],
                   plaintext[12], plaintext[13], plaintext[14], plaintext[15]);

            printf("AES_CTR  Ciphertext[0] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[0], ciphertext[1], ciphertext[2], ciphertext[3],
                   ciphertext[4], ciphertext[5], ciphertext[6], ciphertext[7]);
            printf("AES_CTR  Ciphertext[8] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[8],  ciphertext[9],  ciphertext[10], ciphertext[11],
                   ciphertext[12], ciphertext[13], ciphertext[14], ciphertext[15]);
        }

        /* Decryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = ciphertext;
        cryp.dst = ciphertext;
        cryp.iv  = iv;
        cryp.op  = COP_DECRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("AES_CTR size:%u decryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        /* Verify the result */
        if (memcmp(plaintext, ciphertext, DATA_SIZE) != 0) {
            printf("AES_CTR  test failed!\n");
            goto free_session;
        }
        else if (debug)
            printf("AES_CTR  test passed!\n");

        if (switch_pattern) {
            memset(plaintext, (random()&0xff), random()%DATA_SIZE);
        }

        if (sleep_sec)
            sleep(sleep_sec);
    }

free_session:
    /* finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
        perror("ioctl(CIOCFSESSION)");
    }

free_mem:
    if (plaintext)
        free(plaintext);

    if (ciphertext)
        free(ciphertext);

close_cfd:
    /* Close file descriptor */
    if (close(cfd))
        perror("close(cfd)");

exit:
    printf("AES_CTR thread exit!\n");
    pthread_exit(NULL);
}

static void *des3_cbc_thread_main(void *arg)
{
    int               cfd = -1;
    uint8_t           *plaintext  = NULL;
    uint8_t           *ciphertext = NULL;
    uint8_t           iv[DES_BLOCK_SIZE];
    uint8_t           key[DES3_CBC_KEY_SIZE];
    struct session_op sess;
    struct crypt_op   cryp;
    struct timeval    t_start;
    struct timeval    t_end;
    unsigned long     diff_us;

    printf("3DES_CBC thread start!\n");

    /* Open the crypto device */
    cfd = open("/dev/crypto", O_RDWR, 0);
    if (cfd < 0) {
        perror("open(/dev/crypto)");
        goto exit;
    }

    plaintext = malloc(DATA_SIZE);
    if (!plaintext) {
        perror("fail to allocate plaintext buffer\n");
        goto close_cfd;
    }

    ciphertext = malloc(DATA_SIZE);
    if (!ciphertext) {
        perror("fail to allocate ciphertext buffer\n");
        goto free_mem;
    }

    memset(&sess,        0, sizeof(sess));
    memset(&cryp,        0, sizeof(cryp));

    /* prepare test data */
    memset(key,       0x77, sizeof(key));
    memset(iv,        0x07, sizeof(iv));
    memset(plaintext, 0x17, DATA_SIZE);

    if (key_from_efuse) {
        memcpy(key, EFUSE_KEY_HDR, sizeof(EFUSE_KEY_HDR));
        key[4] = 0x01;  ///< key offset = 1
    }

    /* get crypto session */
    sess.cipher = CRYPTO_3DES_CBC;
    sess.keylen = DES3_CBC_KEY_SIZE;
    sess.key    = key;
    if (ioctl(cfd, CIOCGSESSION, &sess)) {
        perror("ioctl(CIOCGSESSION)");
        goto free_mem;
    }

    while (des3_cbc_stop == 0) {
        /* Encryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = plaintext;
        cryp.dst = ciphertext;
        cryp.iv  = iv;
        cryp.op  = COP_ENCRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("3DES_CBC size:%u encryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        if (debug) {
            printf("3DES_CBC  Plaintext[0]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[0], plaintext[1], plaintext[2], plaintext[3],
                   plaintext[4], plaintext[5], plaintext[6], plaintext[7]);
            printf("3DES_CBC  Plaintext[8]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[8],  plaintext[9],  plaintext[10], plaintext[11],
                   plaintext[12], plaintext[13], plaintext[14], plaintext[15]);

            printf("3DES_CBC  Ciphertext[0] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[0], ciphertext[1], ciphertext[2], ciphertext[3],
                   ciphertext[4], ciphertext[5], ciphertext[6], ciphertext[7]);
            printf("3DES_CBC  Claintext[8] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[8],  ciphertext[9],  ciphertext[10], ciphertext[11],
                   ciphertext[12], ciphertext[13], ciphertext[14], ciphertext[15]);
        }

        /* Decryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = ciphertext;
        cryp.dst = ciphertext;
        cryp.iv  = iv;
        cryp.op  = COP_DECRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("3DES_CBC size:%u decryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        /* Verify the result */
        if (memcmp(plaintext, ciphertext, DATA_SIZE) != 0) {
            printf("3DES_CBC test failed!\n");
            goto free_session;
        }
        else if (debug)
            printf("3DES_CBC test passed!\n");

        if (switch_pattern) {
            memset(plaintext, (random()&0xff), random()%DATA_SIZE);
        }

        if (sleep_sec)
            sleep(sleep_sec);
    }

free_session:
    /* finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
        perror("ioctl(CIOCFSESSION)");
    }

free_mem:
    if (plaintext)
        free(plaintext);

    if (ciphertext)
        free(ciphertext);

close_cfd:
    /* Close file descriptor */
    if (close(cfd))
        perror("close(cfd)");

exit:
    printf("3DES_CBC thread exit!\n");
    pthread_exit(NULL);
}

static void *des_cbc_thread_main(void *arg)
{
    int               cfd = -1;
    uint8_t           *plaintext  = NULL;
    uint8_t           *ciphertext = NULL;
    uint8_t           iv[DES_BLOCK_SIZE];
    uint8_t           key[DES_CBC_KEY_SIZE];
    struct session_op sess;
    struct crypt_op   cryp;
    struct timeval    t_start;
    struct timeval    t_end;
    unsigned long     diff_us;

    printf("DES_CBC thread start!\n");

    /* Open the crypto device */
    cfd = open("/dev/crypto", O_RDWR, 0);
    if (cfd < 0) {
        perror("open(/dev/crypto)");
        goto exit;
    }

    plaintext = malloc(DATA_SIZE);
    if (!plaintext) {
        perror("fail to allocate plaintext buffer\n");
        goto close_cfd;
    }

    ciphertext = malloc(DATA_SIZE);
    if (!ciphertext) {
        perror("fail to allocate ciphertext buffer\n");
        goto free_mem;
    }

    memset(&sess,        0, sizeof(sess));
    memset(&cryp,        0, sizeof(cryp));

    /* prepare test data */
    memset(key,       0x88, sizeof(key));
    memset(iv,        0x08, sizeof(iv));
    memset(plaintext, 0x18, DATA_SIZE);

    if (key_from_efuse) {
        memcpy(key, EFUSE_KEY_HDR, sizeof(EFUSE_KEY_HDR));
        key[4] = 0x05;  ///< key offset = 5
    }

    /* get crypto session */
    sess.cipher = CRYPTO_DES_CBC;
    sess.keylen = DES_CBC_KEY_SIZE;
    sess.key    = key;
    if (ioctl(cfd, CIOCGSESSION, &sess)) {
        perror("ioctl(CIOCGSESSION)");
        goto free_mem;
    }

    while (des_cbc_stop == 0) {
        /* Encryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = plaintext;
        cryp.dst = ciphertext;
        cryp.iv  = iv;
        cryp.op  = COP_ENCRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("DES_CBC size:%u encryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        if (debug) {
            printf("DES_CBC  Plaintext[0]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[0], plaintext[1], plaintext[2], plaintext[3],
                   plaintext[4], plaintext[5], plaintext[6], plaintext[7]);
            printf("DES_CBC  Plaintext[8]  => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   plaintext[8],  plaintext[9],  plaintext[10], plaintext[11],
                   plaintext[12], plaintext[13], plaintext[14], plaintext[15]);

            printf("DES_CBC  Ciphertext[0] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[0], ciphertext[1], ciphertext[2], ciphertext[3],
                   ciphertext[4], ciphertext[5], ciphertext[6], ciphertext[7]);
            printf("DES_CBC  Ciphertext[8] => %02x %02x %02x %02x %02x %02x %02x %02x\n",
                   ciphertext[8],  ciphertext[9],  ciphertext[10], ciphertext[11],
                   ciphertext[12], ciphertext[13], ciphertext[14], ciphertext[15]);
        }

        /* Decryption */
        cryp.ses = sess.ses;
        cryp.len = DATA_SIZE;
        cryp.src = ciphertext;
        cryp.dst = ciphertext;
        cryp.iv  = iv;
        cryp.op  = COP_DECRYPT;

        if (perf) {
            gettimeofday(&t_start, NULL);
        }

        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
            perror("ioctl(CIOCCRYPT)");
            goto free_session;
        }

        if (perf) {
            gettimeofday(&t_end, NULL);
            diff_us = (t_end.tv_sec*1000000   + t_end.tv_usec) - (t_start.tv_sec*1000000 + t_start.tv_usec);
            printf("DES_CBC size:%u decryption T:%lu (us)\n", DATA_SIZE, diff_us);
        }

        /* Verify the result */
        if (memcmp(plaintext, ciphertext, DATA_SIZE) != 0) {
            printf("DES_CBC  test failed!\n");
            goto free_session;
        }
        else if (debug)
            printf("DES_CBC  test passed!\n");

        if (switch_pattern) {
            memset(plaintext, (random()&0xff), random()%DATA_SIZE);
        }

        if (sleep_sec)
            sleep(sleep_sec);
    }

free_session:
    /* finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
        perror("ioctl(CIOCFSESSION)");
    }

free_mem:
    if (plaintext)
        free(plaintext);

    if (ciphertext)
        free(ciphertext);

close_cfd:
    /* Close file descriptor */
    if (close(cfd))
        perror("close(cfd)");

exit:
    printf("DES_CBC thread exit!\n");
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int       key;
    int       ret;
    int       to_background  = 0;
    int       is_exit        = 0;
    int       aes_gcm_start  = 0;
    int       aes_cbc_start  = 0;
    int       aes_ecb_start  = 0;
    int       aes_ctr_start  = 0;
    int       des_cbc_start  = 0;
    int       des3_cbc_start = 0;
    pthread_t aes_gcm_thread_id;
    pthread_t aes_cbc_thread_id;
    pthread_t aes_ecb_thread_id;
    pthread_t aes_ctr_thread_id;
    pthread_t des_cbc_thread_id;
    pthread_t des3_cbc_thread_id;
    pid_t     fpid;

    if (argc > 1) {
        sleep_sec = atoi(argv[1]);
    }

    if (argc > 2) {
        to_background = atoi(argv[2]);
    }

    show_menu();

    if (to_background) {
        fpid = fork();
        if (fpid < 0)
            printf("error in fork!");
        else if (fpid == 0) {
            printf("all thread start on background!\n");
            if(pthread_create(&aes_gcm_thread_id,  NULL, aes_gcm_thread_main,  NULL) == 0)
                aes_gcm_start = 1;
            if(pthread_create(&aes_cbc_thread_id,  NULL, aes_cbc_thread_main,  NULL) == 0)
                aes_cbc_start = 1;
            if(pthread_create(&aes_ecb_thread_id,  NULL, aes_ecb_thread_main,  NULL) == 0)
                aes_ecb_start = 1;
            if(pthread_create(&aes_ctr_thread_id,  NULL, aes_ctr_thread_main,  NULL) == 0)
                aes_ctr_start = 1;
            if(pthread_create(&des3_cbc_thread_id, NULL, des3_cbc_thread_main, NULL) == 0)
                des3_cbc_start = 1;
            if(pthread_create(&des_cbc_thread_id,  NULL, des_cbc_thread_main,  NULL) == 0)
                des_cbc_start = 1;

            /* child process run */
            while (1) {
                sleep(5);
            }
        }
        else {
            /* parent process exit */
            goto exit;
        }
    }

    while (!is_exit) {
        key = getchar();
        switch(key) {
        case '1':   /* start/stop AES_GCM thread */
            if (aes_gcm_start == 0) {
                aes_gcm_stop = 0;
                ret = pthread_create(&aes_gcm_thread_id, NULL, aes_gcm_thread_main, NULL);
                if(ret)
                    printf("create AES_GCM test thread failed!\n");
                else
                    aes_gcm_start = 1;
            }
            else {
                aes_gcm_stop = 1;
                pthread_join(aes_gcm_thread_id, NULL);
                aes_gcm_start = 0;
            }
            break;
        case '2':   /* start/stop AES_CBC thread */
            if (aes_cbc_start == 0) {
                aes_cbc_stop = 0;
                ret = pthread_create(&aes_cbc_thread_id, NULL, aes_cbc_thread_main, NULL);
                if(ret)
                    printf("create AES_CBC test thread failed!\n");
                else
                    aes_cbc_start = 1;
            }
            else {
                aes_cbc_stop = 1;
                pthread_join(aes_cbc_thread_id, NULL);
                aes_cbc_start = 0;
            }
            break;
        case '3':   /* start/stop AES_ECB thread */
            if (aes_ecb_start == 0) {
                aes_ecb_stop = 0;
                ret = pthread_create(&aes_ecb_thread_id, NULL, aes_ecb_thread_main, NULL);
                if(ret)
                    printf("create AES_ECB test thread failed!\n");
                else
                    aes_ecb_start = 1;
            }
            else {
                aes_ecb_stop = 1;
                pthread_join(aes_ecb_thread_id, NULL);
                aes_ecb_start = 0;
            }
            break;
        case '4':   /* start/stop AES_CTR thread */
            if (aes_ctr_start == 0) {
                aes_ctr_stop = 0;
                ret = pthread_create(&aes_ctr_thread_id, NULL, aes_ctr_thread_main, NULL);
                if(ret)
                    printf("create AES_CTR test thread failed!\n");
                else
                    aes_ctr_start = 1;
            }
            else {
                aes_ctr_stop = 1;
                pthread_join(aes_ctr_thread_id, NULL);
                aes_ctr_start = 0;
            }
            break;
        case '5':   /* start/stop 3DES_CBC thread */
            if (des3_cbc_start == 0) {
                des3_cbc_stop = 0;
                ret = pthread_create(&des3_cbc_thread_id, NULL, des3_cbc_thread_main, NULL);
                if(ret)
                    printf("create 3DES_CBC test thread failed!\n");
                else
                    des3_cbc_start = 1;
            }
            else {
                des3_cbc_stop = 1;
                pthread_join(des3_cbc_thread_id, NULL);
                des3_cbc_start = 0;
            }
            break;
        case '6':   /* start/stop DES_CBC thread */
            if (des_cbc_start == 0) {
                des_cbc_stop = 0;
                ret = pthread_create(&des_cbc_thread_id, NULL, des_cbc_thread_main, NULL);
                if(ret)
                    printf("create DES_CBC test thread failed!\n");
                else
                    des_cbc_start = 1;
            }
            else {
                des_cbc_stop = 1;
                pthread_join(des_cbc_thread_id, NULL);
                des_cbc_start = 0;
            }
            break;
        case 'D':   /* Enable/Disable debug */
        case 'd':
            debug ^= 1;
            printf("debug %s\n", debug ? "on" : "off");
            break;
        case 'P':
        case 'p':
            perf ^= 1;
            printf("performance %s\n", perf ? "on" : "off");
            break;
        case 'S':
        case 's':
            switch_pattern ^= 1;
            printf("switch pattern %s\n", switch_pattern ? "on" : "off");
            break;
        case 'Q':
        case 'q':
            is_exit = 1;
            break;
        case 'E':
        case 'e':
            key_from_efuse ^= 1;
            printf("key from efuse %s\n", key_from_efuse ? "on" : "off");
            break;
        default:
            show_menu();
            break;
        }
    }

exit:
    if (aes_gcm_start) {
        aes_gcm_stop = 1;
        pthread_join(aes_gcm_thread_id, NULL);
    }

    if (aes_cbc_start) {
        aes_cbc_stop = 1;
        pthread_join(aes_cbc_thread_id, NULL);
    }

    if (aes_ecb_start) {
        aes_ecb_stop = 1;
        pthread_join(aes_ecb_thread_id, NULL);
    }

    if (aes_ctr_start) {
        aes_ctr_stop = 1;
        pthread_join(aes_ctr_thread_id, NULL);
    }

    if (des3_cbc_start) {
        des3_cbc_stop = 1;
        pthread_join(des3_cbc_thread_id, NULL);
    }

    if (des_cbc_start) {
        des_cbc_stop = 1;
        pthread_join(des_cbc_thread_id, NULL);
    }

    return 0;
}

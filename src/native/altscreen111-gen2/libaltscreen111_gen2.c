#define _GNU_SOURCE
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "alt111.h"

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/*
 * MU1440 CarPlay AltScreen Gen-2 vehicle candidate.
 *
 * This source deliberately forks the vehicle-proven Run143 handshake/crypto
 * boundary but replaces UI ownership and local-consumer synchronization with
 * the independently tested alt111 C99 core. Run143 remains untouched.
 *
 *
 * Target baseline (hard gate in installer):
 *   MHI2_ER_SKG13_P4526_MU1440
 *   /mnt/app/eso/lib/libairplay.so
 *   SHA256 193A4FD9101EC2AA05E7159CFA307B96500810D379CA74A194F172ADC13A46B5
 *
 * IRC-parity design:
 *  - Keep the exact MU1440 stock AirPlay implementation and its platform media ABI.
 *  - Replay the recovered MHI2Q IRC negotiation semantics around that stock core.
 *  - Advertise root enabledFeatures before stream 111 is selected.
 *  - Clone stock display[0], remove the reference-proven non-portable fields, and
 *    append a minimally modified AltScreen display without forcing display type=111.
 *  - Pass the original SETUP request to stock first; on 111, clone the requested
 *    stream descriptor and append dataPort + streamID=111 to the stock response.
 *  - Keep startup showUI/forceKeyFrame disabled unless parity-specific opt-in is set.
 *  - Prologue-hook stock Setup/Start/TearDown with the vehicle-proven MAP_FIXED
 *    private-page fallback because stock local binding bypasses plain interposition.
 *  - Reuse stock per-screen AES derivation/AES-CTR and receive stream 111 on TCP 6031.
 *  - Convert AVCC H.264 to Annex-B and expose it on 127.0.0.1:19820.
 *  - Main CarPlay stream 110 remains stock-owned.
 */

typedef const void *CFTypeRef;
typedef const void *CFStringRef;
typedef const void *CFDictionaryRef;
typedef void *CFMutableDictionaryRef;
typedef const void *CFArrayRef;
typedef void *CFMutableArrayRef;
typedef const void *CFDataRef;
typedef long CFIndex;
typedef int32_t OSStatus;
typedef unsigned char Boolean;
typedef void *AirPlayReceiverSessionRef;

#define K_NO_ERR 0
#define K_STREAM_ALT 111
#define K_SCREEN_HDR_SIZE 128
#define K_SCREEN_VIDEO_FRAME 0
#define K_SCREEN_VIDEO_CONFIG 1
#define K_SCREEN_KEEPALIVE 2
#define K_SCREEN_FORCE_KEYFRAME 3
#define K_SCREEN_IGNORE 4
#define K_SCREEN_KEEPALIVE_BODY 5
#define CF_UTF8 0x08000100u
#define MAX_SCREEN_BODY (8u * 1024u * 1024u)
#define ALT_UUID_DEFAULT "b7e6c5a0-2222-4000-8000-000000000002"
#define ALT_URL_DEFAULT  "maps:/car/instrumentcluster"
#define ALT_URL_MAP      "maps:/car/instrumentcluster/map"
#define AIRPLAY_FEATURE_BIT26 (1ULL << 26)

/* Opaque storage. Stock AES_CTR_Context is smaller than this on the target. */
typedef union {
    uint64_t align;
    unsigned char bytes[512];
} aes_ctr_storage_t;

typedef struct {
    uint32_t bodySize;
    uint8_t opcode;
    uint8_t smallParam[3];
    uint8_t params[15 * 8];
} AirPlayScreenHeaderCompat;

typedef OSStatus (*fn_setup_t)(AirPlayReceiverSessionRef, CFDictionaryRef, CFDictionaryRef *);
typedef OSStatus (*fn_start_t)(AirPlayReceiverSessionRef, void *);
typedef void (*fn_teardown_t)(AirPlayReceiverSessionRef, CFDictionaryRef, OSStatus, Boolean *);
typedef CFDictionaryRef (*fn_serverinfo_t)(AirPlayReceiverSessionRef, CFArrayRef, uint8_t *, OSStatus *);
typedef void (*fn_command_completion_t)(OSStatus, CFDictionaryRef, void *);
typedef OSStatus (*fn_sendcmd_t)(AirPlayReceiverSessionRef, CFDictionaryRef, fn_command_completion_t, void *);
typedef OSStatus (*fn_aes_cbc_init_t)(void *, const uint8_t[16], const uint8_t[16], Boolean);
typedef OSStatus (*fn_aes_ctr_init_t)(void *, const uint8_t[16], const uint8_t[16]);
typedef OSStatus (*fn_aes_ctr_update_t)(void *, const void *, size_t, void *);
typedef void (*fn_aes_ctr_final_t)(void *);
typedef void (*fn_derive_screen_t)(const void *, size_t, uint64_t, uint8_t[16], uint8_t[16]);

/* CFLite function types. */
typedef CFStringRef (*fn_cfstr_create_t)(void *, const char *, uint32_t);
typedef CFIndex (*fn_cfdict_count_t)(CFDictionaryRef);
typedef void (*fn_cfdict_keys_t)(CFDictionaryRef, const void **, const void **);
typedef CFTypeRef (*fn_cfdict_get_t)(CFDictionaryRef, CFTypeRef);
typedef void (*fn_cfdict_set_t)(CFMutableDictionaryRef, CFTypeRef, CFTypeRef);
typedef void (*fn_cfdict_remove_t)(CFMutableDictionaryRef, CFTypeRef);
typedef int64_t (*fn_cfdict_get_i64_t)(CFDictionaryRef, CFStringRef, OSStatus *);
typedef void (*fn_cfdict_set_i64_t)(CFMutableDictionaryRef, CFStringRef, int64_t);
typedef CFMutableDictionaryRef (*fn_cfdict_new_t)(void *, CFIndex, const void *, const void *);
typedef CFIndex (*fn_cfarr_count_t)(CFArrayRef);
typedef CFTypeRef (*fn_cfarr_get_t)(CFArrayRef, CFIndex);
typedef void (*fn_cfarr_append_t)(CFMutableArrayRef, CFTypeRef);
typedef CFMutableArrayRef (*fn_cfarr_new_t)(void *, CFIndex, const void *);
typedef CFMutableArrayRef (*fn_cfarr_copy_t)(void *, CFIndex, CFArrayRef);
typedef const uint8_t *(*fn_cfdata_ptr_t)(CFDataRef);
typedef CFIndex (*fn_cfdata_len_t)(CFDataRef);
typedef CFTypeRef (*fn_cfretain_t)(CFTypeRef);
typedef void (*fn_cfrelease_t)(CFTypeRef);

static fn_setup_t g_setup_trampoline;
static fn_start_t g_start_trampoline;
static fn_teardown_t g_teardown_trampoline;
/*
 * Preserve exact stock entry points separately from executable trampolines.
 * This guarantees a true fail-closed fallback when an inline hook cannot be
 * installed: externally-preempted calls still delegate to stock instead of
 * dereferencing a NULL trampoline.
 */
static fn_setup_t g_real_setup;
static fn_start_t g_real_start;
static fn_teardown_t g_real_teardown;
static fn_serverinfo_t g_real_serverinfo;
static fn_sendcmd_t g_sendcmd;
static fn_aes_cbc_init_t g_real_aes_cbc_init;
static fn_aes_ctr_init_t g_aes_ctr_init;
static fn_aes_ctr_update_t g_aes_ctr_update;
static fn_aes_ctr_final_t g_aes_ctr_final;
static fn_derive_screen_t g_derive_screen;

static fn_cfstr_create_t p_CFStringCreateWithCString;
static fn_cfdict_count_t p_CFDictionaryGetCount;
static fn_cfdict_keys_t p_CFDictionaryGetKeysAndValues;
static fn_cfdict_get_t p_CFDictionaryGetValue;
static fn_cfdict_set_t p_CFDictionarySetValue;
static fn_cfdict_remove_t p_CFDictionaryRemoveValue;
static fn_cfdict_get_i64_t p_CFDictionaryGetInt64;
static fn_cfdict_set_i64_t p_CFDictionarySetInt64;
static fn_cfdict_new_t p_CFDictionaryCreateMutable;
static fn_cfarr_count_t p_CFArrayGetCount;
static fn_cfarr_get_t p_CFArrayGetValueAtIndex;
static fn_cfarr_append_t p_CFArrayAppendValue;
static fn_cfarr_new_t p_CFArrayCreateMutable;
static fn_cfarr_copy_t p_CFArrayCreateMutableCopy;
static fn_cfretain_t p_CFRetain;
static fn_cfrelease_t p_CFRelease;
static const void *p_dict_key_callbacks;
static const void *p_dict_val_callbacks;
static const void *p_array_callbacks;
static CFTypeRef p_cfl_boolean_false;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_heartbeat_lock = PTHREAD_MUTEX_INITIALIZER;
static AirPlayReceiverSessionRef g_active_session;
static uint8_t g_master_key[16];
static int g_master_valid;
static uintptr_t g_security_fn;

static int g_alt_listen = -1;
static int g_alt_client = -1;
static int g_alt_listener_ipv6;
static pthread_t g_alt_thread;
static int g_alt_thread_started;
static aes_ctr_storage_t g_alt_aes;
static int g_alt_aes_valid;

static int g_tee_listen = -1;
static int g_tee_client = -1;
static uint64_t g_tee_consumer_generation;
static pthread_t g_tee_thread;
static int g_tee_started;

static int g_enabled = 1;
static int g_alt_port = 6031;
static int g_tee_port = 19820;
static int g_width = 1010;
static int g_height = 376;
static int g_width_mm = 200;
static int g_height_mm = 74;
static int g_fps = 30;
static int g_auto_show = 0;
static int g_viewareas = 1;
static const char *g_viewareas_marker = "/mnt/app/root/mibr-carplay111-viewareas.enabled";
static const char *g_autoshow_disable_marker = "/mnt/app/root/mibr-carplay111-autoshow.disabled";
static const char *g_url_map_marker = "/mnt/app/root/mibr-carplay111-url-map.enabled";
static const char *g_bit26_on_marker = "/mnt/app/root/mibr-carplay111-bit26.force-on";
static const char *g_bit26_off_marker = "/mnt/app/root/mibr-carplay111-bit26.force-off";
static char g_alt_uuid[96] = ALT_UUID_DEFAULT;
static char g_alt_url[160] = ALT_URL_DEFAULT;
static const char *g_log_path = "/tmp/altscreen111.log";
static const char *g_state_path = "/tmp/mibr-carplay111.state";
static const char *g_heartbeat_path = "/tmp/mibr-carplay111.heartbeat";
static int g_streaming;
static int g_video_config_seen;
static uint64_t g_last_heartbeat_ms;

/* Gen-2 core: all access is serialized through g2_core_lock. */
static pthread_mutex_t g2_core_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g2_status_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g2_core_cv = PTHREAD_COND_INITIALIZER;
static struct alt111_profile g2_profile;
static struct alt111_control g2_control;
static struct alt111_video g2_video;
static uint64_t g2_control_session;
static uint64_t g2_video_stream;
static uint64_t g2_last_dispatched_request;
static uint64_t g2_last_completed_request;
static int g2_last_completion_status;
static unsigned g2_command_ready;
static pthread_t g2_control_thread;
static pthread_t g2_output_thread;
static int g2_workers_started;
static const char *g2_status_path = "/tmp/mibr-alt111-gen2.status";
static const char *g2_reacquire_marker = "/tmp/mibr-alt111-gen2-reacquire";

static void gen2_publish_status(void);
static void gen2_control_projection_on(void);
static void gen2_control_release(void);
static void gen2_set_command_ready(unsigned ready);
static void gen2_video_begin_current(void);
static void gen2_video_end_current(void);
static void gen2_close_consumer(void);
static void tee_drop_client_locked(void);

static void logf_u2(const char *fmt, ...)
{
    char buf[768];
    int fd;
    int n;
    va_list ap;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    n = snprintf(buf, sizeof(buf), "%ld.%03ld [altscreen111] ", (long)ts.tv_sec, ts.tv_nsec / 1000000L);
    if (n < 0) return;
    va_start(ap, fmt);
    n += vsnprintf(buf + n, (n < (int)sizeof(buf)) ? sizeof(buf) - (size_t)n : 0, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n >= sizeof(buf) - 2) n = (int)sizeof(buf) - 2;
    buf[n++] = '\n';
    fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) { (void)write(fd, buf, (size_t)n); close(fd); }
}

static uint64_t monotonic_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000L);
}

static void publish_state(const char *state)
{
    int fd;
    size_t n;
    if (!state) return;
    fd = open(g_state_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        n = strlen(state);
        (void)write(fd, state, n);
        (void)write(fd, "\n", 1);
        close(fd);
    }
}

static void publish_video_heartbeat_internal(int force)
{
    char b[48];
    int fd;
    int n;
    uint64_t now = monotonic_ms();

    pthread_mutex_lock(&g_heartbeat_lock);
    if (!force && now && g_last_heartbeat_ms && now - g_last_heartbeat_ms < 250u) {
        pthread_mutex_unlock(&g_heartbeat_lock);
        return;
    }
    if (force && now <= g_last_heartbeat_ms) now = g_last_heartbeat_ms + 1u;
    g_last_heartbeat_ms = now;
    n = snprintf(b, sizeof(b), "%llu\n", (unsigned long long)now);
    if (n > 0) {
        fd = open(g_heartbeat_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            (void)write(fd, b, (size_t)n);
            close(fd);
        }
    }
    pthread_mutex_unlock(&g_heartbeat_lock);
}

static void publish_video_heartbeat(void)
{
    publish_video_heartbeat_internal(0);
}

static void publish_video_heartbeat_force(void)
{
    publish_video_heartbeat_internal(1);
}

static void publish_video_heartbeat_source_arm(void)
{
    int need_arm;
    pthread_mutex_lock(&g_heartbeat_lock);
    need_arm = (g_last_heartbeat_ms == 0);
    pthread_mutex_unlock(&g_heartbeat_lock);
    if (need_arm) publish_video_heartbeat_force();
}

static void clear_video_heartbeat(void)
{
    pthread_mutex_lock(&g_heartbeat_lock);
    g_last_heartbeat_ms = 0;
    (void)unlink(g_heartbeat_path);
    pthread_mutex_unlock(&g_heartbeat_lock);
}

static void clear_video_observer(void)
{
    g_streaming = 0;
    g_video_config_seen = 0;
    clear_video_heartbeat();
}

static int env_i(const char *name, int defv)
{
    const char *s = getenv(name);
    return (s && *s) ? atoi(s) : defv;
}

static void env_s(const char *name, char *dst, size_t cap, const char *defv)
{
    const char *s = getenv(name);
    if (!s || !*s) s = defv;
    snprintf(dst, cap, "%s", s);
}

static CFStringRef s_cf(const char *s);

/*
 * Persistent A/B controls are intentionally file-based so the vehicle can
 * switch protocol variants without replacing/recompiling the injected binary.
 */
static const char *active_alt_url(void)
{
    return access(g_url_map_marker, F_OK) == 0 ? ALT_URL_MAP : g_alt_url;
}

/* 0 = preserve stock, +1 = force bit 26, -1 = clear bit 26. */
static int airplay_bit26_mode(void)
{
    if (access(g_bit26_on_marker, F_OK) == 0) return 1;
    if (access(g_bit26_off_marker, F_OK) == 0) return -1;
    return 0;
}

static void apply_airplay_bit26_ab(CFMutableDictionaryRef info)
{
    CFStringRef k;
    OSStatus err = 0;
    int mode;
    int64_t before, after;

    if (!info) return;
    mode = airplay_bit26_mode();
    if (!mode) return;

    k = s_cf("features");
    if (!k) return;
    before = p_CFDictionaryGetInt64(info, k, &err);
    if (err) {
        logf_u2("GEN2 A/B bit26 mode=%s root features unavailable; unchanged",
                mode > 0 ? "force-on" : "force-off");
        p_CFRelease(k);
        return;
    }

    after = (int64_t)(mode > 0
        ? ((uint64_t)before | AIRPLAY_FEATURE_BIT26)
        : ((uint64_t)before & ~AIRPLAY_FEATURE_BIT26));
    if (after != before) p_CFDictionarySetInt64(info, k, after);
    logf_u2("GEN2 A/B bit26 mode=%s features=0x%llx->0x%llx",
            mode > 0 ? "force-on" : "force-off",
            (unsigned long long)(uint64_t)before,
            (unsigned long long)(uint64_t)after);
    p_CFRelease(k);
}

static void *sym_next(const char *name)
{
    void *p = dlsym(RTLD_NEXT, name);
    if (!p) p = dlsym(RTLD_DEFAULT, name);
    return p;
}

static int init_api(void)
{
#define RESOLVE(dst, name) do { dst = (void *)sym_next(name); if (!(dst)) { logf_u2("missing symbol %s", name); return -1; } } while (0)
    RESOLVE(g_real_serverinfo, "AirPlayCopyServerInfo");
    RESOLVE(g_sendcmd, "AirPlayReceiverSessionSendCommand");
    RESOLVE(g_real_aes_cbc_init, "AES_CBCFrame_Init");
    RESOLVE(g_aes_ctr_init, "AES_CTR_Init");
    RESOLVE(g_aes_ctr_update, "AES_CTR_Update");
    RESOLVE(g_aes_ctr_final, "AES_CTR_Final");
    RESOLVE(g_derive_screen, "AirPlay_DeriveAESKeySHA512ForScreen");
    RESOLVE(p_CFStringCreateWithCString, "CFStringCreateWithCString");
    RESOLVE(p_CFDictionaryGetCount, "CFDictionaryGetCount");
    RESOLVE(p_CFDictionaryGetKeysAndValues, "CFDictionaryGetKeysAndValues");
    RESOLVE(p_CFDictionaryGetValue, "CFDictionaryGetValue");
    RESOLVE(p_CFDictionarySetValue, "CFDictionarySetValue");
    RESOLVE(p_CFDictionaryRemoveValue, "CFDictionaryRemoveValue");
    RESOLVE(p_CFDictionaryGetInt64, "CFDictionaryGetInt64");
    RESOLVE(p_CFDictionarySetInt64, "CFDictionarySetInt64");
    RESOLVE(p_CFDictionaryCreateMutable, "CFDictionaryCreateMutable");
    RESOLVE(p_CFArrayGetCount, "CFArrayGetCount");
    RESOLVE(p_CFArrayGetValueAtIndex, "CFArrayGetValueAtIndex");
    RESOLVE(p_CFArrayAppendValue, "CFArrayAppendValue");
    RESOLVE(p_CFArrayCreateMutable, "CFArrayCreateMutable");
    RESOLVE(p_CFArrayCreateMutableCopy, "CFArrayCreateMutableCopy");
    RESOLVE(p_CFRetain, "CFRetain");
    RESOLVE(p_CFRelease, "CFRelease");
    p_dict_key_callbacks = dlsym(RTLD_DEFAULT, "kCFLDictionaryKeyCallBacksCFLTypes");
    p_dict_val_callbacks = dlsym(RTLD_DEFAULT, "kCFLDictionaryValueCallBacksCFLTypes");
    p_array_callbacks = dlsym(RTLD_DEFAULT, "kCFLArrayCallBacksCFLTypes");
    {
        CFTypeRef *false_slot = (CFTypeRef *)dlsym(RTLD_DEFAULT, "kCFLBooleanFalse");
        p_cfl_boolean_false = false_slot ? *false_slot : NULL;
    }
    if (!p_dict_key_callbacks || !p_dict_val_callbacks || !p_array_callbacks ||
        !p_cfl_boolean_false) {
        logf_u2("missing CFLite callback tables/boolean singleton");
        return -1;
    }
    g_security_fn = (uintptr_t)sym_next("AirPlayReceiverSessionSetSecurityInfo");
    if (!g_security_fn) { logf_u2("missing AirPlayReceiverSessionSetSecurityInfo"); return -1; }
#undef RESOLVE
    return 0;
}

static CFStringRef s_cf(const char *s)
{
    return p_CFStringCreateWithCString(NULL, s, CF_UTF8);
}

static CFMutableDictionaryRef dict_new(void)
{
    return p_CFDictionaryCreateMutable(NULL, 0, p_dict_key_callbacks, p_dict_val_callbacks);
}

static CFMutableDictionaryRef dict_clone(CFDictionaryRef src)
{
    CFMutableDictionaryRef dst;
    CFIndex n, i;
    const void **keys, **vals;
    if (!src) return NULL;
    dst = dict_new();
    if (!dst) return NULL;
    n = p_CFDictionaryGetCount(src);
    if (n <= 0) return dst;
    keys = calloc((size_t)n, sizeof(*keys));
    vals = calloc((size_t)n, sizeof(*vals));
    if (!keys || !vals) { free(keys); free(vals); p_CFRelease(dst); return NULL; }
    p_CFDictionaryGetKeysAndValues(src, keys, vals);
    for (i = 0; i < n; ++i) p_CFDictionarySetValue(dst, keys[i], vals[i]);
    free(keys); free(vals);
    return dst;
}

static int stream_type(CFDictionaryRef d)
{
    OSStatus e = 0;
    CFStringRef k = s_cf("type");
    int64_t v = p_CFDictionaryGetInt64(d, k, &e);
    p_CFRelease(k);
    return e ? -1 : (int)v;
}

static uint64_t stream_connection_id(CFDictionaryRef d)
{
    OSStatus e = 0;
    CFStringRef k = s_cf("streamConnectionID");
    int64_t v = p_CFDictionaryGetInt64(d, k, &e);
    p_CFRelease(k);
    return e ? 0 : (uint64_t)v;
}

static CFArrayRef get_streams(CFDictionaryRef d)
{
    CFStringRef k;
    CFArrayRef a;
    if (!d) return NULL;
    k = s_cf("streams");
    a = (CFArrayRef)p_CFDictionaryGetValue(d, k);
    p_CFRelease(k);
    return a;
}

static int contains_stream_type(CFDictionaryRef request, int wanted)
{
    CFArrayRef a = get_streams(request);
    CFIndex i, n;
    if (!a) return 0;
    n = p_CFArrayGetCount(a);
    for (i = 0; i < n; ++i) {
        CFDictionaryRef sd = (CFDictionaryRef)p_CFArrayGetValueAtIndex(a, i);
        if (stream_type(sd) == wanted) return 1;
    }
    return 0;
}

static int contains_stream111(CFDictionaryRef request, CFDictionaryRef *outDesc, int *outOtherCount)
{
    CFArrayRef a = get_streams(request);
    CFIndex i, n;
    int found = 0, other = 0;
    if (outDesc) *outDesc = NULL;
    if (!a) { if (outOtherCount) *outOtherCount = 0; return 0; }
    n = p_CFArrayGetCount(a);
    for (i = 0; i < n; ++i) {
        CFDictionaryRef sd = (CFDictionaryRef)p_CFArrayGetValueAtIndex(a, i);
        if (stream_type(sd) == K_STREAM_ALT) {
            found = 1;
            if (outDesc && !*outDesc) *outDesc = sd;
        } else ++other;
    }
    if (outOtherCount) *outOtherCount = other;
    return found;
}

static CFMutableDictionaryRef clone_without_111(CFDictionaryRef request)
{
    CFMutableDictionaryRef d = dict_clone(request);
    CFArrayRef a = get_streams(request);
    CFMutableArrayRef b;
    CFStringRef k;
    CFIndex i, n;
    if (!d || !a) return d;
    b = p_CFArrayCreateMutable(NULL, 0, p_array_callbacks);
    if (!b) return d;
    n = p_CFArrayGetCount(a);
    for (i = 0; i < n; ++i) {
        CFDictionaryRef sd = (CFDictionaryRef)p_CFArrayGetValueAtIndex(a, i);
        if (stream_type(sd) != K_STREAM_ALT) p_CFArrayAppendValue(b, sd);
    }
    k = s_cf("streams");
    p_CFDictionarySetValue(d, k, b);
    p_CFRelease(k);
    p_CFRelease(b);
    return d;
}

/*
 * Recovered MHI2Q IRC behavior: every successful /info or SETUP response
 * advertises the AltScreen capability at the root before the peer has to
 * choose stream 111. The reference replaces enabledFeatures with
 * ["altScreen","viewAreas"] rather than waiting for a 111 request.
 */
static void set_reference_enabled_features(CFMutableDictionaryRef response)
{
    CFStringRef k = NULL, alt = NULL, va = NULL;
    CFMutableArrayRef a = NULL;
    if(!response) return;

    k = s_cf("enabledFeatures");
    alt = s_cf("altScreen");
    va = s_cf("viewAreas");
    a = p_CFArrayCreateMutable(NULL,0,p_array_callbacks);
    if(a && alt) p_CFArrayAppendValue(a,alt);
    if(a && va && g_viewareas) p_CFArrayAppendValue(a,va);
    if(a && k) p_CFDictionarySetValue(response,k,a);

    if(a) p_CFRelease(a);
    if(va) p_CFRelease(va);
    if(alt) p_CFRelease(alt);
    if(k) p_CFRelease(k);
}

/* Clone the exact requested 111 descriptor and preserve unknown peer fields. */
static void append_alt_setup_response(CFMutableDictionaryRef response,
                                      CFDictionaryRef requested,
                                      int data_port)
{
    CFStringRef kstreams = NULL, kport = NULL, kstreamid = NULL;
    CFArrayRef old = NULL;
    CFMutableArrayRef a = NULL;
    CFMutableDictionaryRef sd = NULL;

    if(!response || !requested) return;
    kstreams = s_cf("streams");
    old = (CFArrayRef)p_CFDictionaryGetValue(response,kstreams);
    a = old ? p_CFArrayCreateMutableCopy(NULL,0,old)
            : p_CFArrayCreateMutable(NULL,0,p_array_callbacks);
    sd = dict_clone(requested);
    kport = s_cf("dataPort");
    kstreamid = s_cf("streamID");

    if(a && sd && kport && kstreamid) {
        p_CFDictionarySetInt64(sd,kport,data_port);
        p_CFDictionarySetInt64(sd,kstreamid,K_STREAM_ALT);
        p_CFArrayAppendValue(a,sd);
        p_CFDictionarySetValue(response,kstreams,a);
    }

    if(sd) p_CFRelease(sd);
    if(a) p_CFRelease(a);
    if(kstreamid) p_CFRelease(kstreamid);
    if(kport) p_CFRelease(kport);
    if(kstreams) p_CFRelease(kstreams);
}

/*
 * Two deliberately different network boundaries:
 *  - stream 111 is inbound from the iPhone and therefore must be reachable on
 *    the CarPlay link (INADDR_ANY / actual head-unit address);
 *  - the decoded H.264 tee is local-only and must never leave loopback.
 *
 * Only the iPhone-facing listener may fall back to an ephemeral port because
 * its actual dataPort is returned in SETUP. The renderer tee is configured by
 * URL and must fail closed if its fixed port is occupied.
 */
static int bind_listener_ipv4(int preferred, int *out_port, int loopback, int allow_ephemeral)
{
    int fd, one = 1;
    struct sockaddr_in sa;
    socklen_t sl = sizeof(sa);
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) return -1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(loopback ? INADDR_LOOPBACK : INADDR_ANY);
    sa.sin_port = htons((uint16_t)preferred);
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        if (!allow_ephemeral) { close(fd); return -1; }
        sa.sin_port = 0;
        if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) { close(fd); return -1; }
    }
    if (listen(fd, 2) < 0) { close(fd); return -1; }
    if (getsockname(fd, (struct sockaddr *)&sa, &sl) == 0 && out_port) *out_port = ntohs(sa.sin_port);
    return fd;
}

static int bind_listener_stream111(int preferred, int *out_port, int allow_ephemeral)
{
#ifdef AF_INET6
    int fd,one=1,off=0;
    struct sockaddr_in6 sa6;
    socklen_t sl6=sizeof(sa6);
    fd=socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
    if(fd>=0){
        setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
#ifdef IPV6_V6ONLY
        (void)setsockopt(fd,IPPROTO_IPV6,IPV6_V6ONLY,&off,sizeof(off));
#endif
        memset(&sa6,0,sizeof(sa6));
        sa6.sin6_family=AF_INET6;
        sa6.sin6_port=htons((uint16_t)preferred);
        if(bind(fd,(struct sockaddr *)&sa6,sizeof(sa6))<0){
            if(allow_ephemeral){
                sa6.sin6_port=0;
                if(bind(fd,(struct sockaddr *)&sa6,sizeof(sa6))<0){close(fd);fd=-1;}
            }else{
                close(fd);
                fd=-1;
            }
        }
        if(fd>=0){
            if(listen(fd,1)==0){
                if(getsockname(fd,(struct sockaddr *)&sa6,&sl6)==0 && out_port)*out_port=ntohs(sa6.sin6_port);
                g_alt_listener_ipv6=1;
                return fd;
            }
            close(fd);
        }
    }
#endif
    g_alt_listener_ipv6=0;
    return bind_listener_ipv4(preferred,out_port,0,allow_ephemeral);
}

static CFMutableDictionaryRef command_force_keyframe(void);

static void set_active_session(AirPlayReceiverSessionRef s)
{
    AirPlayReceiverSessionRef old = NULL, keep = NULL;

    if (s) keep = (AirPlayReceiverSessionRef)p_CFRetain(s);
    pthread_mutex_lock(&g_lock);
    if (g_active_session == s) {
        pthread_mutex_unlock(&g_lock);
        if (keep) p_CFRelease(keep);
        return;
    }
    old = g_active_session;
    g_active_session = keep;
    pthread_mutex_unlock(&g_lock);
    if (old) p_CFRelease(old);
}

static AirPlayReceiverSessionRef retain_active_session(void)
{
    AirPlayReceiverSessionRef s = NULL;
    pthread_mutex_lock(&g_lock);
    if (g_active_session) s = (AirPlayReceiverSessionRef)p_CFRetain(g_active_session);
    pthread_mutex_unlock(&g_lock);
    return s;
}

/* ---------------- Gen-2 video adapter ---------------- */

static void gen2_close_consumer(void)
{
    pthread_mutex_lock(&g_lock);
    tee_drop_client_locked();
    pthread_mutex_unlock(&g_lock);

    pthread_mutex_lock(&g2_core_lock);
    alt111_video_detach(&g2_video);
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);
}

static void gen2_video_begin_current(void)
{
    uint64_t session;
    gen2_close_consumer();
    pthread_mutex_lock(&g2_core_lock);
    session = g2_control_session ? g2_control_session : (g2_control.session + 1u);
    g2_video_stream = alt111_video_begin(&g2_video, session);
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);
    gen2_publish_status();
}

static void gen2_video_end_current(void)
{
    pthread_mutex_lock(&g2_core_lock);
    if (g2_video_stream) (void)alt111_video_end(&g2_video, g2_video_stream);
    g2_video_stream = 0;
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);

    pthread_mutex_lock(&g_lock);
    tee_drop_client_locked();
    pthread_mutex_unlock(&g_lock);
    gen2_publish_status();
}

static int gen2_video_config(const uint8_t *p, size_t n)
{
    int rc;
    pthread_mutex_lock(&g2_core_lock);
    rc = g2_video_stream ? alt111_video_config(&g2_video, g2_video_stream, p, n) : ALT111_STALE;
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);
    if (rc == ALT111_RESTART_CONSUMER || rc < 0) {
        logf_u2("gen2 VideoConfig rc=%d -> consumer reset", rc);
        gen2_close_consumer();
    }
    gen2_publish_status();
    return rc;
}

static int gen2_video_submit_au(const uint8_t *p, size_t n, int *accepted, int *consumer_attached)
{
    int rc;
    uint64_t before, after_count;
    if (accepted) *accepted = 0;
    if (consumer_attached) *consumer_attached = 0;
    pthread_mutex_lock(&g2_core_lock);
    before = g2_video.source_aus;
    rc = g2_video_stream ? alt111_video_submit(&g2_video, g2_video_stream, p, n, 1) : ALT111_STALE;
    after_count = g2_video.source_aus;
    if (accepted && after_count > before) *accepted = 1;
    if (consumer_attached) *consumer_attached = g2_video.attached ? 1 : 0;
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);
    if (rc == ALT111_RESTART_CONSUMER || rc < 0) {
        logf_u2("gen2 video AU rc=%d body=%zu -> consumer reset", rc, n);
        gen2_close_consumer();
    }
    return rc;
}

static void gen2_keyframe_intent(void)
{
    int rc = ALT111_WAIT;
    pthread_mutex_lock(&g2_core_lock);
    if (g2_control_session)
        rc = alt111_control_keyframe(&g2_control, g2_control_session);
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);
    logf_u2("gen2 consumer keyframe intent rc=%d", rc);
}

static void gen2_consumer_attach(void)
{
    int rc;
    uint64_t generation = 0;
    pthread_mutex_lock(&g2_core_lock);
    if (g2_video.attached) alt111_video_detach(&g2_video);
    rc = alt111_video_attach(&g2_video);
    if (rc == ALT111_OK) generation = g2_video.consumer;
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);

    if (rc == ALT111_OK) {
        pthread_mutex_lock(&g_lock);
        if (g_tee_client >= 0) g_tee_consumer_generation = generation;
        pthread_mutex_unlock(&g_lock);
    }
    logf_u2("gen2 renderer consumer attach rc=%d generation=%llu",
            rc,(unsigned long long)generation);
    if (rc == ALT111_OK) gen2_keyframe_intent();
    gen2_publish_status();
}

static void *gen2_output_worker(void *arg)
{
    uint8_t *copy = NULL;
    size_t cap = 0;
    (void)arg;

    for (;;) {
        const uint8_t *p = NULL;
        size_t n = 0, chunk = 0;
        struct alt111_output_ticket ticket;
        int prc, fd, send_failed = 0, would_block = 0, stale_socket = 0;
        int arc = ALT111_WAIT, primed_before = 0, primed_after = 0;
        uint64_t delivered_before = 0, delivered_after = 0;
        ssize_t sent = 0;

        /*
         * Keep the core generation stable from the final peek through the
         * nonblocking socket write and advance. This closes the last race where
         * a codec/consumer generation could be invalidated after bytes were
         * copied but before they were sent.
         *
         * g_lock is nested only inside g2_core_lock here. All other adapter
         * paths release g_lock before acquiring g2_core_lock, so there is no
         * reverse nested order.
         */
        pthread_mutex_lock(&g2_core_lock);
        prc = alt111_video_peek(&g2_video, &p, &n, &ticket);
        if (prc != ALT111_OK || !n) {
            pthread_mutex_unlock(&g2_core_lock);
            usleep(5000);
            continue;
        }

        chunk = n > 65536u ? 65536u : n;
        if (cap < chunk) {
            uint8_t *next = (uint8_t *)realloc(copy, chunk);
            if (!next) {
                pthread_mutex_unlock(&g2_core_lock);
                usleep(10000);
                continue;
            }
            copy = next;
            cap = chunk;
        }
        memcpy(copy, p, chunk);

        delivered_before = g2_video.delivered_aus;
        primed_before = g2_video.consumer_primed;

        pthread_mutex_lock(&g_lock);
        fd = g_tee_client;
        if (fd >= 0 && g_tee_consumer_generation != ticket.consumer) {
            stale_socket = 1;
        } else if (fd >= 0) {
            do {
                sent = send(fd, copy, chunk, MSG_NOSIGNAL);
            } while (sent < 0 && errno == EINTR);

            if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                would_block = 1;
            } else if (sent <= 0) {
                tee_drop_client_locked();
                send_failed = 1;
            }
        } else {
            send_failed = 1;
        }
        pthread_mutex_unlock(&g_lock);

        if (stale_socket) {
            pthread_mutex_unlock(&g2_core_lock);
            usleep(1000);
            continue;
        }

        if (would_block) {
            pthread_mutex_unlock(&g2_core_lock);
            usleep(5000);
            continue;
        }

        if (send_failed) {
            alt111_video_detach(&g2_video);
            pthread_cond_broadcast(&g2_core_cv);
            pthread_mutex_unlock(&g2_core_lock);
            gen2_publish_status();
            usleep(10000);
            continue;
        }

        arc = alt111_video_advance(&g2_video, &ticket, (size_t)sent);
        delivered_after = g2_video.delivered_aus;
        primed_after = g2_video.consumer_primed;
        pthread_mutex_unlock(&g2_core_lock);

        if (arc == ALT111_OK && delivered_after > delivered_before) {
            /*
             * Before a consumer attaches the source heartbeat is enough to let
             * Auto-Direct start the bridge. Once attached, only actual complete
             * AU delivery advances the heartbeat, so DIRECT cannot be declared
             * ready merely on non-IDR source traffic.
             *
             * The first priming AU bypasses the 250 ms throttle: a static map
             * may emit no second frame.
             */
            if (!primed_before && primed_after)
                publish_video_heartbeat_force();
            else
                publish_video_heartbeat();

            if (!primed_before || primed_after != primed_before ||
                (delivered_after & 7u) == 0u)
                gen2_publish_status();
        }
    }
    return NULL;
}

static int read_exact(int fd, void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;
    while (len) {
        ssize_t n = recv(fd, p, len, 0);
        if (n == 0) return 0;
        if (n < 0) { if (errno == EINTR) continue; return -1; }
        p += n; len -= (size_t)n;
    }
    return 1;
}

static void tee_drop_client_locked(void)
{
    if (g_tee_client >= 0) close(g_tee_client);
    g_tee_client = -1;
    g_tee_consumer_generation = 0;
}

static void *tee_accept_thread(void *arg)
{
    (void)arg;
    for (;;) {
        int c = accept(g_tee_listen, NULL, NULL);
        if (c < 0) { if (errno == EINTR) continue; sleep(1); continue; }
        {
            int flags = fcntl(c, F_GETFL, 0);
            if (flags >= 0) (void)fcntl(c, F_SETFL, flags | O_NONBLOCK);
        }
        pthread_mutex_lock(&g_lock);
        tee_drop_client_locked();
        g_tee_client = c;
        pthread_mutex_unlock(&g_lock);
        logf_u2("gen2 renderer connected nonblocking on 127.0.0.1:%d; waiting for config+complete IDR", g_tee_port);
        gen2_consumer_attach();
    }
    return NULL;
}

static int start_tee_server(void)
{
    int actual = 0;
    if (g_tee_started) return 0;
    g_tee_listen = bind_listener_ipv4(g_tee_port, &actual, 1, 0);
    if (g_tee_listen < 0) { logf_u2("cannot bind renderer tee port %d: %s", g_tee_port, strerror(errno)); return -1; }
    g_tee_port = actual;
    if (pthread_create(&g_tee_thread, NULL, tee_accept_thread, NULL) != 0) { close(g_tee_listen); g_tee_listen=-1; return -1; }
    pthread_detach(g_tee_thread);
    g_tee_started = 1;
    logf_u2("renderer tee listening on 127.0.0.1:%d", g_tee_port);
    return 0;
}

/*
 * Stop and join the previous stream-111 worker before its global AES/session
 * state can be reused. The worker owns the accepted client fd; stop only
 * shutdowns that fd to wake read_exact(), then joins the worker. This prevents
 * a late cleanup from an old CarPlay session finalising a newly-created AES
 * context after a fast reconnect.
 */
static void stop_alt_receiver(void)
{
    pthread_t t;
    int do_join = 0;

    pthread_mutex_lock(&g_lock);
    if (g_alt_client >= 0) (void)shutdown(g_alt_client, SHUT_RDWR);
    if (g_alt_listen >= 0) {
        (void)shutdown(g_alt_listen, SHUT_RDWR);
        close(g_alt_listen);
        g_alt_listen = -1;
    }
    if (g_alt_thread_started && !pthread_equal(pthread_self(), g_alt_thread)) {
        t = g_alt_thread;
        g_alt_thread_started = 0;
        do_join = 1;
    }
    pthread_mutex_unlock(&g_lock);

    if (do_join) (void)pthread_join(t, NULL);

    pthread_mutex_lock(&g_lock);
    if (!do_join && g_alt_client >= 0) {
        /* Constructor/failure fallback: no worker owns this descriptor. */
        close(g_alt_client);
        g_alt_client = -1;
    }
    if (g_alt_aes_valid) {
        g_aes_ctr_final(&g_alt_aes);
        g_alt_aes_valid = 0;
    }
    pthread_mutex_unlock(&g_lock);
    gen2_video_end_current();
}

static void *alt_receiver_thread(void *arg)
{
    int listen_fd = (int)(intptr_t)arg;
    int c;
    logf_u2("waiting for iPhone AltScreen connection on 0.0.0.0:%d", g_alt_port);
    c = accept(listen_fd, NULL, NULL);
    if (c < 0) { logf_u2("AltScreen accept failed: %s", strerror(errno)); return NULL; }
    pthread_mutex_lock(&g_lock); g_alt_client = c; pthread_mutex_unlock(&g_lock);
    logf_u2("AltScreen stream 111 connected");
    publish_state("connected");
    for (;;) {
        AirPlayScreenHeaderCompat h;
        uint8_t *body = NULL;
        int rr = read_exact(c, &h, sizeof(h));
        if (rr <= 0) break;
        if (h.bodySize > MAX_SCREEN_BODY) { logf_u2("reject bodySize=%u", h.bodySize); break; }
        if (h.bodySize) {
            body = malloc(h.bodySize);
            if (!body) break;
            rr = read_exact(c, body, h.bodySize);
            if (rr <= 0) { free(body); break; }
        }
        switch (h.opcode) {
            case K_SCREEN_VIDEO_CONFIG:
                if (body && h.bodySize) {
                    float source_w = 0.0f, source_h = 0.0f;
                    memcpy(&source_w, h.params + 8, sizeof(source_w));
                    memcpy(&source_h, h.params + 12, sizeof(source_h));
                    logf_u2("VideoConfig header source=%.1fx%.1f flags=0x%02x body=%u",
                            (double)source_w, (double)source_h,
                            (unsigned)h.smallParam[1], (unsigned)h.bodySize);
                    {
                        int vrc = gen2_video_config(body, h.bodySize);
                        if (vrc == ALT111_OK || vrc == ALT111_RESTART_CONSUMER) {
                            if (!g_video_config_seen) {
                                g_video_config_seen = 1;
                                publish_state("video_config");
                                logf_u2("gen2: stream111 valid transactional video config received");
                            }
                        } else if (vrc == ALT111_INVALID) {
                            g_video_config_seen = 0;
                            g_streaming = 0;
                            clear_video_heartbeat();
                            publish_state("video_config_invalid");
                            logf_u2("gen2: invalid VideoConfig revoked Stream111 readiness");
                        }
                    }
                }
                break;
            case K_SCREEN_VIDEO_FRAME:
                if (body && h.bodySize) {
                    if (!g_alt_aes_valid || g_aes_ctr_update(&g_alt_aes, body, h.bodySize, body) != K_NO_ERR) {
                        logf_u2("AES-CTR decrypt failed"); free(body); goto done;
                    }
                    if (!g_video_config_seen) {
                        logf_u2("video frame ignored before valid avcC config body=%u", (unsigned)h.bodySize);
                        break;
                    }
                    {
                        int accepted = 0, consumer_attached = 0;
                        int vrc = gen2_video_submit_au(body, h.bodySize, &accepted, &consumer_attached);
                        if (accepted) {
                            if (!g_streaming) {
                                g_streaming = 1;
                                publish_state("streaming");
                                logf_u2("gen2: first complete Screen VIDEO_FRAME accepted as AU");
                            }
                            /* Source activity only creates the initial arm
                             * heartbeat. It never advances that heartbeat while
                             * no local consumer exists; otherwise Auto-Direct
                             * could mistake pre-connect source traffic for
                             * post-connect decoder-ready output. */
                            if (!consumer_attached) publish_video_heartbeat_source_arm();
                        }
                        if (vrc == ALT111_INVALID) {
                            g_streaming = 0;
                            clear_video_heartbeat();
                            publish_state("video_invalid");
                            logf_u2("gen2: rejected VIDEO_FRAME body=%u as invalid complete AU; source readiness revoked",
                                    (unsigned)h.bodySize);
                        }
                    }
                }
                break;
            case K_SCREEN_KEEPALIVE:
            case K_SCREEN_KEEPALIVE_BODY:
            case K_SCREEN_IGNORE:
            case K_SCREEN_FORCE_KEYFRAME:
                break;
            default:
                logf_u2("unknown screen opcode=%u", h.opcode);
                break;
        }
        free(body);
    }
done:
    logf_u2("AltScreen stream 111 disconnected");
    clear_video_observer();
    publish_state("disconnected");
    pthread_mutex_lock(&g_lock);
    if (g_alt_client == c) g_alt_client = -1;
    close(c);
    if (g_alt_listen == listen_fd) {
        close(g_alt_listen);
        g_alt_listen = -1;
    }
    if (g_alt_aes_valid) { g_aes_ctr_final(&g_alt_aes); g_alt_aes_valid=0; }
    pthread_mutex_unlock(&g_lock);
    gen2_video_end_current();
    return NULL;
}

static int start_alt_receiver(uint64_t connection_id)
{
    uint8_t key[16], iv[16];
    int actual = 0;
    int allow_ephemeral = env_i("ALTSCREEN111_ALLOW_EPHEMERAL", 0);

    /* Complete previous worker teardown before reusing global session state. */
    stop_alt_receiver();
    clear_video_observer();

    /* Start the core generation before taking g_lock: gen2_video_begin_current()
     * deliberately closes/reset the local consumer and therefore takes g_lock. */
    gen2_video_begin_current();

    pthread_mutex_lock(&g_lock);
    if (!g_master_valid) {
        pthread_mutex_unlock(&g_lock);
        gen2_video_end_current();
        logf_u2("no captured session AES key; cannot start stream111");
        return -1;
    }

    g_derive_screen(g_master_key, 16, connection_id, key, iv);
    memset(&g_alt_aes, 0, sizeof(g_alt_aes));
    if (g_aes_ctr_init(&g_alt_aes, key, iv) != K_NO_ERR) {
        memset(key,0,sizeof(key)); memset(iv,0,sizeof(iv));
        pthread_mutex_unlock(&g_lock);
        gen2_video_end_current();
        return -1;
    }
    memset(key,0,sizeof(key)); memset(iv,0,sizeof(iv));
    g_alt_aes_valid = 1;

    /* iPhone-facing listener: reachable from CarPlay link, not loopback. */
    g_alt_listen = bind_listener_stream111(g_alt_port, &actual, allow_ephemeral);
    if (g_alt_listen < 0) {
        g_aes_ctr_final(&g_alt_aes);
        g_alt_aes_valid=0;
        pthread_mutex_unlock(&g_lock);
        gen2_video_end_current();
        logf_u2("cannot bind iPhone stream111 listener on port %d: %s", g_alt_port, strerror(errno));
        return -1;
    }
    g_alt_port = actual;
    if (pthread_create(&g_alt_thread, NULL, alt_receiver_thread, (void *)(intptr_t)g_alt_listen) != 0) {
        close(g_alt_listen);
        g_alt_listen=-1;
        g_aes_ctr_final(&g_alt_aes);
        g_alt_aes_valid=0;
        pthread_mutex_unlock(&g_lock);
        gen2_video_end_current();
        return -1;
    }
    g_alt_thread_started = 1;
    pthread_mutex_unlock(&g_lock);
    publish_state("listening");
    logf_u2("stream111 receiver ready: conn=%llu dataPort=%d transport=%s",
            (unsigned long long)connection_id,g_alt_port,
            g_alt_listener_ipv6?"IPv6-dualstack":"IPv4");
    return g_alt_port;
}

static void set_str(CFMutableDictionaryRef d, const char *key, const char *val)
{
    CFStringRef k=s_cf(key), v=s_cf(val); p_CFDictionarySetValue(d,k,v); p_CFRelease(v); p_CFRelease(k);
}

static void set_i64(CFMutableDictionaryRef d, const char *key, int64_t val)
{
    CFStringRef k=s_cf(key); p_CFDictionarySetInt64(d,k,val); p_CFRelease(k);
}

static void set_false(CFMutableDictionaryRef d, const char *key)
{
    CFStringRef k;
    if(!d || !key || !p_cfl_boolean_false) return;
    k=s_cf(key);
    if(k){p_CFDictionarySetValue(d,k,p_cfl_boolean_false);p_CFRelease(k);}
}

static void remove_key(CFMutableDictionaryRef d, const char *name)
{
    CFStringRef k;
    if(!d || !name) return;
    k=s_cf(name);
    if(k){p_CFDictionaryRemoveValue(d,k);p_CFRelease(k);}
}

/*
 * Keep the recovered reference ViewArea/SafeArea structure, but do not import
 * the Audi-specific 420x330 safe window into the 1010x376 Skoda VC baseline.
 *
 * Vehicle-PoC policy: initially expose the complete secondary-display canvas
 * as both ViewArea and SafeArea. This lets iOS/the navigation app decide what
 * cluster UI it can render without us prematurely constraining overlays to a
 * narrow center strip. A smaller/tube-specific SafeArea can be added later
 * once the full-width and classic VC layouts have been measured on-car.
 */
static void add_reference_viewarea(CFMutableDictionaryRef alt)
{
    CFMutableDictionaryRef view=NULL,safe=NULL;
    CFMutableArrayRef areas=NULL;
    CFStringRef k=NULL;
    int safe_w,safe_h,safe_x,safe_y;

    if(!alt || g_width<=0 || g_height<=0) return;
    safe_w=g_width;
    safe_h=g_height;
    safe_x=0;
    safe_y=0;

    view=dict_new();
    safe=dict_new();
    areas=p_CFArrayCreateMutable(NULL,0,p_array_callbacks);
    if(!view||!safe||!areas) goto done;

    set_i64(view,"widthPixels",g_width);
    set_i64(view,"heightPixels",g_height);
    set_i64(view,"originXPixels",0);
    set_i64(view,"originYPixels",0);

    set_i64(safe,"widthPixels",safe_w);
    set_i64(safe,"heightPixels",safe_h);
    set_i64(safe,"originXPixels",safe_x);
    set_i64(safe,"originYPixels",safe_y);

    /*
     * These are ViewArea policy booleans, not numeric zero values. MIBSI
     * serializes both as real CFBoolean false objects.
     */
    set_false(view,"drawUIOutsideSafeArea");
    set_false(view,"viewAreaTransitionControl");

    k=s_cf("safeArea");
    p_CFDictionarySetValue(view,k,safe);
    p_CFRelease(k); k=NULL;

    p_CFArrayAppendValue(areas,view);
    k=s_cf("viewAreas");
    p_CFDictionarySetValue(alt,k,areas);
    p_CFRelease(k); k=NULL;
    set_i64(alt,"initialViewArea",0);

done:
    if(k)p_CFRelease(k);
    if(areas)p_CFRelease(areas);
    if(safe)p_CFRelease(safe);
    if(view)p_CFRelease(view);
}

static void log_stream_types(const char *tag, CFDictionaryRef request)
{
    CFArrayRef a=get_streams(request);
    CFIndex i,n;
    char b[256];
    int used=0;
    if(!a){logf_u2("%s streams=<none>",tag);return;}
    n=p_CFArrayGetCount(a);
    used=snprintf(b,sizeof(b),"%s streams=%ld types=",tag,(long)n);
    for(i=0;i<n && i<16 && used>0 && used<(int)sizeof(b)-16;++i)
        used+=snprintf(b+used,sizeof(b)-(size_t)used,"%s%d",(i?",":""),stream_type((CFDictionaryRef)p_CFArrayGetValueAtIndex(a,i)));
    logf_u2("%s",b);
}

CFDictionaryRef AirPlayCopyServerInfo(AirPlayReceiverSessionRef session, CFArrayRef properties, uint8_t *mac, OSStatus *outErr)
{
    CFDictionaryRef base,stock_display;
    CFMutableDictionaryRef info=NULL,alt=NULL;
    CFStringRef kdisplays=NULL;
    CFArrayRef old=NULL;
    CFMutableArrayRef displays=NULL;

    if(!g_real_serverinfo)g_real_serverinfo=(fn_serverinfo_t)sym_next("AirPlayCopyServerInfo");
    if(!g_real_serverinfo){
        logf_u2("AirPlayCopyServerInfo stock delegate unavailable");
        if(outErr)*outErr=-1;
        return NULL;
    }
    base=g_real_serverinfo(session,properties,mac,outErr);
    if(!g_enabled||!base)return base;

    info=dict_clone(base);
    if(!info)return base;

    /* Reference order: root capability first, display transformation second. */
    apply_airplay_bit26_ab(info);
    set_reference_enabled_features(info);

    kdisplays=s_cf("displays");
    old=(CFArrayRef)p_CFDictionaryGetValue(base,kdisplays);
    if(!old || p_CFArrayGetCount(old)<=0){
        logf_u2("IRC-parity /info: root features added but stock displays missing");
        p_CFRelease(kdisplays);
        p_CFRelease(base);
        return info;
    }

    stock_display=(CFDictionaryRef)p_CFArrayGetValueAtIndex(old,0);
    {
        OSStatus fe=0, ie=0;
        CFStringRef fk=s_cf("features"), ik=s_cf("primaryInputDevice");
        int64_t fv=p_CFDictionaryGetInt64(stock_display,fk,&fe);
        int64_t iv=p_CFDictionaryGetInt64(stock_display,ik,&ie);
        logf_u2("GEN2 stock display capability baseline: features=%s%lld primaryInputDevice=%s%lld; alt clone removes primaryInputDevice",
                fe?"<absent>":"",(long long)(fe?0:fv),
                ie?"<absent>":"",(long long)(ie?0:iv));
        p_CFRelease(ik); p_CFRelease(fk);
    }
    displays=p_CFArrayCreateMutableCopy(NULL,0,old);
    alt=dict_clone(stock_display);
    if(displays&&alt){
        /* Exact reference removal set before AltScreen-specific overrides. */
        remove_key(alt,"primaryInputDevice");
        remove_key(alt,"edid");
        remove_key(alt,"platformLayer");
        remove_key(alt,"avcc");
        remove_key(alt,"xOffset");
        remove_key(alt,"yOffset");
        remove_key(alt,"windowWidth");
        remove_key(alt,"windowHeight");

        /*
         * Coherent GEN2 no-HID profile. Do not inherit main-display touch
         * capabilities or cadence. The validated profile and MIBSI parity
         * evidence both identify this secondary display explicitly as type 111.
         */
        set_i64(alt,"type",(int64_t)g2_profile.type);
        set_i64(alt,"maxFPS",(int64_t)g2_profile.max_fps);
        set_i64(alt,"features",(int64_t)g2_profile.features);
        set_i64(alt,"widthPixels",(int64_t)g2_profile.width);
        set_i64(alt,"heightPixels",(int64_t)g2_profile.height);
        set_i64(alt,"widthPhysical",(int64_t)g2_profile.width_mm);
        set_i64(alt,"heightPhysical",(int64_t)g2_profile.height_mm);
        set_str(alt,"uuid",g_alt_uuid);
        set_str(alt,"initialURL",active_alt_url());
        if(g_viewareas)add_reference_viewarea(alt);

        p_CFArrayAppendValue(displays,alt);
        p_CFDictionarySetValue(info,kdisplays,displays);
        logf_u2("GEN2 /info ready: root=altScreen%s type=%u maxFPS=%u features=%u input=none geometry=%ux%u physical=%ux%u uuid=%s url=%s",
                g_viewareas?"+viewAreas":"",
                g2_profile.type,g2_profile.max_fps,g2_profile.features,
                g2_profile.width,g2_profile.height,g2_profile.width_mm,g2_profile.height_mm,
                g_alt_uuid,active_alt_url());
    }else{
        logf_u2("IRC-parity /info: display clone failed");
    }

    if(alt)p_CFRelease(alt);
    if(displays)p_CFRelease(displays);
    p_CFRelease(kdisplays);
    p_CFRelease(base);
    return info;
}

static CFMutableDictionaryRef command_showui(void)
{
    CFMutableDictionaryRef req=dict_new(), params=dict_new();
    if(!req||!params){if(req)p_CFRelease(req);if(params)p_CFRelease(params);return NULL;}
    set_str(req,"type","showUI"); set_str(params,"uuid",g_alt_uuid); set_str(params,"url",active_alt_url());
    { CFStringRef k=s_cf("params"); p_CFDictionarySetValue(req,k,params); p_CFRelease(k); }
    p_CFRelease(params); return req;
}

static CFMutableDictionaryRef command_force_keyframe(void)
{
    CFMutableDictionaryRef req=dict_new(), params=dict_new();
    if(!req||!params){if(req)p_CFRelease(req);if(params)p_CFRelease(params);return NULL;}
    set_str(req,"type","forceKeyFrame"); set_str(params,"uuid",g_alt_uuid);
    { CFStringRef k=s_cf("params"); p_CFDictionarySetValue(req,k,params); p_CFRelease(k); }
    p_CFRelease(params); return req;
}

static CFMutableDictionaryRef command_stopui(void)
{
    CFMutableDictionaryRef req=dict_new(), params=dict_new();
    if(!req||!params){if(req)p_CFRelease(req);if(params)p_CFRelease(params);return NULL;}
    set_str(req,"type","stopUI"); set_str(params,"uuid",g_alt_uuid);
    { CFStringRef k=s_cf("params"); p_CFDictionarySetValue(req,k,params); p_CFRelease(k); }
    p_CFRelease(params); return req;
}

static CFMutableDictionaryRef command_update_view(unsigned view)
{
    CFMutableDictionaryRef req=dict_new(), params=dict_new();
    if(!req||!params){if(req)p_CFRelease(req);if(params)p_CFRelease(params);return NULL;}
    set_str(req,"type","updateViewArea");
    set_str(params,"uuid",g_alt_uuid);
    set_i64(params,"viewAreaIndex",(int64_t)view);
    set_i64(params,"animationDurationMillis",0);
    { CFStringRef k=s_cf("params"); p_CFDictionarySetValue(req,k,params); p_CFRelease(k); }
    p_CFRelease(params); return req;
}

static CFMutableDictionaryRef gen2_command_dictionary(const struct alt111_command *cmd)
{
    if (!cmd) return NULL;
    switch (cmd->type) {
    case ALT111_CMD_SHOW: return command_showui();
    case ALT111_CMD_STOP: return command_stopui();
    case ALT111_CMD_VIEW: return command_update_view(cmd->view);
    case ALT111_CMD_KEYFRAME: return command_force_keyframe();
    default: return NULL;
    }
}

static void gen2_publish_status(void)
{
    char b[768];
    int fd, n;
    struct alt111_control cs;
    struct alt111_video vs;
    unsigned command_ready;
    uint64_t last_dispatched, last_completed;
    int last_completion_status;
    pthread_mutex_lock(&g2_core_lock);
    cs = g2_control;
    vs = g2_video;
    command_ready = g2_command_ready;
    last_dispatched = g2_last_dispatched_request;
    last_completed = g2_last_completed_request;
    last_completion_status = g2_last_completion_status;
    pthread_mutex_unlock(&g2_core_lock);
    n = snprintf(b,sizeof(b),
        "gen2=1\ncontrol_session=%llu\ncommand_ready=%u\nprojection_desired=%u\nshown_ack=%u\nreacquiring=%u\n"
        "last_dispatched_request=%llu\nlast_completed_request=%llu\nlast_completion_status=%d\n"
        "stream_gen=%llu\ncodec_gen=%llu\nconsumer_gen=%llu\nconfig_valid=%u\n"
        "source_aus=%llu\nsource_idrs=%llu\nconsumer_primed=%u\ndelivered_aus=%llu\n"
        "dropped_aus=%llu\nqueue_count=%u\nqueue_bytes=%zu\n",
        (unsigned long long)cs.session,command_ready,cs.desired,cs.shown_ack,cs.reacquiring,
        (unsigned long long)last_dispatched,(unsigned long long)last_completed,last_completion_status,
        (unsigned long long)vs.stream,(unsigned long long)vs.codec,
        (unsigned long long)vs.consumer,vs.config_valid,
        (unsigned long long)vs.source_aus,(unsigned long long)vs.source_idrs,
        vs.consumer_primed,(unsigned long long)vs.delivered_aus,
        (unsigned long long)vs.dropped_aus,vs.count,vs.queued_bytes);
    if(n<=0)return;
    if((size_t)n>=sizeof(b))n=(int)sizeof(b)-1;
    pthread_mutex_lock(&g2_status_lock);
    fd=open(g2_status_path,O_WRONLY|O_CREAT|O_TRUNC,0644);
    if(fd>=0){(void)write(fd,b,(size_t)n);close(fd);}
    pthread_mutex_unlock(&g2_status_lock);
}

static void gen2_set_command_ready(unsigned ready)
{
    pthread_mutex_lock(&g2_core_lock);
    g2_command_ready = ready ? 1u : 0u;
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);
    gen2_publish_status();
}

static void gen2_control_projection_on(void)
{
    int repeated = 0, rrc = ALT111_OK;
    pthread_mutex_lock(&g2_core_lock);
    if (!g2_control_session) {
        g2_control_session = alt111_control_begin(&g2_control);
    } else {
        repeated = 1;
    }
    (void)alt111_control_intent(&g2_control,1,0);
    if (repeated)
        rrc = alt111_control_reacquire(&g2_control,g2_control_session);
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);
    if (repeated)
        logf_u2("gen2 repeated stream111 SETUP -> ownership reacquire rc=%d",rrc);
    gen2_publish_status();
}

static void gen2_control_release(void)
{
    uint64_t s;
    int stop_handed_off = 0;
    pthread_mutex_lock(&g2_core_lock);
    s=g2_control_session;
    if(s)(void)alt111_control_intent(&g2_control,0,0);
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);

    /*
     * Do not confuse SendCommand() queueing with the controller response.
     * During teardown we cannot wait indefinitely for the HTTP completion,
     * but we do wait until either:
     *   - stopUI has completed successfully (may_be_visible == 0), or
     *   - the STOP request has at least been handed to AirPlay's HTTP client.
     * A later callback is generation-checked by the core and may go stale
     * after alt111_control_end(), which is intentional.
     */
    if(s){
        unsigned waitn;
        for(waitn=0; waitn<100; ++waitn){
            pthread_mutex_lock(&g2_core_lock);
            if(!g2_control.may_be_visible){
                stop_handed_off = 1;
            }else if(g2_control.pending.type == ALT111_CMD_STOP &&
                     g2_last_dispatched_request == g2_control.pending.request){
                stop_handed_off = 1;
            }
            pthread_mutex_unlock(&g2_core_lock);
            if(stop_handed_off)break;
            usleep(5000);
        }
        if(!stop_handed_off)
            logf_u2("gen2 stopUI not handed off within bounded teardown window session=%llu",
                    (unsigned long long)s);
    }

    pthread_mutex_lock(&g2_core_lock);
    if(s)(void)alt111_control_end(&g2_control,s);
    g2_control_session=0;
    pthread_mutex_unlock(&g2_core_lock);
    gen2_publish_status();
}

struct gen2_command_context {
    uint64_t session;
    uint64_t request;
    unsigned type;
};

static void gen2_command_completion(OSStatus status, CFDictionaryRef response, void *opaque)
{
    struct gen2_command_context *ctx = (struct gen2_command_context *)opaque;
    int crc = ALT111_STALE;
    (void)response;
    if(!ctx)return;

    pthread_mutex_lock(&g2_core_lock);
    g2_last_completed_request = ctx->request;
    g2_last_completion_status = (int)status;
    crc = alt111_control_complete(&g2_control,ctx->session,ctx->request,
                                  status==K_NO_ERR,monotonic_ms());
    pthread_cond_broadcast(&g2_core_cv);
    pthread_mutex_unlock(&g2_core_lock);

    logf_u2("gen2 UI completion type=%u request=%llu status=%d core_rc=%d",
            ctx->type,(unsigned long long)ctx->request,(int)status,crc);
    free(ctx);
    gen2_publish_status();
}

static void *gen2_control_worker(void *arg)
{
    (void)arg;
    for (;;) {
        struct alt111_command cmd;
        struct gen2_command_context *ctx=NULL;
        int nrc, queued=0;
        CFMutableDictionaryRef req=NULL;
        AirPlayReceiverSessionRef s=NULL;
        OSStatus e=-1;

        pthread_mutex_lock(&g2_core_lock);
        if(!g2_command_ready){
            pthread_mutex_unlock(&g2_core_lock);
            usleep(10000);
            continue;
        }
        pthread_mutex_unlock(&g2_core_lock);

        if(access(g2_reacquire_marker,F_OK)==0){
            unlink(g2_reacquire_marker);
            pthread_mutex_lock(&g2_core_lock);
            if(g2_control_session){
                int r=alt111_control_reacquire(&g2_control,g2_control_session);
                logf_u2("gen2 explicit reacquire requested rc=%d",r);
            }
            pthread_mutex_unlock(&g2_core_lock);
        }

        pthread_mutex_lock(&g2_core_lock);
        nrc=alt111_control_next(&g2_control,monotonic_ms(),&cmd);
        pthread_mutex_unlock(&g2_core_lock);
        if(nrc!=ALT111_OK){usleep(10000);continue;}

        req=gen2_command_dictionary(&cmd);
        s=retain_active_session();
        ctx=(struct gen2_command_context *)calloc(1,sizeof(*ctx));
        if(ctx){
            ctx->session=cmd.session;
            ctx->request=cmd.request;
            ctx->type=(unsigned)cmd.type;
        }

        if(req&&s&&ctx){
            e=g_sendcmd(s,req,gen2_command_completion,ctx);
            if(e==K_NO_ERR){
                queued=1;
                pthread_mutex_lock(&g2_core_lock);
                g2_last_dispatched_request=cmd.request;
                pthread_cond_broadcast(&g2_core_cv);
                pthread_mutex_unlock(&g2_core_lock);
                ctx=NULL; /* completion callback owns it */
            }
        }
        if(req)p_CFRelease(req);
        if(s)p_CFRelease(s);

        if(!queued){
            if(ctx)free(ctx);
            pthread_mutex_lock(&g2_core_lock);
            (void)alt111_control_complete(&g2_control,cmd.session,cmd.request,0,monotonic_ms());
            pthread_cond_broadcast(&g2_core_cv);
            pthread_mutex_unlock(&g2_core_lock);
            logf_u2("gen2 UI command dispatch failed type=%u request=%llu os=%d",
                    (unsigned)cmd.type,(unsigned long long)cmd.request,(int)e);
        }else{
            logf_u2("gen2 UI command dispatched type=%u request=%llu awaiting completion",
                    (unsigned)cmd.type,(unsigned long long)cmd.request);
        }
        gen2_publish_status();
    }
    return NULL;
}

static int gen2_start_workers(void)
{
    if(g2_workers_started)return 0;
    if(pthread_create(&g2_control_thread,NULL,gen2_control_worker,NULL)!=0)return -1;
    pthread_detach(g2_control_thread);
    if(pthread_create(&g2_output_thread,NULL,gen2_output_worker,NULL)!=0)return -1;
    pthread_detach(g2_output_thread);
    g2_workers_started=1;
    return 0;
}

/* ARM32 absolute jump trampoline: first 8 target bytes must be position-independent prologue. */
static int install_arm_hook(void *target, void *replacement, void **trampoline, const char *name)
{
#if defined(__arm__)
    uint32_t *src=(uint32_t *)target;
    uint32_t expected0=0xe92d4ff0u, expected1=0xed2d8b02u;
    uint32_t *tr;
    long ps=sysconf(_SC_PAGESIZE);
    uintptr_t page;
    int saved_errno;

    if(ps<=0){
        logf_u2("%s hook sysconf(_SC_PAGESIZE) failed ps=%ld errno=%d %s",
                name,ps,errno,strerror(errno));
        return -1;
    }
    page=((uintptr_t)target)&~((uintptr_t)ps-1u);

    if(src[0]!=expected0||src[1]!=expected1){
        logf_u2("%s prologue mismatch %08x %08x",name,src[0],src[1]);
        return -1;
    }

    /*
     * Keep W^X: never request writable+executable memory at the same time.
     * Older QNX targets may reject RWX even though an RW -> RX transition is
     * permitted.
     */
    errno=0;
    tr=mmap(NULL,16,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    if(tr==MAP_FAILED){
        saved_errno=errno;
        logf_u2("%s trampoline mmap RW failed errno=%d %s",
                name,saved_errno,strerror(saved_errno));
        return -1;
    }

    tr[0]=src[0];
    tr[1]=src[1];
    tr[2]=0xe51ff004u;
    tr[3]=(uint32_t)((uintptr_t)target+8u);
    __builtin___clear_cache((char *)tr,(char *)tr+16);

    errno=0;
    if(mprotect((void *)tr,16,PROT_READ|PROT_EXEC)!=0){
        saved_errno=errno;
        logf_u2("%s trampoline mprotect RX failed errno=%d %s",
                name,saved_errno,strerror(saved_errno));
        munmap(tr,16);
        return -1;
    }

    errno=0;
    if(mprotect((void *)page,(size_t)ps,PROT_READ|PROT_WRITE)!=0){
        uint8_t *shadow;
        void *fixed;
        saved_errno=errno;
        logf_u2("%s target mprotect RW denied page=%p size=%ld errno=%d %s; trying anonymous MAP_FIXED clone",
                name,(void *)page,ps,saved_errno,strerror(saved_errno));

        /*
         * QNX may refuse write permission on a file-backed executable text
         * mapping. Preserve the already-relocated live page byte-for-byte,
         * replace only that page by private anonymous RW memory at the same
         * virtual address, then restore RX after the 8-byte hook patch.
         *
         * The target functions live in libairplay; this installer executes
         * from the preload library, so replacing the target page does not
         * replace the currently executing code.
         */
        shadow=(uint8_t *)malloc((size_t)ps);
        if(!shadow){
            logf_u2("%s target page shadow allocation failed size=%ld",name,ps);
            munmap(tr,16);
            return -1;
        }
        memcpy(shadow,(const void *)page,(size_t)ps);

        errno=0;
        fixed=mmap((void *)page,(size_t)ps,PROT_READ|PROT_WRITE,
                   MAP_PRIVATE|MAP_ANON|MAP_FIXED,-1,0);
        if(fixed==MAP_FAILED || fixed!=(void *)page){
            saved_errno=errno;
            logf_u2("%s anonymous MAP_FIXED clone failed wanted=%p got=%p size=%ld errno=%d %s",
                    name,(void *)page,fixed,ps,saved_errno,strerror(saved_errno));
            free(shadow);
            munmap(tr,16);
            return -1;
        }
        memcpy(fixed,shadow,(size_t)ps);
        free(shadow);
        src=(uint32_t *)target;
        logf_u2("%s target page cloned private RW at %p size=%ld",
                name,(void *)page,ps);
    }

    src[0]=0xe51ff004u;
    src[1]=(uint32_t)(uintptr_t)replacement;
    __builtin___clear_cache((char *)src,(char *)src+8);

    errno=0;
    if(mprotect((void *)page,(size_t)ps,PROT_READ|PROT_EXEC)!=0){
        saved_errno=errno;
        logf_u2("%s target restore RX failed page=%p size=%ld errno=%d %s",
                name,(void *)page,ps,saved_errno,strerror(saved_errno));
        /* The hook is already installed. Keep the trampoline authoritative. */
    }

    *trampoline=tr;
    logf_u2("installed ARM hook %s target=%p replacement=%p trampoline=%p page=%p pagesize=%ld",
            name,target,replacement,tr,(void *)page,ps);
    return 0;
#else
    (void)target;(void)replacement;(void)trampoline;(void)name;
    return 0; /* host syntax-test only */
#endif
}

static OSStatus call_stock_setup(AirPlayReceiverSessionRef s, CFDictionaryRef request, CFDictionaryRef *outResponse)
{
    fn_setup_t fn = g_setup_trampoline ? g_setup_trampoline : g_real_setup;
    if(!fn){
        logf_u2("FATAL no stock SessionSetup delegate");
        return -1;
    }
    return fn(s,request,outResponse);
}

static OSStatus call_stock_start(AirPlayReceiverSessionRef s, void *info)
{
    fn_start_t fn = g_start_trampoline ? g_start_trampoline : g_real_start;
    if(!fn){
        logf_u2("FATAL no stock SessionStart delegate");
        return -1;
    }
    return fn(s,info);
}

static void call_stock_teardown(AirPlayReceiverSessionRef s, CFDictionaryRef request, OSStatus reason, Boolean *outDone)
{
    fn_teardown_t fn = g_teardown_trampoline ? g_teardown_trampoline : g_real_teardown;
    if(!fn){
        logf_u2("FATAL no stock SessionTearDown delegate");
        if(outDone)*outDone=1;
        return;
    }
    fn(s,request,reason,outDone);
}

static OSStatus mibr_session_setup(AirPlayReceiverSessionRef s, CFDictionaryRef request, CFDictionaryRef *outResponse)
{
    CFDictionaryRef altDesc=NULL;
    CFDictionaryRef stockResp=NULL;
    int other=0,hasAlt=0;
    OSStatus e;
    int port=-1;
    uint64_t cid=0;

    log_stream_types("SETUP entry",request);

    /*
     * Recovered IRC ordering: pass the original request to stock first.
     * Only after a successful stock response advertise root capabilities and
     * inspect the original request for stream 111.
     */
    e=call_stock_setup(s,request,&stockResp);
    logf_u2("SETUP stock result=%d response=%s",(int)e,stockResp?"yes":"no");
    if(e==K_NO_ERR && g_enabled) set_active_session(s);
    if(e!=K_NO_ERR){
        if(outResponse)*outResponse=stockResp;
        else if(stockResp)p_CFRelease(stockResp);
        return e;
    }

    /*
     * A partially installed inline-hook set must be indistinguishable from
     * stock once the constructor disables GEN2. In particular, do not mutate
     * enabledFeatures when SessionSetup was hooked successfully but a later
     * SessionStart/TearDown hook failed.
     */
    if(!g_enabled){
        if(outResponse)*outResponse=stockResp;
        else if(stockResp)p_CFRelease(stockResp);
        return e;
    }

    if(stockResp)set_reference_enabled_features((CFMutableDictionaryRef)stockResp);

    hasAlt=g_enabled && contains_stream111(request,&altDesc,&other);
    if(!hasAlt){
        if(outResponse)*outResponse=stockResp;
        else if(stockResp)p_CFRelease(stockResp);
        return e;
    }

    cid=stream_connection_id(altDesc);
    logf_u2("GEN2 SETUP contains stream111 cid=%llu otherStreams=%d",
            (unsigned long long)cid,other);
    publish_state("setup");

    if(!cid){
        logf_u2("stream111 SETUP fail-soft: missing streamConnectionID");
        publish_state("setup_failed");
    }else{
        port=start_alt_receiver(cid);
        if(port>0 && stockResp){
            append_alt_setup_response((CFMutableDictionaryRef)stockResp,altDesc,port);
            gen2_control_projection_on();
            logf_u2("GEN2 stream111 SETUP accepted cid=%llu dataPort=%d response=cloned-request+streamID111; UI acquisition armed",
                    (unsigned long long)cid,port);
        }else{
            if(port>0) stop_alt_receiver();
            clear_video_observer();
            logf_u2("stream111 SETUP fail-soft: receiver/key setup failed cid=%llu",
                    (unsigned long long)cid);
            publish_state("setup_failed");
        }
    }

    if(outResponse)*outResponse=stockResp;
    else if(stockResp)p_CFRelease(stockResp);
    return e;
}

static OSStatus mibr_session_start(AirPlayReceiverSessionRef s, void *info)
{
    OSStatus e=call_stock_start(s,info);
    if(e==K_NO_ERR && g_enabled) {
        set_active_session(s);
        gen2_set_command_ready(1);
    }
    /* Gen-2 ownership commands are serialized by gen2_control_worker. */
    return e;
}

static void clear_master_key(void)
{
    pthread_mutex_lock(&g_lock);
    memset(g_master_key, 0, sizeof(g_master_key));
    g_master_valid = 0;
    pthread_mutex_unlock(&g_lock);
    logf_u2("cleared captured CarPlay master AES key");
}

static void mibr_session_teardown(AirPlayReceiverSessionRef s, CFDictionaryRef request, OSStatus reason, Boolean *outDone)
{
    int other=0;
    int has_alt;

    if(!g_enabled){
        call_stock_teardown(s,request,reason,outDone);
        return;
    }

    has_alt = request && contains_stream111(request,NULL,&other);
    int has_main = request && contains_stream_type(request,110);
    int full_session = (request == NULL) || has_main;

    /*
     * Do not kill stream 111 merely because CarPlay tears down an audio or
     * microphone stream. Stop it only when 111 itself or the main screen/full
     * session is being torn down.
     */
    if(has_alt){
        logf_u2("GEN2 TEARDOWN includes stream111 otherStreams=%d main=%d",other,has_main);
        gen2_control_release();
        if(full_session) gen2_set_command_ready(0);
        stop_alt_receiver();
        clear_video_observer();
        publish_state(full_session ? "idle" : "idle");
        if(other==0){if(outDone)*outDone=0;return;}
        {
            CFMutableDictionaryRef f=clone_without_111(request);
            call_stock_teardown(s,f?f:request,reason,outDone);
            if(f)p_CFRelease(f);
        }
        if(full_session) {
            set_active_session(NULL);
            clear_master_key();
        }
        return;
    }

    if(full_session) {
        gen2_control_release();
        gen2_set_command_ready(0);
        stop_alt_receiver();
        clear_video_observer();
        publish_state("idle");
    }
    call_stock_teardown(s,request,reason,outDone);
    if(full_session) {
        set_active_session(NULL);
        clear_master_key();
    }
}

OSStatus AirPlayReceiverSessionSetup(AirPlayReceiverSessionRef s, CFDictionaryRef request, CFDictionaryRef *outResponse)
{
    return mibr_session_setup(s,request,outResponse);
}

OSStatus AirPlayReceiverSessionStart(AirPlayReceiverSessionRef s, void *info)
{
    return mibr_session_start(s,info);
}

void AirPlayReceiverSessionTearDown(AirPlayReceiverSessionRef s, CFDictionaryRef request, OSStatus reason, Boolean *outDone)
{
    mibr_session_teardown(s,request,reason,outDone);
}

/* Capture the master session key at the exact stock SetSecurityInfo caller. */
__attribute__((noinline))
OSStatus AES_CBCFrame_Init(void *ctx, const uint8_t key[16], const uint8_t iv[16], Boolean encrypt)
{
    void *ra=__builtin_return_address(0);
    OSStatus e;
    if(!g_real_aes_cbc_init)g_real_aes_cbc_init=(fn_aes_cbc_init_t)sym_next("AES_CBCFrame_Init");
    e=g_real_aes_cbc_init(ctx,key,iv,encrypt);
    if(g_enabled && e==K_NO_ERR && !encrypt && g_security_fn &&
       (uintptr_t)ra>=g_security_fn && (uintptr_t)ra<g_security_fn+0x100u){
        pthread_mutex_lock(&g_lock); memcpy(g_master_key,key,16); g_master_valid=1; pthread_mutex_unlock(&g_lock);
        logf_u2("captured stock CarPlay master AES key at SetSecurityInfo caller");
    }
    return e;
}

__attribute__((constructor))
static void altscreen111_init(void)
{
    void *setup,*start,*td;
    signal(SIGPIPE,SIG_IGN);
    g_enabled=env_i("ALTSCREEN111_ENABLED",1);
    g_alt_port=env_i("ALTSCREEN111_PORT",6031);
    g_tee_port=env_i("ALTSCREEN111_TEE_PORT",19820);
    g_width=env_i("ALTSCREEN111_WIDTH",1010);
    g_height=env_i("ALTSCREEN111_HEIGHT",376);
    /* Keep MU-target physical geometry; Audi 290x90 is not portable evidence. */
    g_width_mm=env_i("ALTSCREEN111_WIDTH_MM",200);
    g_height_mm=env_i("ALTSCREEN111_HEIGHT_MM",
                      g_width>0 ? (200*g_height)/g_width : 74);
    g_fps=env_i("ALTSCREEN111_FPS",30);
    /*
     * Reference IRC does not prove startup showUI/forceKeyFrame. Ignore the
     * legacy AUTO_SHOW=1 package default unless parity-specific opt-in is set.
     */
    g_auto_show=0; /* Gen-2 control worker owns UI acquisition. */
    if(access(g_autoshow_disable_marker,F_OK)==0)g_auto_show=0;
    g_viewareas=env_i("ALTSCREEN111_PARITY_VIEWAREAS",1);
    if(access(g_viewareas_marker,F_OK)==0)g_viewareas=1;
    /* Do not inherit Run117/118 legacy UUID/URL env values in the parity build. */
    env_s("ALTSCREEN111_PARITY_UUID",g_alt_uuid,sizeof(g_alt_uuid),ALT_UUID_DEFAULT);
    env_s("ALTSCREEN111_PARITY_URL",g_alt_url,sizeof(g_alt_url),ALT_URL_DEFAULT);
    clear_video_observer();
    publish_state(g_enabled ? "initializing" : "disabled");
    if(!g_enabled)return;
    if(init_api()!=0){
        g_enabled=0;
        publish_state("error");
        logf_u2("GEN2 disabled: API initialization failed");
        return;
    }

    alt111_profile_mu1440(&g2_profile,0);
    if(g_width<=0 || g_height<=0 || g_width_mm<=0 || g_height_mm<=0 ||
       g_fps<=0 || g_fps>60){
        logf_u2("gen2 invalid runtime geometry/fps %dx%d physical=%dx%d fps=%d",
                g_width,g_height,g_width_mm,g_height_mm,g_fps);
        publish_state("error");
        return;
    }
    if(strlen(g_alt_uuid)!=36u){
        logf_u2("gen2 invalid secondary display UUID length=%zu",strlen(g_alt_uuid));
        publish_state("error");
        return;
    }
    memcpy(g2_profile.uuid,g_alt_uuid,37u);
    g2_profile.width=(uint32_t)g_width;
    g2_profile.height=(uint32_t)g_height;
    g2_profile.width_mm=(uint32_t)g_width_mm;
    g2_profile.height_mm=(uint32_t)g_height_mm;
    g2_profile.max_fps=(uint32_t)g_fps;
    g2_profile.views[0].area.width=g2_profile.width;
    g2_profile.views[0].area.height=g2_profile.height;
    g2_profile.views[0].safe=g2_profile.views[0].area;
    if(alt111_profile_validate(&g2_profile)!=ALT111_OK){
        g_enabled=0;
        logf_u2("gen2 runtime profile validation failed; disabled fail-closed");
        publish_state("error");
        return;
    }
    if(alt111_control_init(&g2_control,g2_profile.view_count)!=ALT111_OK){
        g_enabled=0;
        logf_u2("gen2 control init failed; disabled fail-closed");
        publish_state("error");
        return;
    }
    alt111_video_init(&g2_video);
    if(gen2_start_workers()!=0){
        g_enabled=0;
        logf_u2("gen2 worker start failed; disabled fail-closed");
        publish_state("error");
        return;
    }
    if(start_tee_server()!=0){
        g_enabled=0;
        publish_state("error");
        logf_u2("cannot start local H264 tee; AltScreen disabled fail-closed");
        return;
    }
    setup=sym_next("AirPlayReceiverSessionSetup");
    start=sym_next("AirPlayReceiverSessionStart");
    td=sym_next("AirPlayReceiverSessionTearDown");
    g_real_setup=(fn_setup_t)setup;
    g_real_start=(fn_start_t)start;
    g_real_teardown=(fn_teardown_t)td;
    if(!setup||!start||!td){
        logf_u2("missing direct-hook target(s)");
        g_enabled=0;
        publish_state("error");
        return;
    }
    logf_u2("hook destinations setup=%p start=%p teardown=%p",
            (void *)mibr_session_setup,(void *)mibr_session_start,(void *)mibr_session_teardown);
    if(install_arm_hook(setup,(void *)mibr_session_setup,(void **)&g_setup_trampoline,"SessionSetup")!=0 ||
       install_arm_hook(start,(void *)mibr_session_start,(void **)&g_start_trampoline,"SessionStart")!=0 ||
       install_arm_hook(td,(void *)mibr_session_teardown,(void **)&g_teardown_trampoline,"SessionTearDown")!=0){
        g_enabled=0;
        publish_state("error");
        logf_u2("direct hook install failed; disabled fail-closed");
        return;
    }
    publish_state("ready");
    logf_u2("GEN2 candidate active: 111=%dx%d@%d physical=%dx%d altPort=%d tee=%d URL=%s uuid=%s viewAreas=%d autoShow=%d bit26Mode=%d",
            g_width,g_height,g_fps,g_width_mm,g_height_mm,g_alt_port,g_tee_port,active_alt_url(),g_alt_uuid,
            g_viewareas,g_auto_show,airplay_bit26_mode());
}

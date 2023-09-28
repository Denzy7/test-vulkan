#include <android/native_activity.h>
#include <android/native_app_glue/android_native_app_glue.h>

#include "teskvk_android.h"

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int vkmain(int argc, char** argv);
struct android_app* _vkapp = NULL;
int logthr_run = 0;

struct android_app* testvk_getapp()
{
    return _vkapp;
}

void* logthr(void* arg)
{
    struct pollfd fd;
    int ipc[2];
    int i;
    char *buf;
    size_t buflen = 8192;
    ssize_t rd;

    buf = malloc(buflen);
    pipe(ipc);

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    dup2(ipc[1], STDOUT_FILENO);
    dup2(ipc[1], STDERR_FILENO);

    fd.fd = ipc[0];
    fd.events = POLLIN;

    logthr_run = 1;

    while(logthr_run)
    {
        poll(&fd, 1, -1);
        if(fd.revents & POLLIN)
        {
            memset(buf, 0, buflen);
            rd = read(ipc[0], buf, buflen);
            LOGI("%s", buf);
        }
    }
    free(buf);

    return NULL;
}

void android_main(struct android_app* app)
{
    pthread_t logthr_thr;
    pthread_create(&logthr_thr, NULL, logthr, NULL);
    int vkmain_status;

    while(!logthr_run)
        continue;

    _vkapp = app;

    LOGI("ANDROID_MAIN: START");

    static char* argv[] = 
    {
        "vkmain",

        /*"-uselayers",*/
        /* extract layers to app/src/main/jniLibs:
         * https://developer.android.com/ndk/guides/graphics/validation-layer
         *
         * also enable VK_EXT_debug_utils in window_andk.c
         * */

        "-bufferframes",
        "3"
    };
    vkmain_status = vkmain(arysz(argv), argv);
    LOGI("ANDROID_MAIN: vkmain exited status : %d",vkmain_status );
    logthr_run = 0;
    pthread_join(logthr_thr, NULL);
    LOGI("ANDROID_MAIN: DONE");
    return;
}

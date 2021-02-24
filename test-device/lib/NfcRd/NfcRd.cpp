/*--------------------------------------------------------------------------------
File Name	:	NfcRd.cpp 
Description	:	NFC读卡器控制接口jni库，兼容一下芯片
				NXP：PN512,RC522,RC520
				FM：FM17520,FM17522,FM17550
Revision	:	V1.0.0
Author		:	Bryan Jiang <598612779@qq.com>
Date		:	2018/10/25
----------------------------------------------------------------------------------*/

#define LOG_TAG "NfcRd"
//#define LOG_NDDEBUG 0

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <cassert>
#include <cstdlib>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>

#include "jni.h"
#include "JNIHelp.h"

#include "pn512.h"
#include "MifareCard.h"
#include "yl0301.h"

using namespace std;

void SwipingCallback(NfcDev *dev, unsigned char* id, int len, int type);

class NfcRd
{
private:

public:
	NfcRd();
	~NfcRd();

	PN512 *ic_dev;
	YL0301 *id_dev;
	MifareCard *Mifare;

	JavaVM *g_VM;
	jobject g_obj;

};

NfcRd::NfcRd()
{
	ic_dev = new PN512();
	Mifare = new MifareCard(ic_dev->pn512_io);
	//配置回调函数
	ic_dev->swiping_event = SwipingCallback;

	id_dev = new YL0301();
	//配置回调函数
	id_dev->swiping_event = SwipingCallback;
}

NfcRd::~NfcRd()
{
	delete Mifare;
	Mifare = NULL;

	delete ic_dev;
	ic_dev = NULL;

	delete id_dev;
	id_dev = NULL;
}

static NfcRd *NfcRead = NULL;

//----------------------------------------------------------------------
// Name:			ID_SwipingCallback
// Parameters:		
// Returns:			
// Description:		刷卡回调函数
//----------------------------------------------------------------------
void SwipingCallback(NfcDev *dev, unsigned char* id, int len, int type)
{
	JNIEnv *env;
	int mNeedDetach;
	int res, i;
	char str[64], *p_str;
	jstring uidStr;
	
	ALOGI("Got ID card(%d):%02X%02X%02X%02X\n", len, id[0], id[1], id[2], id[3]);

	//获取当前native线程是否有没有被附加到jvm环境中
	int getEnvStat = NfcRead->g_VM->GetEnv((void **)&env, JNI_VERSION_1_6);
	if (getEnvStat == JNI_EDETACHED) {
		//如果没有， 主动附加到jvm环境中，获取到env
		if (NfcRead->g_VM->AttachCurrentThread(&env, NULL) != 0) {
			return;
		}
		mNeedDetach = JNI_TRUE;
	}

	//通过全局变量g_obj 获取到要回调的类
	jclass javaClass = env->GetObjectClass(NfcRead->g_obj);
	if (javaClass == 0) {
		ALOGE("%s:Unable to find class", __func__);
		NfcRead->g_VM->DetachCurrentThread();
		return;
	}

	//获取要回调的方法ID
	jmethodID javaCallbackId = env->GetMethodID(javaClass, "NfcRdEventCallback", "(Ljava/lang/String;I)I");
	if (javaCallbackId == NULL) {
		ALOGE("%s:Unable to find method : NfcRdEventCallback", __func__);
		NfcRead->g_VM->DetachCurrentThread();
		return;
	}

	//将UID号转换为字符串
	p_str = str;
	for (i = 0; i < len; i++)
	{
		sprintf(p_str, "%02X", id[i]);
		p_str += 2;
	}
	*p_str = 0;

	uidStr = env->NewStringUTF(str);
	//执行回调
	env->CallIntMethod(NfcRead->g_obj, javaCallbackId, uidStr, type);

	//释放当前线程
	if (mNeedDetach) {
		NfcRead->g_VM->DetachCurrentThread();
	}
	env = NULL;
}


//----------------------------------------------------------------------
// Name:			Open
// Parameters:		none
// Returns:			=0 :成功;
//					<0 :失败;
// Description:		开启NFC读卡器
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_NfcRdFactory_Open(JNIEnv *env, jobject thiz)
{
	int ret;

	if (NfcRead != NULL)
	{
		ALOGE("%s:NfcRd already opened\n", __func__);
		return -1;
	}

	ALOGI("%s:Open the NfcRd function\n", __func__);

	NfcRead = new NfcRd();

	/*
	//JavaVM是虚拟机在JNI中的表示，等下再其他线程回调java层需要用到
	NfcRead->ic_dev->env->GetJavaVM(&NfcRead->ic_dev->g_VM);
	NfcRead->id_dev->env->GetJavaVM(&NfcRead->id_dev->g_VM);
	// 生成一个全局引用保留下来，以便回调
	NfcRead->ic_dev->g_obj = NfcRead->ic_dev->env->NewGlobalRef(thiz);
	NfcRead->id_dev->g_obj = NfcRead->id_dev->env->NewGlobalRef(thiz);
	*/

	//JavaVM是虚拟机在JNI中的表示，等下再其他线程回调java层需要用到
	env->GetJavaVM(&NfcRead->g_VM);
	// 生成一个全局引用保留下来，以便回调
	NfcRead->g_obj = env->NewGlobalRef(thiz);
	
	//启动线程
	NfcRead->ic_dev->start();
	NfcRead->id_dev->start();

	return 0;
}

extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_Open(JNIEnv *env, jobject thiz)
{
	return Java_com_common_nfc_api_NfcRd_NfcRdFactory_Open(env, thiz);
}

//----------------------------------------------------------------------
// Name:			Close
// Parameters:		none
// Returns:			=0 :成功;
//					<0 :失败;
// Description:		关闭NFC读卡器
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_NfcRdFactory_Close(JNIEnv *env, jobject thiz)
{
	if (NfcRead == NULL)
	{
		ALOGE("%s:NfcRd already close\n", __func__);
		return 0;
	}

	ALOGI("%s:Close the NfcRd function\n", __func__);

	//结束线程
	NfcRead->ic_dev->stop();
	NfcRead->id_dev->stop();

	/*
	//释放当前线程
	if (NfcRead->ic_dev->javaCallbackId != NULL) {
		NfcRead->ic_dev->g_VM->DetachCurrentThread();
	}
	NfcRead->ic_dev->env = NULL;
	if (NfcRead->id_dev->javaCallbackId != NULL) {
		NfcRead->id_dev->g_VM->DetachCurrentThread();
	}
	NfcRead->id_dev->env = NULL;
	*/

	NfcRead->~NfcRd();
	delete NfcRead;
	NfcRead = NULL;

	return 0;
}

extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_Close(JNIEnv *env, jobject thiz)
{
	return Java_com_common_nfc_api_NfcRd_NfcRdFactory_Close(env, thiz);
}

//----------------------------------------------------------------------
// Name:			MifareAuth
// Parameters:
//      mode：      认证模式，0：key A认证，1：key B认证；
//      sector：    认证的扇区号，0~15
// 	    key：       6字节认证密钥数组
// Returns:			=0 :成功;
//					<0 :失败;
// Description:		M1卡密钥认证
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareAuth(JNIEnv *env, jobject thiz, jint mode, jint sector, jcharArray key)
{
	jchar* key_buf;
	jsize size;
	unsigned char u8_key[6], i;

	
	if (NfcRead->ic_dev->get_available() == 0){
		ALOGE("%s:NfcRd device not work\n", __func__);
		return -1;
	}

	if(NfcRead->ic_dev->foundcard == 0){
		ALOGE("%s:cannot find IC card\n", __func__);
		return -2;
	}

	key_buf = env->GetCharArrayElements(key, 0);
	size = env->GetArrayLength(key);
	if (size != 6) {
		ALOGE("%s:auth key size not match\n", __func__);
		return -3;
	}

	for (i = 0; i < 6; i++)
		u8_key[i] = (unsigned char)key_buf[i];

	if (NfcRead->Mifare->Auth(mode, sector, u8_key, NfcRead->ic_dev->uid) >= 0)
		return 0;
	else
		return -4;
}

extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_MifareAuth(JNIEnv *env, jobject thiz, jint mode, jint sector, jcharArray key)
{
	return Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareAuth(env, thiz, mode, sector, key);
}

//----------------------------------------------------------------------
// Name:			MifareBlockRead
// Parameters:
//      block:		块号,0~63
// Returns:			16字节读块数据数组;
//					NULL,失败;
// Description:		M1卡块读取数据
//----------------------------------------------------------------------
extern "C" JNIEXPORT jcharArray
JNICALL Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockRead(JNIEnv *env, jobject thiz, jint block)
{
	jcharArray jarr = env->NewCharArray(16);
	jchar *arr = env->GetCharArrayElements(jarr, NULL);
	unsigned char buf[16] = {0xFF}, i;

	if (NfcRead->ic_dev->get_available() == 0) {
		ALOGE("%s:NfcRd device not work\n", __func__);
		return NULL;
	}

	if (NfcRead->ic_dev->foundcard == 0) {
		ALOGE("%s:cannot find IC card\n", __func__);
		return NULL;
	}

	if (NfcRead->Mifare->BlockRead(block, buf) > 0)
	{
		for (i = 0; i < 16; i++)
			arr[i] = buf[i] & 0xff;
	}
	else
		ALOGE("%s:read error\n", __func__);
			
	env->ReleaseCharArrayElements(jarr, arr, 0);
	return jarr;
}

extern "C" JNIEXPORT jcharArray
JNICALL Java_com_common_nfc_api_NfcRd_MifareBlockRead(JNIEnv *env, jobject thiz, jint block)
{
	return Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockRead(env, thiz, block);
}

//----------------------------------------------------------------------
// Name:			MifareBlockWrite
// Parameters:
//      block:		块号,0~63
//		buffer:		16字节写块数据数组
// Returns:			=0 :成功;
//					<0 :失败;
// Description:		M1卡块写数据
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockWrite(JNIEnv *env, jobject thiz, jint block, jcharArray buffer)
{
	jchar* data;
	jsize size;
	unsigned char u8_data[16], i;
	
	if (NfcRead->ic_dev->get_available() == 0) {
		ALOGE("%s:NfcRd device not work\n", __func__);
		return -1;
	}

	if (NfcRead->ic_dev->foundcard == 0) {
		ALOGE("%s:cannot find IC card\n", __func__);
		return -2;
	}

	data = env->GetCharArrayElements(buffer, 0);
	size = env->GetArrayLength(buffer);
	if (size != 16) {
		ALOGE("%s:block size not match 16\n", __func__);
		return -3;
	}

	for (i = 0; i < 16; i++)
		u8_data[i] = (unsigned char)data[i];

	if (NfcRead->Mifare->BlockWrite((unsigned char)block, u8_data) >= 0)
		return 0;
	else
		return -4;
}

extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_MifareBlockWrite(JNIEnv *env, jobject thiz, jint block, jcharArray buffer)
{
	return Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockWrite(env, thiz, block, buffer);
}

//----------------------------------------------------------------------
// Name:			MifareBlockSet
// Parameters:
//      block:		块号,0~63
//		buffer:		需要设置的4字节数值数组
// Returns:			=0 :成功;
//					<0 :失败;
// Description:		M1卡片数值设置
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockSet(JNIEnv *env, jobject thiz, jint block, jcharArray buffer)
{
	jchar* data;
	jsize size;
	unsigned char u8_data[4], i;
	
	if (NfcRead->ic_dev->get_available() == 0) {
		ALOGE("%s:NfcRd device not work\n", __func__);
		return -1;
	}

	if (NfcRead->ic_dev->foundcard == 0) {
		ALOGE("%s:cannot find IC card\n", __func__);
		return -2;
	}

	data = env->GetCharArrayElements(buffer, 0);
	size = env->GetArrayLength(buffer);
	if (size != 4) {
		ALOGE("%s:block size not match 4\n", __func__);
		return -3;
	}

	for (i = 0; i < 4; i++)
		u8_data[i] = (unsigned char)data[i];

	if (NfcRead->Mifare->BlockSet((unsigned char)block, u8_data) >= 0)
		return 0;
	else
		return -4;
}

extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_MifareBlockSet(JNIEnv *env, jobject thiz, jint block, jcharArray buffer)
{
	return Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockSet(env, thiz, block, buffer);
}

//----------------------------------------------------------------------
// Name:			MifareBlockIn
// Parameters:
//      block:		块号,0~63
//		buffer:		4字节增值数据数组
// Returns:			=0 :成功;
//					<0 :失败;
// Description:		M1卡片增值操作
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockInc(JNIEnv *env, jobject thiz, jint block, jcharArray buffer)
{
	jchar* data;
	jsize size;
	unsigned char u8_data[4], i;

	if (NfcRead->ic_dev->get_available() == 0) {
		ALOGE("%s:NfcRd device not work\n", __func__);
		return -1;
	}

	if (NfcRead->ic_dev->foundcard == 0) {
		ALOGE("%s:cannot find IC card\n", __func__);
		return -2;
	}

	data = env->GetCharArrayElements(buffer, 0);
	size = env->GetArrayLength(buffer);
	if (size != 4) {
		ALOGE("%s:block size not match 4\n", __func__);
		return -3;
	}

	for (i = 0; i < 4; i++)
		u8_data[i] = (unsigned char)data[i];

	if (NfcRead->Mifare->BlockInc((unsigned char)block, u8_data) >= 0)
		return 0;
	else
		return -4;
}

extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_MifareBlockInc(JNIEnv *env, jobject thiz, jint block, jcharArray buffer)
{
	return Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockInc(env, thiz, block, buffer);
}

//----------------------------------------------------------------------
// Name:			MifareBlockDec
// Parameters:
//      block:		块号,0~63
//		buffer:		4字节减值数据数组
// Returns:			=0 :成功;
//					<0 :失败;
// Description:		M1卡片减值操作	
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockDec(JNIEnv *env, jobject thiz, jint block, jcharArray buffer)
{
	jchar* data;
	jsize size;
	unsigned char u8_data[4], i;

	if (NfcRead->ic_dev->get_available() == 0) {
		ALOGE("%s:NfcRd device not work\n", __func__);
		return -1;
	}

	if (NfcRead->ic_dev->foundcard == 0) {
		ALOGE("%s:cannot find IC card\n", __func__);
		return -2;
	}

	data = env->GetCharArrayElements(buffer, 0);
	size = env->GetArrayLength(buffer);
	if (size != 4) {
		ALOGE("%s:block size not match 4\n", __func__);
		return -3;
	}

	for (i = 0; i < 4; i++)
		u8_data[i] = (unsigned char)data[i];

	if (NfcRead->Mifare->BlockDec((unsigned char)block, u8_data) >= 0)
		return 0;
	else
		return -4;
}

extern "C" JNIEXPORT jint
JNICALL Java_com_common_nfc_api_NfcRd_MifareBlockDec(JNIEnv *env, jobject thiz, jint block, jcharArray buffer)
{
	return Java_com_common_nfc_api_NfcRd_NfcRdFactory_MifareBlockDec(env, thiz, block, buffer);
}


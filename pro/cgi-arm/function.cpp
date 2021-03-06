/*************************************************************************
 > File Name: function.cpp
 > Author: zyb
 > Mail:nevergdy@gmail.com 
 > Created Time: 2013年01月16日 星期三 15时59分47秒
 ************************************************************************/


#include <stdio.h>
#include "HCNetSDK.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <dirent.h>


extern "C"
{
	#include "cgic.h"
}

#define HPR_OK 1
#define HPR_ERROR 0
#define LOCAL_IP "192.168.1.130"
#define LOCAL_PORT 7200

int Demo_AlarmFortify();
int saveData();
int saveFlag = 0;
char* GetIP();
//char fileName[128] = "/mnt/sdcard/test.mp4";
char fileName[128] = "/test.mp4";
LONG lUserID;
NET_DVR_DEVICEINFO_V30 struDevice;
 long lRealPlayHandle;
 
void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
    char tempbuf[256] = {0};
    switch(dwType) 
    {
    case EXCEPTION_RECONNECT:    //设置预览时重连
        printf("----------reconnect--------%d\n", time(NULL));
        saveFlag = 1;
    break;
	default:
    break;
    }
}

void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void* pUser)
{
	int i;
	NET_DVR_ALARMINFO struAlarmInfo;
	memcpy(&struAlarmInfo, pAlarmInfo, sizeof(NET_DVR_ALARMINFO));
	switch(lCommand) 
	{       
	case COMM_ALARM:
		{
			switch (struAlarmInfo.dwAlarmType)
			{
			case 3: //移动侦测报警
               for (i=0; i<16; i++)   //#define MAX_CHANNUM   16  最大通道数
               {
                   if (struAlarmInfo.dwChannel[i] == 1)
                   {
                       printf("发生报警的通道是：%d\n", i+1);
                       if(saveData() == HPR_ERROR)
							break;
						else
							printf("报警录像成功!\n");
                   }
               }       
			break;
			default:
				break;
          }
		}
	break;
	default:
	break;
  }

}

int mk_dir(char *dir)
{
    DIR *mydir = NULL;
    if((mydir= opendir(dir))==NULL)//判断目录
    {
      int ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);//创建目录
      if (ret != 0)
      {
          return -1;
      }
    }

    return 0;
}

void TestRebootDVR(LONG lUserID)
{
	if(NET_DVR_RebootDVR(lUserID))
		printf("reboot success!\n");
	else
		printf("Reboot failed!%d\n",NET_DVR_GetLastError());
}


void TestShutdownDVR(LONG lUserID)
{
	if(NET_DVR_ShutDownDVR(lUserID))
		printf("shutdown success!\n");
	else
		printf("shutdown failed!%d\n",NET_DVR_GetLastError());
}

int saveData()
{
	/*
	printf("Start realplay...\n");
    lRealPlayHandle = NET_DVR_RealPlay_V30(lUserID, &ClientInfo, g_RealDataCallBack_V30, NULL, 0);
    lRealPlayHandle = NET_DVR_RealPlay_V30(lUserID, &ClientInfo,NULL,NULL,0);
    if (lRealPlayHandle < 0)
    {
        printf("pyd1---NET_DVR_RealPlay_V30 error\n");
        return HPR_ERROR;
    }

    //Set rtsp callback function of getting stream.
    //NET_DVR_SetStandardDataCallBack(lRealPlayHandle, g_StdDataCallBack, 0);
    */
    
    char dir[128]="/opt/homewell/video/";
    strcat(dir,GetIP());
    mk_dir(dir);
    printf("Start save data...\n");
    time_t nowtime;
	struct tm *timeinfo;

	time(&nowtime);
	timeinfo = localtime(&nowtime);
	sprintf(fileName,"%s%d%02d%02d-%02d-%02d-%02d.mp4","/",1900+timeinfo->tm_year,1+timeinfo->tm_mon,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	//char filePath[64] = "/mnt/sdcard/";
	//char filePath[64] = "./";
	strcat(dir,fileName);
    int iRet,fd;
	fd = open(dir,O_RDWR|O_CREAT|O_TRUNC,S_IRWXU|S_IRGRP|S_IROTH);
    close(fd);
	iRet = NET_DVR_SaveRealData(lRealPlayHandle,dir);
    if(!iRet)
    {
		printf("pyd1---NET_DVR_SaveRealData error:%s\n",NET_DVR_GetLastError());
        NET_DVR_StopSaveRealData(lRealPlayHandle);
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();  
        return HPR_ERROR;
	}
	//getchar();

#ifdef _WIN32
    Sleep(5000);  //millisecond
#elif  defined(__linux__) ||defined(__APPLE__)
    sleep(10);    //second
#endif
/*		
	while(saveFlag)
	{
		wait(NULL);
	}
	printf("save data success!\n");
    */
    //stop		
    NET_DVR_StopSaveRealData(lRealPlayHandle);
    //NET_DVR_StopRealPlay(lRealPlayHandle);
	//NET_DVR_Logout(lUserID);
	//NET_DVR_Logout_V30(lUserID);
	//cleanup
	//NET_DVR_Cleanup();
   
    return HPR_OK;
}

int Demo_Alarm()
{
//     if (Demo_AlarmListen() == HPR_ERROR)
//     {
//         return HPR_ERROR;
//     }

	if (Demo_AlarmFortify() == HPR_ERROR)
	{
		return HPR_ERROR;
	}

    return HPR_OK;
}

int Demo_AlarmFortify()
{
	//ÉèÖÃ±¨¾¯»Øµ÷º¯Êý
	NET_DVR_SetDVRMessageCallBack_V30(MessageCallback, NULL);
  
	//ÆôÓÃ²¼·À
	LONG lHandle;
	lHandle = NET_DVR_SetupAlarmChan_V30(lUserID);
	if (lHandle < 0)
	{
		printf("NET_DVR_SetupAlarmChan_V30 error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Logout(lUserID);
		NET_DVR_Cleanup(); 
		return HPR_ERROR;
	}
	
	saveFlag = 0;
	sleep(5000);
	//³·Ïú²¼·ÀÉÏ´«Í¨µÀ
	if (!NET_DVR_CloseAlarmChan_V30(lHandle))
	{
		printf("NET_DVR_CloseAlarmChan_V30 error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Logout(lUserID);
		NET_DVR_Cleanup(); 
		return HPR_ERROR;
	}
	saveFlag = 1;
	//×¢ÏúÉè±¸
	NET_DVR_Logout(lUserID);
	//ÊÍ·ÅSDK×ÊÔ´
	NET_DVR_Cleanup();
	return HPR_OK;
}

int capture()
{
	NET_DVR_JPEGPARA strPicPara = {0};
    strPicPara.wPicQuality = 2;
    strPicPara.wPicSize = 0;
    int iRet,fd;
    
    char dir[128]="/opt/homewell/video/";
    strcat(dir,GetIP());
    mk_dir(dir);
	//char file[64] = "/mnt/sdcard/temp.jpeg";
	char file[64] = "/temp.jpeg";
	strcat(dir,file);
    fd = open(dir,O_RDWR|O_TRUNC|O_CREAT|O_APPEND,S_IRWXU|S_IRGRP|S_IROTH);
	close(fd);
	printf("dir is %s\n",dir);
	iRet = NET_DVR_CaptureJPEGPicture(lUserID, struDevice.byStartChan, &strPicPara, dir);
    if (!iRet)
    {
        printf("pyd1---NET_DVR_CaptureJPEGPicture error, %d\n", NET_DVR_GetLastError());
        return HPR_ERROR;
    }
    
    printf("Get picture: temp.jpeg success!\n");
    
    return HPR_OK;
}

int record()
{
	//---------------------------------------
	//启动预览并设置回调数据流
	//LONG lRealPlayHandle;
	NET_DVR_CLIENTINFO ClientInfo = {0};
	ClientInfo.hPlayWnd = 0;         //需要SDK解码时句柄设为有效值，仅取流不解码时可设为空
	ClientInfo.lChannel     = 1;       //预览通道号
	ClientInfo.lLinkMode    = 0;       //最高位(31)为0表示主码流，为1表示子码流0～30位表示连接方式：0－TCP方式；1－UDP方式；2－多播方式；3－RTP方式;
	ClientInfo.sMultiCastIP = NULL;   //多播地址，需要多播预览时配置
  
	BOOL bPreviewBlock = false;       //请求码流过程是否阻塞，0：否，1：是
	//lRealPlayHandle = NET_DVR_RealPlay_V30(lUserID, &ClientInfo, NULL, NULL, 0);
	if (lRealPlayHandle < 0)
	{
		printf("NET_DVR_RealPlay_V30 error:%d\n",NET_DVR_GetLastError());
		NET_DVR_Logout(lUserID);
		NET_DVR_Cleanup();
		return 1;
	}
  
	char dir[128]="/opt/homewell/video/";
    strcat(dir,GetIP());
    mk_dir(dir);
    strcat(dir,fileName);
	printf("Start save...\n");
	int iRet,fd;	
	fd = open(dir,O_RDWR|O_CREAT|O_TRUNC|O_APPEND,S_IRWXU|S_IRGRP|S_IROTH);//先调用open创建之后，不许要root权限即可录制，避免文加大小为0
	close(fd);
	iRet = NET_DVR_SaveRealData(lRealPlayHandle,dir);
    if(!iRet)
    {
		printf("pyd1---NET_DVR_SaveRealData error:%s\n",NET_DVR_GetLastError());
        NET_DVR_StopSaveRealData(lRealPlayHandle);
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();  
        return HPR_ERROR;
	}
	printf("saving...\n");
	sleep(10);
	
	NET_DVR_StopSaveRealData(lRealPlayHandle);
	printf("save data success!\n");
	return HPR_OK;
}

char *GetIP()
{
	/*
    mqd_t mqd;  
    struct mq_attr attr;  
    char *ptr;  
    unsigned int prio;  
    size_t n;  
    int rc;  
	const char *file = "/getCameraIP";
     

    //只读模式打开消息队列
    mqd = mq_open(file, O_RDONLY);  
    if(mqd < 0)  
    {  
        perror("打开消息队列失败");  
        return NULL;   
    }     

    // 取得消息队列属性，根据mq_msgsize动态申请内存  
    rc = mq_getattr(mqd, &attr);  
    if(rc < 0)  
    {  
        perror("取得消息队列属性失败");  
        return NULL;   
    }  

    //动态申请保证能存放单条消息的内存 
    ptr = (char*)calloc(attr.mq_msgsize, sizeof(char));  
    if(NULL == ptr)  
    {  
        printf("动态申请内存失败\n");  
        mq_close(mqd);  
        return NULL;   
    }     
	printf("size %ld\n",attr.mq_msgsize);
    //*接收一条消息
    n = mq_receive(mqd, ptr, attr.mq_msgsize, &prio);  
    if(n < 0)  
    {  
        perror("读取失败");  
        mq_close(mqd);  
        free(ptr);  
        return NULL;   
    }  
    
    printf("received %s \n", ptr); 
    rc = mq_close(mqd);  
    if(0 != rc)  
    {  
        perror("关闭失败");  
        return NULL;  
    }    
    
    rc = mq_unlink(file);  
    if(0 != rc)  
    {  
        perror("关闭失败");  
        return NULL;  
    } 
    */
    char *ip,*file = "/tmp/cameraIP";
    int fd,len = 32;
    
    ip = (char*)calloc(len,sizeof(char));
    if(fd == -1)
    {
		perror("calloc");
		return NULL;
	}
	
    fd = open(file,O_RDONLY,0644);
    if(fd == -1)
    {
		perror("open");
		return NULL;
	}
	
	read(fd,ip,len);
	
	close(fd);
    return ip;  
}

int cgiMain(void)
{
	
	char *ip;
	ip = (char *) calloc(32, sizeof(char));
	//初始化
	NET_DVR_Init();

	NET_DVR_SetLogToFile(3,"./sdkLog");

	//设置链接时间与重连时间
	NET_DVR_SetConnectTime(2000,1);
	NET_DVR_SetReconnect(10000,true);

	//注册设备
	LONG lUserID;
	//NET_DVR_DEVICEINFO_V30 struDevice;
	strcpy(ip,GetIP());
	lUserID = NET_DVR_Login_V30(ip,8000,"admin","12345",&struDevice);
	if(lUserID < 0)
	{
		printf("Login in ERROR:%d\n",NET_DVR_GetLastError());
		NET_DVR_Cleanup();
		return 1;
	}
	printf("login success!\n");
	free(ip);
	
	//---------------------------------------
	//设置异常消息回调函数
	NET_DVR_SetExceptionCallBack_V30(0, NULL,g_ExceptionCallBack, NULL);
	
	NET_DVR_CLIENTINFO ClientInfo = {0};
#if (defined(_WIN32) || defined(_WIN_WCE)) || defined(__APPLE__)
    ClientInfo.hPlayWnd     = NULL;
#elif defined(__linux__)
   ClientInfo.hPlayWnd     = 0;
#endif

    ClientInfo.lChannel     = 1;  //channel NO.
    //ClientInfo.lLinkMode    = 0x40000000; //Record when breaking network.
    ClientInfo.lLinkMode    = 0;
    ClientInfo.sMultiCastIP = NULL;
    BOOL bPreviewBlock = false;       //请求码流过程是否阻塞，0：否，1：是
	lRealPlayHandle = NET_DVR_RealPlay_V30(lUserID, &ClientInfo,NULL,NULL,0);
	if (lRealPlayHandle < 0)
	{
		printf("NET_DVR_RealPlay_V30 error:%d\n",NET_DVR_GetLastError());
		NET_DVR_Logout(lUserID);
		NET_DVR_Cleanup();
		return 1;
	}
	
	
	cgiHeaderContentType("Text/html");
	//while(1){
		if(cgiFormSubmitClicked("reboot") == cgiFormSuccess)
		{
			TestRebootDVR(lUserID);
		}

		if(cgiFormSubmitClicked("Alarm") == cgiFormSuccess)
		{
			if(Demo_Alarm() == HPR_ERROR)
				goto EXIT;
		}
		
		if(cgiFormSubmitClicked("capture") == cgiFormSuccess)
		{
			if(capture() == HPR_ERROR)
				goto EXIT;
		}
		
		if(cgiFormSubmitClicked("record") == cgiFormSuccess)
		{
			if(record() == HPR_ERROR)
				goto EXIT;
		}
	//}
	
EXIT:
	NET_DVR_Logout_V30(lUserID);
	NET_DVR_Cleanup();

	return 0;
}

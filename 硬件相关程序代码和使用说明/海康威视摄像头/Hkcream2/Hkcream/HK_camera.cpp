#include "pch.h"

#include <mongoc.h>
#include <bson.h>
#include"HK_camera.h"
#include "PlayM4.h"
#include <windows.h>
#include <string>
#include <ctime>
#include <iostream>
#include <thread>

#pragma comment(lib,"../3rd_x64/mongo-c-driver/usr/lib/bson-1.0.lib")
#pragma comment(lib,"../3rd_x64/mongo-c-driver/usr/lib/mongoc-1.0.lib")

using namespace std;





//ȫ�ֱ���
LONG g_nPort;
Mat g_BGRImage;
LONG lRealHandle;

//���ݽ���ص�������
//���ܣ���YV_12��ʽ����Ƶ������ת��Ϊ�ɹ�opencv�����BGR���͵�ͼƬ���ݣ���ʵʱ��ʾ��
void CALLBACK DecCBFun(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nUser, long nReserved2)
{
	if (pFrameInfo->nType == T_YV12)
	{
		std::cout << "the frame infomation is T_YV12" << std::endl;
		if (g_BGRImage.empty())
		{
			g_BGRImage.create(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3);
		}
		Mat YUVImage(pFrameInfo->nHeight + pFrameInfo->nHeight / 2, pFrameInfo->nWidth, CV_8UC1, (unsigned char*)pBuf);

		cvtColor(YUVImage, g_BGRImage, COLOR_YUV2BGR_YV12);
		imshow("RGBImage1", g_BGRImage);
		waitKey(15);

		YUVImage.~Mat();
	}
}

//ʵʱ��Ƶ�������ݻ�ȡ �ص�����
void CALLBACK g_RealDataCallBack_V30(LONG lPlayHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void* pUser)
{
	if (dwDataType == NET_DVR_STREAMDATA)//��������
	{
		if (dwBufSize > 0 && g_nPort != -1)
		{
			if (!PlayM4_InputData(g_nPort, pBuffer, dwBufSize))
			{
				//std::cout << "fail input data" << std::endl;
			}
			else
			{
				std::cout << "success input data" << std::endl;
			}

		}
	}
}
//���캯��
HK_camera::HK_camera(void)
{

}
//��������
HK_camera::~HK_camera(void)
{
}
//��ʼ��������������ʼ��״̬���
bool HK_camera::Init()
{
	if (NET_DVR_Init())
	{
		return true;
	}
	else
	{
		return false;
	}
}

//��¼��������������ͷid�Լ����������¼
//bool HK_camera::Login(char* sDeviceAddress, char* sUserName, char* sPassword, WORD wPort)
bool HK_camera::Login(const char* sDeviceAddress, const char* sUserName, const char* sPassword, WORD wPort)       //��½��VS2017�汾��
{
	NET_DVR_USER_LOGIN_INFO pLoginInfo = { 0 };
	NET_DVR_DEVICEINFO_V40 lpDeviceInfo = { 0 };

	pLoginInfo.bUseAsynLogin = 0;     //ͬ����¼��ʽ
	strcpy_s(pLoginInfo.sDeviceAddress, sDeviceAddress);
	strcpy_s(pLoginInfo.sUserName, sUserName);
	strcpy_s(pLoginInfo.sPassword, sPassword);
	pLoginInfo.wPort = wPort;

	lUserID = NET_DVR_Login_V40(&pLoginInfo, &lpDeviceInfo);

	if (lUserID < 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

//��Ƶ����ʾ����
void HK_camera::show()
{
	if (PlayM4_GetPort(&g_nPort))            //��ȡ���ſ�ͨ����
	{
		if (PlayM4_SetStreamOpenMode(g_nPort, STREAME_REALTIME))      //������ģʽ
		{
			if (PlayM4_OpenStream(g_nPort, NULL, 0, 1024 * 1024))         //����
			{
				if (PlayM4_SetDecCallBackExMend(g_nPort, DecCBFun, NULL, 0, NULL))
				{
					if (PlayM4_Play(g_nPort, NULL))
					{
						std::cout << "success to set play mode" << std::endl;
					}
					else
					{
						std::cout << "fail to set play mode" << std::endl;
					}
				}
				else
				{
					std::cout << "fail to set dec callback " << std::endl;
				}
			}
			else
			{
				std::cout << "fail to open stream" << std::endl;
			}
		}
		else
		{
			std::cout << "fail to set stream open mode" << std::endl;
		}
	}
	else
	{
		std::cout << "fail to get port" << std::endl;
	}
	//����Ԥ�������ûص�������
	NET_DVR_PREVIEWINFO struPlayInfo = { 0 };
	struPlayInfo.hPlayWnd = NULL; //����Ϊ�գ��豸SDK������ֻȡ��
	struPlayInfo.lChannel = 1; //Channel number �豸ͨ��
	struPlayInfo.dwStreamType = 0;// �������ͣ�0-��������1-��������2-����3��3-����4, 4-����5,5-����6,7-����7,8-����8,9-����9,10-����10
	struPlayInfo.dwLinkMode = 0;// 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP 
	struPlayInfo.bBlocked = 1; //0-������ȡ��, 1-����ȡ��, �������SDK�ڲ�connectʧ�ܽ�����5s�ĳ�ʱ���ܹ�����,���ʺ�����ѯȡ������.

	if (NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, g_RealDataCallBack_V30, NULL))
	{
		namedWindow("RGBImage2");
	}
}

inline bool isexists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

string getlocaldata()
{
	mongoc_client_t *client;
	mongoc_collection_t *collection;
	//mongoc_collection_t  *col_status;
	bson_t *filter;
	bson_t *opts;
	mongoc_cursor_t *cursor;
	const bson_t *doc;
	char *str = NULL;
	string s2;


	mongoc_init();

	client = mongoc_client_new("mongodb://localhost:27017/");
	collection = mongoc_client_get_collection(client, "ugv", "ugv_status_test");

	filter = BCON_NEW();
	opts = BCON_NEW("limit", BCON_INT64(1), "sort", "{", "_id", BCON_INT32(-1), "}");
	cursor = mongoc_collection_find_with_opts(collection, filter, opts, NULL);

	while (mongoc_cursor_next(cursor, &doc)) {
		str = bson_as_json(doc, NULL);
		s2 = str;
		//printf("%s\n", str);
		//cout << s2.substr(72, 21) << endl;
		//cout << s2.substr(95, 22) << endl;
		bson_free(str);
	}

	bson_destroy(filter);
	bson_destroy(opts);
	mongoc_cursor_destroy(cursor);
	mongoc_collection_destroy(collection);
	mongoc_client_destroy(client);
	mongoc_cleanup();


	return s2.substr(81, 10) + "-" + s2.substr(104, 12);

}





void HK_camera::Record()
{



	char videoname[256];
	char localname[64];
	time_t currTime;
	struct tm *mt;
	string localdata;

	NET_DVR_PREVIEWINFO struPlayInfo = { 0 };
	struPlayInfo.hPlayWnd = NULL; //����Ϊ�գ��豸SDK������ֻȡ��
	struPlayInfo.lChannel = 1; //Channel number �豸ͨ��
	struPlayInfo.dwStreamType = 0;// �������ͣ�0-��������1-��������2-����3��3-����4, 4-����5,5-����6,7-����7,8-����8,9-����9,10-����10
	struPlayInfo.dwLinkMode = 0;// 0��TCP��ʽ,1��UDP��ʽ,2���ಥ��ʽ,3 - RTP��ʽ��4-RTP/RTSP,5-RSTP/HTTP 
	struPlayInfo.bBlocked = 1; //0-������ȡ��, 1-����ȡ��, �������SDK�ڲ�connectʧ�ܽ�����5s�ĳ�ʱ���ܹ�����,���ʺ�����ѯȡ������.
	
	

	while(true) {
		currTime = time(NULL);
		mt = localtime(&currTime);
		/*�������������ļ���*/
		lRealHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, g_RealDataCallBack_V30, NULL);
		localdata = "A-" + getlocaldata();
		cout << getlocaldata() << endl;
		for (int i = 0; i < localdata.length(); i++) 
		{
			localname[i] = localdata[i];
		}
		cout << localname << endl;
		
		/*sprintf_s(videoname, "C:\\Users\\Administrator.SC-201905221819\\Desktop\\vidio\\%s-%d-%02d-%02d-%02d-%02d-%02d.mp4", \
			localdata,mt->tm_year + 1900, mt->tm_mon + 1, mt->tm_mday, mt->tm_hour, mt->tm_min, mt->tm_sec);*/

		sprintf_s(videoname, "C:\\Users\\Administrator.SC-201905221819\\Desktop\\vidio\\%s-%d-%02d-%02d-%02d-%02d-%02d.mp4", \
			localname,mt->tm_year + 1900, mt->tm_mon + 1, mt->tm_mday, mt->tm_hour, mt->tm_min, mt->tm_sec);

		NET_DVR_SaveRealData(lRealHandle, videoname);
		cout << "��ʼ¼�ƣ��ļ�λ�ã�" << videoname << endl;
		for (int j = 0; j < 6; j++) 
		{
			Sleep(10000);
		}
		
		NET_DVR_StopSaveRealData(lRealHandle);
		if (isexists(videoname))
		{
			cout << "��Ƶ¼�Ƴɹ�" << endl;
		}
		else
		{
			cout << "��Ƶ¼��ʧ��" << endl;
		}
		
	}
	
  
}

void HK_camera::StartRecordThread()
{
	RecordThread = thread(&HK_camera::Record, this);
	RecordThread.detach();
}




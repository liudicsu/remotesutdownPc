
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "iphlpapi.h"
#pragma comment(lib, "iphlpapi.lib")
#pragma comment( lib, "ws2_32.lib" ) 
#define  FILE_PATH "D:\\remote.txt"
#define SLEEP_TIME 5000
#define PORT 8929
bool brun=false;
SERVICE_STATUS servicestatus;
SERVICE_STATUS_HANDLE hstatus;
SOCKET        s;                                   //2
int WriteToLog(char* str);
int WriteToLog(char* str)
{
    FILE* pfile;
    fopen_s(&pfile,FILE_PATH,"a+");
    if (pfile==NULL)
    {
        return -1;
    }
    fprintf_s(pfile,"%s\n",str);
    fclose(pfile);
    return 0;
}
void WINAPI ServiceMain(int argc, char** argv);
void WINAPI CtrlHandler(DWORD request);
int loop();
void shutdown()
{
	HANDLE ToHandle;
   TOKEN_PRIVILEGES tkp;
   //打开本进程访问信令
   if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&ToHandle))
   {
    //修改本进程权限
    LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
    tkp.PrivilegeCount=1;
    tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
    //通知系统已修改
    AdjustTokenPrivileges(ToHandle,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);
	WriteToLog("shuting down\n");
    //获得权限后关闭计算机,要实现注销或重启则对应EWX_LOGOFF,EWX_REBOOT
    ExitWindowsEx(EWX_SHUTDOWN|EWX_FORCE,0);
   }
}
void WINAPI ServiceMain(int argc, char** argv)
{
    servicestatus.dwServiceType = SERVICE_WIN32;
    servicestatus.dwCurrentState = SERVICE_START_PENDING;
    servicestatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_STOP;//在本例中只接受系统关机和停止服务两种控制命令
    servicestatus.dwWin32ExitCode = 0;
    servicestatus.dwServiceSpecificExitCode = 0;
    servicestatus.dwCheckPoint = 0;
    servicestatus.dwWaitHint = 0;
    hstatus = ::RegisterServiceCtrlHandler(L"remoteShutdown", CtrlHandler);
    if (hstatus==0)
    {
        WriteToLog("RegisterServiceCtrlHandler failed");
        return;
    }
    WriteToLog("RegisterServiceCtrlHandler success");
    servicestatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus (hstatus, &servicestatus);
	WriteToLog("start loop\n");
	loop();
	WriteToLog("out of  loop\n");
	servicestatus.dwCurrentState = SERVICE_STOPPED;
}
void WINAPI CtrlHandler(DWORD request)
{
    switch (request)
    {
        case SERVICE_CONTROL_STOP:
			closesocket(s);
			servicestatus.dwCurrentState = SERVICE_ACCEPT_STOP;
            break;
        case SERVICE_CONTROL_SHUTDOWN:
			closesocket(s);
			servicestatus.dwCurrentState = SERVICE_ACCEPT_SHUTDOWN;
            break;
        default:
            break;
    }
    SetServiceStatus (hstatus, &servicestatus);
}
void printfMac(unsigned char * mac,char *macstr)
{
	macstr[13] = '\0';
	for(int i = 0;i<6;++i)
	{
		if((mac[i] >>4) <10)
		{
			macstr[2*i] =  (mac[i] >>4)+'0';
		}else
		{
			macstr[2*i] =  (mac[i] >>4)+'A'-10;
		}
		if((mac[i] & 0x0F) <10)
		{
			macstr[2*i+1] =  (mac[i] & 0x0F)+'0';
		}else
		{
			macstr[2*i+1] =  (mac[i] & 0x0F)+'A'-10;
		}
	}
	//printf("%s\n",macstr);
}
BOOL GetMacAddress(char *mac)
{
    PIP_ADAPTER_INFO pAdapterInfo;  
    DWORD AdapterInfoSize;  
    TCHAR szMac[32] = {0};  
    DWORD Err;  
     
    AdapterInfoSize = 0;  
    Err = GetAdaptersInfo(NULL, &AdapterInfoSize);  
     
    if((Err != 0) && (Err != ERROR_BUFFER_OVERFLOW))
        {  
        WriteToLog("获得网卡信息失败！");  
        return   FALSE;  
    }  
     
    //   分配网卡信息内存  
    pAdapterInfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, AdapterInfoSize);  
    if(pAdapterInfo == NULL)
        {  
       WriteToLog("分配网卡信息内存失败");  
        return   FALSE;  
    }  
     
    if(GetAdaptersInfo(pAdapterInfo, &AdapterInfoSize) != 0)
        {  
       WriteToLog("获得网卡信息失败！\n");  
        GlobalFree(pAdapterInfo);  
        return   FALSE;  
    }
	   mac[0]=pAdapterInfo->Address[0];
       mac[1]=pAdapterInfo->Address[1]; 
       mac[2]=pAdapterInfo->Address[2];  
       mac[3]=pAdapterInfo->Address[3];  
       mac[4]=pAdapterInfo->Address[4];  
       mac[5]=pAdapterInfo->Address[5];
    GlobalFree(pAdapterInfo);  
    return   TRUE;  
 
}
int loop()
{
	WSADATA       wsd;                                              //2
	if (WSAStartup(MAKEWORD(2,2), &wsd) != 0)
	{
		WriteToLog("Failed to load Winsock!\n");
		return 1;
	}
	char mac[7] ={"\0"};
	char macstr[13] = {'\0'};
	char macstr1[13] = {'\0'};
	if(!GetMacAddress(mac))
	{
		return 0;
	}
	printfMac((unsigned char*)mac,macstr);
	//printf("%ud-%ud-%2X-%ud-%ud-%ud\n",mac[0],mac[1],(unsigned char)mac[2],mac[3],mac[4],mac[5]);
	
	int          ret;
	struct sockaddr_in local,sender;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if(s == INVALID_SOCKET)
		return 1;
	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	int a = 0;
	if ((a = bind(s, (struct sockaddr *)&local,sizeof(sockaddr_in))) == SOCKET_ERROR)
	{
		//printf("bind() failed: %d\n", WSAGetLastError());
		closesocket(s);
		WSACleanup();
		return 1;
	}
	char recvbuf[1024] = {'\0'};
	char *pmac = recvbuf +6;
	int dwSenderSize = sizeof(sockaddr_in);   //3
	while(1)
	{
	ret = recvfrom(s, recvbuf, 1024, 0, (SOCKADDR *)&sender, &dwSenderSize);
	if (ret == SOCKET_ERROR)
	{
		WriteToLog("sokect error or service stop\n");
		closesocket(s);
		WSACleanup();
		break;
	}
	else if (ret == 0)
	{
		continue;

	}
	else if(ret !=102)
	{

		continue;
	}else
	{
		recvbuf[13] = '\0';
		printfMac((unsigned char*)pmac,macstr1);
		//printf("%ud-%ud-%ud-%ud-%ud-%ud\n",pmac[0],pmac[1],(unsigned char)pmac[2],pmac[3],pmac[4],pmac[5]);
		if(strcmp(macstr,macstr1) ==0)
		{
			WriteToLog("get the shut down instructon\n");
			shutdown();

		}else
		{
			continue;
		}
	}
	}
	WriteToLog("going out of loop\n");
    closesocket(s);
	WSACleanup();
	return 0;
}
void main()
{
	SERVICE_TABLE_ENTRY entrytable[2];
    entrytable[0].lpServiceName = L"remoteShutdown";
    entrytable[0].lpServiceProc=(LPSERVICE_MAIN_FUNCTION)ServiceMain;
    entrytable[1].lpServiceName=NULL;
    entrytable[1].lpServiceProc=NULL;
    StartServiceCtrlDispatcher(entrytable);
}

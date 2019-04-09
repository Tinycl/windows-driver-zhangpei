/*++
Abstract: 
 A Simple ASIO Application
 Based on ASIO SDK 2.0 Sample

Copyright (c) Moore.Zhang 09/21/2010

Contact: 
 zhang.mibox@gmail.com

Last modified: 
1 2/01/2010
*/

#include "stdafx.h"
#include <conio.h>
#include <assert.h>
#include "command.h"
#include "AsioTool.h"
#include "playmusicfile.h"

#define array_size(array) (sizeof(array)/sizeof((array)[0]))

// ������һ�ű�Ķ���
typedef bool (* pcmdEntry)(char * par);

typedef struct
{
	char * name;
	pcmdEntry handler;
} CMDTable;


CMDTable gCommandTable[]=
{
	{"?", help},
	{"h", help},

	{"p", play},
	{"r", record},
	{"t", stop},
	{"q", quit},
	
	{"s", setASIOPar}
};

int AvailableSampleRates[] = 
{
	44100,
	48000,
	88200,
	96000
};

char g_cmdBuf[128] = {0};
char* g_cmdName = NULL;
char* g_cmdPar = NULL;

void InitializeCommandVaribles()
{
	HANDLE std_out;
	BOOL ret;
	COORD console_size ={120, 9999};
	SMALL_RECT console_window = {0,0, 120, 26};
	CONSOLE_SCREEN_BUFFER_INFO screen_buffer_info;

	ret = SetConsoleTitle("ASIO��¼�������̨�� 1.0  2010-03-03  ����: ����");
	std_out = GetStdHandle(STD_OUTPUT_HANDLE);
	ret = GetConsoleScreenBufferInfo(std_out, &screen_buffer_info);
	ret = SetConsoleScreenBufferSize(std_out, console_size);
	ret = SetConsoleWindowInfo(std_out, TRUE, &console_window);
	printf("cmd>");
}

//
// ���ܲ������û�����
void CommandProcess()
{
	int i;

	__try{

		// ȡ���û�����
		if(!_kbhit() || gets_s(g_cmdBuf, sizeof(g_cmdBuf)) == NULL)
		{
			Sleep(100);
			__leave;
		}

		// ��������
		if(!pars_cmd(g_cmdBuf, &g_cmdName, &g_cmdPar))
		{
			if(g_cmdName)
				printf("invalid command: %s\
					   \ncmd>", 
					   g_cmdBuf);
			else
				printf("cmd>");
			__leave;
		}

		for(i=0; i<array_size(gCommandTable); i++)
		{
			if(strcmp(gCommandTable[i].name, g_cmdName) == 0)
			{
				if(gCommandTable[i].handler)
					gCommandTable[i].handler(g_cmdPar);
				break;
			}
		}
	
		if(i == array_size(gCommandTable))
			printf("invalid command: %s\n", g_cmdBuf);
		else
			printf("cmd>");

	}
	__finally
	{

	}
}

//
// �������
// ���û��������ݽ����ɣ�"���� + ����1 + ����2..." ����ʽ
bool pars_cmd(char * g_cmdBuf, char ** g_cmdName, char **g_cmdPar)
{
	if(g_cmdBuf == NULL || g_cmdName == NULL || g_cmdPar == NULL)
		return false;

	// ��дת��
	char* chPnt = g_cmdBuf;
	while(*chPnt != NULL)
	{
		if(*chPnt >= 'A' && *chPnt <= 'Z')
			*chPnt = *chPnt -  'A' + 'a';
		chPnt++;
	}

	// �����Ʊ��
	while(*g_cmdBuf && (*g_cmdBuf == ' ' || *g_cmdBuf == '\t' || *g_cmdBuf == '\n' || *g_cmdBuf == '\r'))
	{
		g_cmdBuf ++;
	}

	// 
	// �����������ַ���

	// ���ַ�������alphabetic character
	if(!isalpha(*g_cmdBuf))
	{
		*g_cmdName = NULL;
		return false;
	}

	*g_cmdName = g_cmdBuf++;
	while(*g_cmdBuf && *g_cmdBuf != ' ' && *g_cmdBuf != '\t' && *g_cmdBuf != '\n' && *g_cmdBuf != '\r')
	{
		if(!isalpha(*g_cmdBuf) && !isdigit(*g_cmdBuf))
		{
			*g_cmdName = NULL;
			return false;
		}
		g_cmdBuf ++;
	}

	if(*g_cmdBuf)
	{
		*g_cmdBuf ++ = 0;
	}
	else
	{
		*g_cmdPar = NULL;
		return true;
	}

	//
	// ʣ�µ��ǲ����ַ���

	while(*g_cmdBuf && (*g_cmdBuf == ' ' || *g_cmdBuf == '\t' || *g_cmdBuf == '\n' || *g_cmdBuf == '\r'))
	{
		g_cmdBuf ++;
	}

	if(*g_cmdBuf)
	{
		*g_cmdPar = g_cmdBuf;
	}
	else
	{
		*g_cmdPar = NULL;
	}

	return true;
}

//
// �˳�����
bool quit(char* par)
{
	closeWaveFile();
	EndRecord();
	stop(NULL);// �������عأ��ٴ��ڹع�
	g_AsioInfo.bMainQuit = TRUE;
	return true;
}

//
// ֹͣ����/¼��
// par: ֹͣ�����������ѡ��ֹͣ���š�ֹͣ¼��
bool stop(char* par)
{
	BOOL bClearASIOBuf = g_AsioInfo.bAsioStarted;

	if(par != NULL)
	{
		while(*par && (*par == ' ' || *par == '\t' || *par == '\n' || *par == '\r'))
			par++;

		if(*par == 'r' || *par == 'R'){
			g_AsioInfo.bAudioRecord = FALSE;
		}
		else if(*par == 'p' || *par == 'P'){
			g_AsioInfo.bAsioStarted = FALSE;
		}
		else{
			fprintf(stdout, "invalid parameter: %s\
							\n>cmd",
							par
							);
			return false;
		}
	}
	else
	{
		g_AsioInfo.bAudioRecord = FALSE;
		g_AsioInfo.bAsioStarted = FALSE;
	}

	// �رմ򿪵��ļ����
	if(!g_AsioInfo.bAsioStarted && g_hWaveFile)
	{
		CloseHandle(g_hWaveFile);
		g_hWaveFile = NULL;
	}

	if(!g_AsioInfo.bAudioRecord && g_hWaveFileRecord)
	{
		recordWaveFileEnd(g_hWaveFileRecord);
		g_hWaveFileRecord = NULL;
	}

	// ֹͣ������
	if(!g_AsioInfo.bAudioRecord && !g_AsioInfo.bAsioStarted)
		g_pASIODrv->stop();


	// ������������
	if(bClearASIOBuf && !g_AsioInfo.bAsioStarted)
	{		
		for (int i = 0; i < g_AsioInfo.inputBuffers + g_AsioInfo.outputBuffers; i++)
		{
			if ( g_AsioInfo.bufferInfos[i].isInput == false){
				memset(g_AsioInfo.bufferInfos[i].buffers[0], 0, g_AsioInfo.preferredSize*4);
				memset(g_AsioInfo.bufferInfos[i].buffers[1], 0, g_AsioInfo.preferredSize*4);
			}
		}
	}

	return true;
}


bool play(char* par)
{
	if(g_AsioInfo.bAsioStarted) 
	{
		fprintf(stdout, "on running!");
		return false;
	}

	int nRet = -1;
	if(par == NULL){
		fprintf(stdout, "need parameter.\n");
		return false;
	}

	// ��������
	if(*par == 'f' || *par == 'F') 
	{
		int nOpenFileRet = 0;		

		// ȡ���ļ���
		char* filename = par+1;
		while(*filename && 
			(*filename == ' ' || 
			*filename == '\t' || 
			*filename == '\n' || 
			*filename == '\r'))
		{
			filename ++;
		}

		if(*filename == NULL)
		{
			fprintf(stdout, "not found music file name\n");
		}
		else if(nOpenFileRet = playWaveFile(filename))
		{
			if(nOpenFileRet == 1)
				fprintf(stdout, "invalid file name:%s\n", filename);
			else
				fprintf(stdout, "failed to open the music file: %s(%d)\n", filename, nOpenFileRet);
		}
		else 
		{
			nRet = ASE_OK;
		}
	}
	else
	{
		printf("invalid parameter: %s\n", par);
		return false;
	}

	if (nRet == ASE_OK && 
		(g_AsioInfo.bAudioRecord || 
		g_pASIODrv->start() == ASE_OK))
	{
		g_AsioInfo.bAsioStarted = TRUE; // ȫ�ֱ����������ˡ�
		fprintf (stdout, "ASIO Driver started successfully.\n");
	}
	else
	{
		fprintf (stdout, "ASIO Driver failed to start.\n");
		return false;
	}

	return true;
}


bool record(char* par)
{
	bool bRet = false;

	if(par == NULL)
	{
		fprintf(stdout, "no parameter.\n");
		return false;
	}

	__try{

		if(g_AsioInfo.bAudioRecord) 
			__leave;

		int nOpenFileRet = 0;

		// ȡ���ļ���
		char* filename = par;
		while(*filename && 
			(*filename == ' ' || 
			*filename == '\t' || 
			*filename == '\n' ||
			*filename == '\r'))
		{
			filename ++;
		}

		if(*filename == NULL)
		{
			fprintf(stdout, "not found music file name\n");
		}
		else if(nOpenFileRet = BeginRecord(filename))
		{// ��дһ��
			if(nOpenFileRet == 1)
				fprintf(stdout, "invalid file name:%s\n", filename);
			else
				fprintf(stdout, "failed to open the music file: %s(%d)\n", filename, nOpenFileRet);
		}
		else 
		{
			bRet = true;
		}

		if (bRet){
			if(g_AsioInfo.bAsioStarted || 
				g_pASIODrv->start() == ASE_OK)
			{
				g_AsioInfo.bAudioRecord = TRUE;
				fprintf (stdout, "\nASIO Driver started successfully.\n\n");
			}
			else 
			{
				bRet = false;
			}
		}

		if(!bRet)
		{
			fprintf (stdout, "\nASIO Driver failed to start.\n\n");
		}

	}
	__finally{
	}

	return bRet;
}

bool help(char* )
{
	printf("����: p [f|r|s|w] [filename]\
		   \n¼��:r filename\
		   \nֹͣ:t [p|r]\
		   \n�˳�:q\
		   \n����:h | ?\
		   \n����: s rate [sample rate]\n");

	return true;
}

// ����ASIO����
// Ŀǰֻ֧�����ò�����
bool setASIOPar(char* par)
{
	if (NULL == par)
	{
		printf("No set command is input.\n");
		return false;
	}

	// ��������
	int nRet = -1;
	char* tcsCmd = par;
	char* tcsCmdPnt;
	while(*tcsCmd && 
		(*tcsCmd == ' ' || 
		*tcsCmd == '\t' || 
		*tcsCmd == '\n' || 
		*tcsCmd == '\r'))
	{
		tcsCmd ++;
	}

	tcsCmdPnt = tcsCmd;

	while(*tcsCmdPnt && 
		*tcsCmdPnt != ' ' 
		&& *tcsCmdPnt != '\t' 
		&& *tcsCmdPnt != '\n' 
		&& *tcsCmdPnt != '\r')
	{
		if(!isalpha(*tcsCmdPnt) &&
			!isdigit(*tcsCmdPnt))
		{
			printf("Invalid parameter:%s\n", par);
			return false;
		}

		tcsCmdPnt ++;
	}

	if(*tcsCmdPnt)
	{
		*tcsCmdPnt++ = 0;

		for(int i = 0; ; i++)
		{
			if(tcsCmd[i] == 0) 
				break;

			if(tcsCmd[i] >= 'A' && 
				tcsCmd[i] <= 'Z')
			{
				tcsCmd[i] -= 'A';
				tcsCmd[i] += 'a';
			}
		}

		if(!strncmp(tcsCmd, "rate", 10))
		{// samplerate
			char* tcsCmdPar = tcsCmdPnt;
			char* tcsCmdParPnt = tcsCmdPnt;
			while(*tcsCmdParPnt && 
				(*tcsCmdParPnt == ' ' ||
				*tcsCmdParPnt == '\t' ||
				*tcsCmdParPnt == '\n' ||
				*tcsCmdParPnt == '\r'))
			{
				tcsCmdParPnt ++;
			}

			while(*tcsCmdParPnt &&
				*tcsCmdParPnt != ' ' && 
				*tcsCmdParPnt != '\t' && 
				*tcsCmdParPnt != '\n' && \
				*tcsCmdParPnt != '\r')
			{
				if(!isdigit(*tcsCmdParPnt))
				{
					printf("Invalid sample rate value:%s\n", tcsCmdPar);
					return false;
				}
				tcsCmdParPnt++;
			}

			if(*tcsCmdParPnt)
				*tcsCmdParPnt = 0;

			int i;
			int nSampleRate = atoi(tcsCmdPar);

			for(i = 0; i < array_size(AvailableSampleRates); i++)
			{
				if(nSampleRate == AvailableSampleRates[i])
					break;
			}

			if(i >= array_size(AvailableSampleRates)){
				printf("Invalid sample rate value:%d.(44100, 48000)\n", nSampleRate);
				return false;
			}else{
				nRet = g_pASIODrv->setSampleRate(nSampleRate);
				if(nRet != ASE_OK){
					printf("failed to set sameplerate:%d. Check if the S/PDIF input has been open.\n", nSampleRate);
				}else{
					printf("current sameplerate is:%d\n", nSampleRate);
				}
			}
		}else{// ����
			printf("Invalid set command parameter:%s\n", par);
		}
	}

	if(nRet == ASE_OK)
		return true;
	else
		return false;
}
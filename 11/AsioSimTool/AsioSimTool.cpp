/*++
Abstract: 
 A Simple ASIO Application
 Based on ASIO SDK 2.0 Sample

Copyright (c) Moore.Zhang 09/21/2010
 
Contact: 
 zhang.mibox@gmail.com

Last modified: 
 12/01/2010
*/

#include "stdafx.h"
#include <Windows.h>
#include <Mmsystem.h>
#include <assert.h>
#include "asiotool.h"
#include "command.h"
#include "playmusicfile.h"
#include "asiolist.h"

// �����ĸ�Message�ص�
ASIOTime *bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow);
void bufferSwitch(long index, ASIOBool processNow);
void sampleRateChanged(ASIOSampleRate sRate);
long asioMessages(long selector, long value, void* message, double* opt);


IASIO*		g_pASIODrv = NULL;
DriverInfo  g_AsioInfo;

int _tmain(int argc, _TCHAR* argv[])
{
	ASIOError result;
	char strErr[128];
	ASIOCallbacks asioCallbacks;

	__try
	{
		//
		// ��ȡASIO�����ӿ����ʵ�������ǵ�һ������������ɹ����ܽ�������ĳ�ʼ������
		//

		g_pASIODrv = loadDriver(argv[1]);
		if(g_pASIODrv == NULL)
		{
			printf("�Ҳ���ASIO�豸\n");
			__leave;
		}

		// 
		// ��ʼ��
		//

		result = g_pASIODrv->init(NULL);
		if(ASIOTrue != result)
		{
			g_pASIODrv->getErrorMessage(strErr); // ��ȡ������Ϣ
			printf("����init����ʧ��:%d %s", result, strErr);
			__leave;
		}

		// ��ȡASIO��������
		g_pASIODrv->getDriverName(g_AsioInfo.driverInfo.name);
		g_AsioInfo.driverInfo.driverVersion = g_pASIODrv->getDriverVersion();
		printf ("\n����: %s\n�汾: %d\n", 
			g_AsioInfo.driverInfo.name,
			g_AsioInfo.driverInfo.driverVersion
			);

		// ��ȡ������Ϣ
		result = g_pASIODrv->getBufferSize(&g_AsioInfo.minSize, &g_AsioInfo.maxSize, 
			&g_AsioInfo.preferredSize, &g_AsioInfo.granularity);
		if(result != ASE_OK)
		{
			g_pASIODrv->getErrorMessage(strErr); // ��ȡ������Ϣ
			printf("����getBufferSize����ʧ��:%d %s", result, strErr);
			__leave;
		}

		// ��ȡ��ǰ����Ƶ��
		result = g_pASIODrv->getSampleRate(&g_AsioInfo.sampleRate);
		if(result != ASE_OK)
		{
			g_pASIODrv->getErrorMessage(strErr); // ��ȡ������Ϣ
			printf("����getSampleRate����ʧ��:%d %s", result, strErr);
			__leave;
		}
		
		// �жϻ�ȡ�ĵ�ǰ����Ƶ��ֵ�Ƿ�����Ч����
		if (g_AsioInfo.sampleRate <= 0.0 || g_AsioInfo.sampleRate > 96000.0)
		{
			result = g_pASIODrv->setSampleRate(44100.0);
			if(ASE_OK != result)
			{
				printf("��֧��Ĭ�ϲ���Ƶ�ʣ�%d", result);
				__leave;
			}

			result = g_pASIODrv->getSampleRate(&g_AsioInfo.sampleRate);
			if(result != ASE_OK)
			{
				g_pASIODrv->getErrorMessage(strErr); // ��ȡ������Ϣ
				printf("����getSampleRate����ʧ��:%d %s", result, strErr);
				__leave;
			}
		}
		
		// ��ȡ����/���������
		result = g_pASIODrv->getChannels(&g_AsioInfo.inputChannels, &g_AsioInfo.outputChannels);
		if(result != ASE_OK)
		{
			g_pASIODrv->getErrorMessage(strErr); // ��ȡ������Ϣ
			printf("����getChannels����ʧ��:%d %s", result, strErr);
			__leave;
		}
		
		// ���ÿ����������ʼ��ASIO����
		//
		ASIOBufferInfo *info = g_AsioInfo.bufferInfos;

		// input
		if (g_AsioInfo.inputChannels > kMaxInputChannels)
			g_AsioInfo.inputBuffers = kMaxInputChannels;
		else
			g_AsioInfo.inputBuffers = g_AsioInfo.inputChannels;

		for(int i = 0; i < g_AsioInfo.inputBuffers; i++, info++)
		{
			info->isInput = ASIOTrue;
			info->channelNum = i;
			info->buffers[0] = info->buffers[1] = 0;
		}

		// outputs
		if (g_AsioInfo.outputChannels > kMaxOutputChannels)
			g_AsioInfo.outputBuffers = kMaxOutputChannels;
		else
			g_AsioInfo.outputBuffers = g_AsioInfo.outputChannels;

		for(int i = 0; i < g_AsioInfo.outputBuffers; i++, info++)
		{
			info->isInput = ASIOFalse;
			info->channelNum = i;
			info->buffers[0] = info->buffers[1] = 0;
		}

		// ��ʼ���ص��������顣��Щ������������Ƶ�����Լ��ṩ����ASIO�������á�
		// bufferSwitch��ʵ�����ֲ��ŵĹؼ����˴����������ļ����ݣ����ܲ��ų���Ӧ�������ˡ�
		// sampleRateChanged���豸�����ʸı�ʱ�����ã������Ѳ������������ʾ��Ϣ��
		// bufferSwitch��bufferSwitchTimeInfo�����ص�������ASIO��������Ҫ������Ƶ���ݵ�ʱ��������ǡ�
		// ������ǰ�ߵ������棬��ASIO�����ڵ��õ�ʱ��ᴫ��һ��ʱ����Ϣ�ṹָ�룬������Ƶ�����ͬ������λ�Ȳ�����
		// asioMessages�ص�������ASIO�������õ��˽���Ƶ����ġ�������������������֧�ֵ�ASIO�汾����1.0����2.0�ȡ�
		asioCallbacks.bufferSwitch = &bufferSwitch;
		asioCallbacks.sampleRateDidChange = &sampleRateChanged;
		asioCallbacks.asioMessage = &asioMessages;
		asioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;

		// ������������ע��ص�����
		result = g_pASIODrv->createBuffers(g_AsioInfo.bufferInfos,
			g_AsioInfo.inputBuffers + g_AsioInfo.outputBuffers,
			g_AsioInfo.preferredSize, 
			&asioCallbacks
			);

		if(result != ASE_OK)
		{
			g_pASIODrv->getErrorMessage(strErr); 
			printf("����createBuffers����ʧ��:%d %s", result, strErr);
			__leave;
		}
	
		// ����createBuffers�󣬿ɻ�ȡ�豸����������ӳ١�
		// �ӳ���һ�����ڵģ����ǲ���ϵͳ�ܹ��쵽�յ�һ���ֽڣ����̲���һ���ֽڵ��ٶȡ�
		// ��Ȼ�л������������Ⱥ󡢵ȴ����ȴ���ʱ������ӳ١�
		// �����ӳ٣�һ����Ƶ�����ӱ��豸��ȡ����������Ƶ�������������ʱ��
		// ����ӳ٣�һ���������ݴ���Ƶ������ȥ��ֱ�������������ų������ڼ���������ʱ��
		g_pASIODrv->getLatencies(&g_AsioInfo.inputLatency, &g_AsioInfo.outputLatency);

		// ��ȡ��������ϸ��Ϣ
		printf("������Ϣ...\n\n����������%d��\t���������%d��\n", g_AsioInfo.inputBuffers, g_AsioInfo.outputBuffers);
		for (int i = 0; i < g_AsioInfo.inputBuffers + g_AsioInfo.outputBuffers; i++)
		{
			g_AsioInfo.channelInfos[i].channel = g_AsioInfo.bufferInfos[i].channelNum;
			g_AsioInfo.channelInfos[i].isInput = g_AsioInfo.bufferInfos[i].isInput;
			result = g_pASIODrv->getChannelInfo(&g_AsioInfo.channelInfos[i]);

			if (result == ASE_OK)
				printf("%d %s����%d:%s group: %d type: %d\n", 
					i, g_AsioInfo.channelInfos[i].isInput ? "����":"���",
					g_AsioInfo.channelInfos[i].channel,
					g_AsioInfo.channelInfos[i].name,
					g_AsioInfo.channelInfos[i].channelGroup,
					g_AsioInfo.channelInfos[i].type);
		}

		printf("\n��Ƶ��Ϣ...\n\n");
		printf( "������:%d\n", g_AsioInfo.sampleRate);
		printf( "�����ӳ�:%d, ����ӳ�:%d\n", 
			g_AsioInfo.inputLatency, g_AsioInfo.outputLatency);
		printf( "��󻺳�:%d, ��С����:%d, ��ǰ����:%d, ��������:%d\n",
			g_AsioInfo.maxSize, g_AsioInfo.minSize, 
			g_AsioInfo.preferredSize, g_AsioInfo.granularity);

		// ����ASIO�����Ƿ�֧��outputReady�ӿڡ�
		// ���ASIO������֧�֣�����SDK������˵��������ֵ��ΪASE_NotPresent��
		// ��������£����ǳ�������Ͳ�Ӧ������outputReady��ASIO������������ͬ����
		if(ASE_OK == g_pASIODrv->outputReady())
			g_AsioInfo.postOutput = TRUE;
		else
			g_AsioInfo.postOutput = FALSE;

		InitializeCommandVaribles();
		
		// ����߳����ȼ�
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

		// �ȴ��������û����ֱ���˳���
		while (!g_AsioInfo.bMainQuit)
		{
			CommandProcess();
		}		
	}
	__finally
	{
		if(g_pASIODrv)
		{
			g_pASIODrv->disposeBuffers();
			unloadASIO();
			g_pASIODrv = 0;
		}

		if(g_pDataBuf)
			delete g_pDataBuf;
	}
	

	return 0;
}


ASIOTime *bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow)
{
	// ����ʱ����Ϣ��ASIO��ʱ���Ϊ64λ���ݡ�
	// ASIO֧�ֶ���ʱ����ĸ�ʽ��ϵͳʱ�䣨ʱ���룩������λ�á�TCʱ����
	g_AsioInfo.tInfo = *timeInfo;

	if (timeInfo->timeInfo.flags & kSystemTimeValid)
		g_AsioInfo.nanoSeconds = ASIO64toDouble(timeInfo->timeInfo.systemTime);
	else
		g_AsioInfo.nanoSeconds = 0;

	if (timeInfo->timeInfo.flags & kSamplePositionValid)
		g_AsioInfo.samples = ASIO64toDouble(timeInfo->timeInfo.samplePosition);
	else
		g_AsioInfo.samples = 0;

	if (timeInfo->timeCode.flags & kTcValid)
		g_AsioInfo.tcSamples = ASIO64toDouble(timeInfo->timeCode.timeCodeSamples);
	else
		g_AsioInfo.tcSamples = 0;

	g_AsioInfo.sysRefTime = timeGetTime();

	// ���ڴ������֧���û��Բ�����������ѡ������Ĭ��ֻ����ǰ�����������/�������������Ч����
	// �����������������������ж��Ƿ��Ѿ��������ǰ�������������
	bool bOutEnd = false;
	bool bInEnd = false;
	
	// ��������
	long buffSize = g_AsioInfo.preferredSize;

	// �������ݴ������ÿһ�������������Ļ��壩
	for (int i = 0; i < g_AsioInfo.inputBuffers + g_AsioInfo.outputBuffers; i++)
	{
		// �����������
		if (g_AsioInfo.bAsioStarted && g_AsioInfo.bufferInfos[i].isInput == false)
		{
			switch (g_AsioInfo.channelInfos[i].type)
			{
			case ASIOSTInt16LSB:
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 2);
				break;

			case ASIOSTInt24MSB:
			case ASIOSTInt24LSB:// used for 20 bits as well
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 3);
				break;

			// ��֧��32λ�����ʽ,�����ĸ�ʽδ֧�֡�
			// ASIO ��Ƶ���������֧��ȫ���Ķ����ʽ����ASIO����ֻ��Ҫ֧������һ�ָ�ʽ���ɡ�
			// 32λ�����ʽ���Ҳ��Թ���Mi-Box��ASIO4ALL��ASIO�����ж���ʹ�á�
			case ASIOSTInt32LSB:
				
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				if(bOutEnd == false)
				{
					static int iBufs = 0;
					iBufs++;

					bOutEnd = true; // ����������������ٴ���

					// �Զ�������֧�֡������һ������Ҳ��������������Զ��뵱ǰ�����ϲ�Ϊ��������
					unsigned char* channel0 = (unsigned char*)g_AsioInfo.bufferInfos[i].buffers[index];
					unsigned char* channel1 = NULL;
					if(g_AsioInfo.bufferInfos[i+1].isInput == false)
					{
						channel1 = (unsigned char*)g_AsioInfo.bufferInfos[i++].buffers[index];
					}

					// ��ȡ��Ƶ����
					if(EOF == readWaveFile(channel0, channel1, buffSize, ASIOSTInt32LSB))
					{
						fprintf(stdout, "�ļ����Ž���(%d)\ncmd>", iBufs);
						iBufs = 0;
						closeWaveFile();
						return 0L;
					}
				};
				break;

			case ASIOSTFloat32LSB:		// IEEE 754 32 bit float, as found on Intel x86 architecture
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				break;

			case ASIOSTFloat64LSB: 		// IEEE 754 64 bit double float, as found on Intel x86 architecture
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 8);
				break;

				// these are used for 32 bit data buffer, with different alignment of the data inside
				// 32 bit PCI bus systems can more easily used with these
			case ASIOSTInt32LSB16:		// 32 bit data with 18 bit alignment
			case ASIOSTInt32LSB18:		// 32 bit data with 18 bit alignment
			case ASIOSTInt32LSB20:		// 32 bit data with 20 bit alignment
			case ASIOSTInt32LSB24:		// 32 bit data with 24 bit alignment
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				break;

			case ASIOSTInt16MSB:
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 2);
				break;

			case ASIOSTInt32MSB:
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				break;

			case ASIOSTFloat32MSB:		// IEEE 754 32 bit float, as found on Intel x86 architecture
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				break;

			case ASIOSTFloat64MSB: 		// IEEE 754 64 bit double float, as found on Intel x86 architecture
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 8);
				break;

				// these are used for 32 bit data buffer, with different alignment of the data inside
				// 32 bit PCI bus systems can more easily used with these
			case ASIOSTInt32MSB16:		// 32 bit data with 18 bit alignment
			case ASIOSTInt32MSB18:		// 32 bit data with 18 bit alignment
			case ASIOSTInt32MSB20:		// 32 bit data with 20 bit alignment
			case ASIOSTInt32MSB24:		// 32 bit data with 24 bit alignment
				memset (g_AsioInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				break;
			}
		}else if(g_AsioInfo.bufferInfos[i].isInput)
		{
			// ���ﴦ��Input������

			switch(g_AsioInfo.channelInfos[i].type)
			{
			case ASIOSTInt32LSB:
				if(g_AsioInfo.bAudioRecord && bInEnd == false)
				{
					bInEnd = true; // ����������������ٴ���

					static char* charTmp = NULL;
					if(charTmp == NULL)	charTmp = new char[g_AsioInfo.maxSize*4];

					// ��������ֵĴ�����ͬ��Ҳ���Զ�˫��������
					for(int j = 0; j < buffSize; j++)
					{
						*(long*)(charTmp + j*8) = ((long*)(g_AsioInfo.bufferInfos[i].buffers[index]))[j];
						
						if(g_AsioInfo.bufferInfos[i+1].isInput)
							*(long*)(charTmp + j*8 + 4) = ((long*)(g_AsioInfo.bufferInfos[i+1].buffers[index]))[j];
						else
							*(long*)(charTmp + j*8 + 4) = 0;
					}

					i++;

					// ����¼�����Ƶ����
					writeWaveFile(charTmp, buffSize*2);
				}
			default: // ������ʽ����֧�֣���֧��Ҳ������
				break;
			}			
		}
	}

	// ֪ͨASIO Driver��������׼���á�
	// ֻ����ASIO����֧��outputReady�ӿڵ�����£��������֮��
	// ����outputReady�ӿڵ��ã�ASIO��������ʡ��һ����ʱ���壬�Ӷ��������ӳ١�
	if (g_AsioInfo.postOutput) g_pASIODrv->outputReady();

	return 0L;
}

void bufferSwitch(long index, ASIOBool processNow)
{
	// bufferSwitchTimeInfo��bufferSwitch�������汾������˵ǰ�߱Ⱥ��߶���һ��ʱ�������
	// ������bufferSwitch�е���bufferSwitchTimeInfo��������һ���Լ���ʱ�������
	// ʱ���������Ϊ�ա������԰�flagsֵ��Ϊ0������bufferSwitchTimeInfo�ͻ�������ˡ�

	ASIOTime  timeInfo;
	memset (&timeInfo, 0, sizeof (timeInfo));

	if(g_pASIODrv->getSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime) == ASE_OK)
		timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;

	bufferSwitchTimeInfo (&timeInfo, index, processNow);
}

void sampleRateChanged(ASIOSampleRate sRate)
{
	printf("�����ʸı�Ϊ:%f", sRate);
}

// 
long asioMessages(long selector, long value, void* message, double* opt)
{
	int ret = 0;
	switch(selector)
	{
	case kAsioSelectorSupported:
		if(value == kAsioResetRequest
			|| value == kAsioEngineVersion
			|| value == kAsioResyncRequest
			|| value == kAsioLatenciesChanged
			// the following three were added for ASIO 2.0, you don't necessarily have to support them
			|| value == kAsioSupportsTimeInfo
			|| value == kAsioSupportsTimeCode
			|| value == kAsioSupportsInputMonitor)
			ret = 1L;
		break;

	case kAsioResetRequest:
		// defer the task and perform the reset of the driver during the next "safe" situation
		// You cannot reset the driver right now, as this code is called from the driver.
		// Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
		// Afterwards you initialize the driver again.
		g_AsioInfo.stopped = TRUE;  // In this sample the processing will just stop
		g_AsioInfo.bAsioStarted = FALSE;
		g_AsioInfo.bAudioRecord = FALSE;

		ret = 1L;
		break;

	case kAsioResyncRequest:
		// This informs the application, that the driver encountered some non fatal data loss.
		// It is used for synchronization purposes of different media.
		// Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
		// Windows Multimedia system, which could loose data because the Mutex was hold too long
		// by another thread.
		// However a driver can issue it in other situations, too.
		ret = 1L;
		break;

	case kAsioLatenciesChanged:
		// This will inform the host application that the drivers were latencies changed.
		// Beware, it this does not mean that the buffer sizes have changed!
		// You might need to update internal delay data.
		ret = 1L;
		break;

	case kAsioEngineVersion:
		// return the supported ASIO version of the host application
		// If a host applications does not implement this selector, ASIO 1.0 is assumed
		// by the driver
		ret = 2L;
		break;

	case kAsioSupportsTimeInfo:
		// informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
		// is supported.
		// For compatibility with ASIO 1.0 drivers the host application should always support
		// the "old" bufferSwitch method, too.
		ret = 1;
		break;

	case kAsioSupportsTimeCode:
		// informs the driver wether application is interested in time code info.
		// If an application does not need to know about time code, the driver has less work
		// to do.
		ret = 0;
		break;
	}

	return ret;
}
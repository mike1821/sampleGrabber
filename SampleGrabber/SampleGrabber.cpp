#include "stdafx.h"

#include <stdio.h>
#include <conio.h>
#include <clocale>
#include <dshow.h>
#include <time.h>


#include <windows.h>


#include <windows.h>
/*
 #include <conio.h>
 #include <tchar.h>*/
#include <iostream>
#include <string>
#include <fstream> 
#include <conio.h>
#include <stdio.h>

#include "SampleGrabber.h"

//#pragma comment (lib,"Strmiids.lib")

//============================================
char imagebuffer[20 * 1024 * 1024];

using namespace std;

const char* szName = "Global\\IncomingImage";
char* pBuf;
void *hMapFile;

int InitSharedMem()
{

	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		0x001400000,              // maximum object size (low-order DWORD)
		(LPCWSTR)szName);                 // name of mapping object

	if (hMapFile == NULL)
	{
		printf("Could not create file mapping object\n");
		//getch();
		return -1;
	}

	pBuf = (char *)MapViewOfFile(hMapFile,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		0x001400000);

	if (pBuf == NULL)
	{
		printf("Could not map view of file\n");
		CloseHandle(hMapFile);
		//getch();
		return -1;
	}
	printf("share mem initialized\n");
	return 0;

}
/*
int UpdateSharedMem()
{
	std::streampos size;
	char * memblock;
	char filename[128];

	memset(filename, 0, sizeof(filename));
	for (int i = 1; i<4; i++){
		sprintf_s(filename, "C:\\temp\\output-%03d.bmp", i);
		std::ifstream ifile(filename, std::ios::in | std::ios::binary);
		if (ifile.is_open())
		{
			ifile.seekg(0, std::ios::end);
			size = ifile.tellg();
			ifile.seekg(0, std::ios::beg);

			memblock = new char[size];

			ifile.read(memblock, size);
			ifile.close();
		}
		else{
			std::cout << "Cannot open file: " << filename << std::endl;
			return -1;
		}

		CopyMemory((PVOID)pBuf, memblock, size);
		std::cout << "File updated to: " << filename << std::endl;
		Sleep(10000);
		delete[] memblock;
	}

	return 0;
}*/
//============================================


Capture::Capture()
{
	mInitialized = false;
	m_nWidth = 640;
	m_nHeight = 480;

	
}

Capture::~Capture()
{
}

/*********************************
*  Write RGB array to BMP file
*********************************/
HRESULT Capture::WriteBitmap(char* pszFileName, BITMAPINFOHEADER *pBMI, size_t cbBMI,
	BYTE *pData, size_t cbData)
{
	

	unsigned long pos = 0;
	/*HANDLE hFile = CreateFileA(pszFileName, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, 0, NULL);
	if (hFile == NULL)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}*/

	BITMAPFILEHEADER bmf = {};

	bmf.bfType = 'MB';
	bmf.bfSize = cbBMI + cbData + sizeof(bmf);
	bmf.bfOffBits = sizeof(bmf) + cbBMI;

	DWORD cbWritten = 0;
	//BOOL result = WriteFile(hFile, &bmf, sizeof(bmf), &cbWritten, NULL);
	memcpy(imagebuffer, &bmf, sizeof(bmf));
	pos += sizeof(bmf);
	
	memcpy(imagebuffer+pos, pBMI, cbBMI);
	pos += cbBMI;

	memcpy(imagebuffer + pos, pData, cbData);
	pos += cbData;
	/*
	if (result)
	{
		result = WriteFile(hFile, pBMI, cbBMI, &cbWritten, NULL);
	}
	if (result)
	{
		result = WriteFile(hFile, pData, cbData, &cbWritten, NULL);
	}*/

	HRESULT hr = 1;// result ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	CopyMemory((PVOID)pBuf, imagebuffer, pos);
	printf("hr=%ld\n", hr);
	//CloseHandle(hFile);

	return hr;
}

/*********************************
*   Takes 5 photos from webcam
*     and saves it to files
*********************************/
bool Capture::CaptureImage(int cur_cam, char *FileName, int width_, int height_)
{

	ICreateDevEnum *pSysDevEnum = 0;
	IEnumMoniker *pEnumCams = 0;
	IMoniker *pMon = 0;
	IBaseFilter *CameraF = 0;

	IGraphBuilder *pGraph = NULL;
	IMediaControl *pControl = NULL;
	IMediaEventEx *pEvent = NULL;
	IBaseFilter *pGrabberF = NULL;
	ISampleGrabber *pSGrabber = NULL;
	IBaseFilter *pSourceF = NULL;
	IEnumPins *pEnum = NULL;
	IPin *pPin = NULL;
	IBaseFilter *pNullF = NULL;

	BYTE *pBuffer = NULL;
	HRESULT hr;
	AM_MEDIA_TYPE mt;
	int count = 0;
	
	
	InitSharedMem();
	printf("Starting with capturing\n");
	//Device list
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void **)&pSysDevEnum);
	if (FAILED(hr)) 
		return false;

	// Obtain a class enumerator for the video compressor category.
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
		&pEnumCams, 0);
	if (hr != S_OK)
		return false;

	// Enumerate the monikers. to desired device (0,1,2,...)
	for (int i = 0; i <= cur_cam; ++i)
	{
		hr = pEnumCams->Next(1, &pMon, NULL);
		if (hr != S_OK)
			return false;
	}
	//Get BaseFilter of chosen camera
	hr = pMon->BindToObject(0, 0, IID_IBaseFilter, (void**)&CameraF);
	if (hr != S_OK)
		return false;

	//Create the Filter Graph Manager 
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pGraph));
	if (FAILED(hr))
		return false;

	//Query for the IMediaControl and IMediaEventEx interfaces
	hr = pGraph->QueryInterface(IID_PPV_ARGS(&pControl));
	if (FAILED(hr))
		return false;

	hr = pGraph->QueryInterface(IID_PPV_ARGS(&pEvent));
	if (FAILED(hr))
		return false;

	// Add web camera to graph as source filter (because first?)
	hr = pGraph->AddFilter(CameraF, L"Capture Source");
	if (hr != S_OK)
		return false;

	// Create an instance of the Sample Grabber Filter
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pGrabberF));
	if (FAILED(hr))
		return false;

	// Add it to the filter graph.
	hr = pGraph->AddFilter(pGrabberF, L"Sample Grabber");
	if (FAILED(hr))
		return false;

	hr = pGrabberF->QueryInterface(IID_PPV_ARGS(&pSGrabber));
	if (FAILED(hr))
		return false;

	//Set the Media Type
	ZeroMemory(&mt, sizeof(mt));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_RGB24;
	mt.formattype = FORMAT_VideoInfo;
	hr = pSGrabber->SetMediaType(&mt);
	if (FAILED(hr))
		return false;

	//Building graph. Connecting Source (CameraF) and Sample Grabber
	hr = CameraF->EnumPins(&pEnum);
	if (FAILED(hr))
		return false;

	while (S_OK == pEnum->Next(1, &pPin, NULL))
	{
		hr = SetResolution(width_, height_, pPin);

		if (FAILED(hr))
			std::cout << "Failed to set resolution" << std::endl;

		hr = ConnectFilters(pGraph, pPin, pGrabberF);
		
		if (SUCCEEDED(hr))
		{
			std::cout << "Success to Connect Filter Graphs" << std::endl;
			break;
		}
		else{
			std::cout << "Failed to Connect Filter Graphs RRR: "<< hr << std::endl;
		}
	}

	if (FAILED(hr)){
		std::cout << "Failed A" << std::endl;
		return false;
	}

	
	SafeRelease(pPin);

	//The following code connects the Sample Grabber to the Null Renderer filter:
	hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pNullF));
	if (FAILED(hr)){
		std::cout << "Failed B" << std::endl;
		return false;
	}
	hr = pGraph->AddFilter(pNullF, L"Null Filter");
	if (FAILED(hr)){
		std::cout << "Failed C" << std::endl;
		return false;
	}

	//ConnectFilters SamplerGrabber and Null Filter
	hr = ConnectFilters(pGraph, pGrabberF, pNullF);
	if (FAILED(hr)){
		std::cout << "Failed D" << std::endl;
		return false;
	}

	hr = pSGrabber->SetBufferSamples(TRUE);
	if (FAILED(hr)){
		std::cout << "Failed E" << std::endl;
		return false;
	}

	hr = pSGrabber->SetOneShot(TRUE);//Halt after sample received
	if (FAILED(hr)){
		std::cout << "Failed F" << std::endl;
		return false;
	}
	hr = pControl->Run();//Run filter graph
	if (FAILED(hr)){
		std::cout << "Failed G" << std::endl;
		return false;
	}

	printf("Wait for a seconds to let cam sensor adopt to light\n");
	Sleep(3000);//Wait 3 seconds
	printf("OK\n");

	//char filename[50];
	printf("Go for the  loop");

	for (;;){

		long evCode;
		while (1)
		{
			hr = pEvent->WaitForCompletion(INFINITE, &evCode);//Wait for frame
			if (evCode == EC_COMPLETE || evCode == EC_SYSTEMBASE)
				break;
			printf("Not complete. event: %d\n", evCode);
			pControl->Pause();//pause the Graph
			pSGrabber->SetOneShot(TRUE);//Set to halt after first frame
			hr = pControl->Run();//resume filter graph
			if (FAILED(hr))
				return false;
			Sleep(1000);//wait for a second
		}
		printf("out of while  loop");
		//if (hr != S_OK)
		//	return false;
		Sleep(100);
		//there frame got. Graph still running, but Sample Grabber halted

		// Find the required buffer size.
		long cbBufSize;
		hr = pSGrabber->GetCurrentBuffer(&cbBufSize, NULL);
		if (FAILED(hr))
			return false;

		pBuffer = (BYTE*)CoTaskMemAlloc(cbBufSize);
		if (!pBuffer)
		{
			hr = E_OUTOFMEMORY;
			return false;
		}

		hr = pSGrabber->GetCurrentBuffer(&cbBufSize, (long*)pBuffer);
		if (FAILED(hr))
			return false;


		hr = pSGrabber->GetConnectedMediaType(&mt);
		if (FAILED(hr))
			return false;


		// Examine the format block.
		if ((mt.formattype == FORMAT_VideoInfo) &&
			(mt.cbFormat >= sizeof(VIDEOINFOHEADER)) && (mt.pbFormat != NULL))
		{
			VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)mt.pbFormat;
			//sprintf_s(filename, sizeof(filename), "photo%d.bmp",++count);
			time_t t1 = time(NULL);
			SYSTEMTIME st;
			GetSystemTime(&st);

			printf("Save frame to file: %s - %02ld,%02ld.%03ld seconds\n", FileName, st.wMinute,st.wSecond, st.wMilliseconds );
			hr = WriteBitmap(FileName, &pVih->bmiHeader, mt.cbFormat - SIZE_PREHEADER,pBuffer, cbBufSize);
		}
		else
		{
			// Invalid format.
			hr = VFW_E_INVALIDMEDIATYPE;
			printf("Invalid frame format\n");
		}

		CoTaskMemFree(pBuffer);

		if ((GetAsyncKeyState(VK_ESCAPE) & 0x01))
			break;
		else if ((GetAsyncKeyState(VK_SPACE) & 0x01))
			CameraProperties(CameraF);
	
	}
	//if (i != 5)//if it wasnt last frame
	//{
	//	pControl->Pause();//pause the Graph
	//	pSGrabber->SetOneShot(TRUE);//Set to halt after first frame
	//	hr = pControl->Run();//resume filter graph
	//	if (FAILED(hr))
	//		return false;

	//	Sleep(1000);//wait for a second
	//}
	//SafeRelease(pPin);

	SafeRelease(pEnum);
	SafeRelease(pNullF);
	SafeRelease(pSourceF);
	SafeRelease(pSGrabber);
	SafeRelease(pGrabberF);
	SafeRelease(pControl);
	SafeRelease(pEvent);
	SafeRelease(pGraph);

	return true;
}

HRESULT Capture::SetResolution(int width_, int height_, IPin *pPin)
{
	// Set the grabbing size
	// First we iterate through the available media types and 
	// store the first one that fits the requested size.
	// If we have found one, we set it.
	// In any case we query the size of the current media type
	// to have this information for clients of this class.

	HRESULT hr;

	IAMStreamConfig *pConfig=NULL;
	//IEnumMediaTypes *pMedia;
	BYTE *pSCC = NULL;
	AM_MEDIA_TYPE *pmt = NULL, *pfnt = NULL, *mfnt=NULL;

	//hr = pPin->EnumMediaTypes(&pMedia);
	int iCount, iSize;
	
	hr = pPin->QueryInterface(IID_IAMStreamConfig, (void **)&pConfig);

	hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);
	pSCC = new BYTE[iSize];
	int i = 0;
	if (SUCCEEDED(hr))
	{
		while (i<=iCount)
		{
			pConfig->GetStreamCaps(i++, &pfnt, pSCC);

			VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)pfnt->pbFormat;
			printf("Size %i  %i\n", vih->bmiHeader.biWidth, vih->bmiHeader.biHeight );
			if (vih->bmiHeader.biWidth == width_ && vih->bmiHeader.biHeight == height_)
			{
				//pfnt = mfnt;
				printf("Found mediatype with %i %i - %x\n", vih->bmiHeader.biWidth, vih->bmiHeader.biHeight, pfnt->formattype );
				hr = pConfig->SetFormat(pfnt);
				//break;
			}

		}
	}

	//DeleteMediaType(pfnt);
	delete[] pSCC;
	pConfig->Release();
	return hr;
}

void Capture::DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	// allow NULL pointers for coding simplicity

	if (pmt == NULL) {
		return;
	}

	if (pmt->cbFormat != 0) {
		CoTaskMemFree((PVOID)pmt->pbFormat);

		// Strictly unnecessary but tidier
		pmt->cbFormat = 0;
		pmt->pbFormat = NULL;
	}
	if (pmt->pUnk != NULL) {
		pmt->pUnk->Release();
		pmt->pUnk = NULL;
	}

	CoTaskMemFree((PVOID)pmt);
}

void Capture::CameraProperties(IBaseFilter *cameraF){

	HWND hWnd = NULL;
	//Display Camera pin properties window
	ISpecifyPropertyPages *pSpec;
	CAUUID cauuid;
	HRESULT hr;

	hr = cameraF->QueryInterface(IID_ISpecifyPropertyPages,
		(void **)&pSpec);
	if (hr == S_OK)
	{
		hr = pSpec->GetPages(&cauuid);

		hr = OleCreatePropertyFrame(hWnd, 30, 30, NULL, 1,
			(IUnknown **)&cameraF, cauuid.cElems,
			(GUID *)cauuid.pElems, 0, 0, NULL);

		CoTaskMemFree(cauuid.pElems);
		pSpec->Release();
	}

}

/*********************************
*  Out Webcam list to console
*********************************/
bool Capture::OutVideoDeviceList()
{
	HRESULT hr;
	ICreateDevEnum *pSysDevEnum = 0;
	IEnumMoniker *pEnumCat = 0;
	IMoniker *pMoniker = 0;

	CoInitialize(NULL);

	// Create the System Device Enumerator.
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void **)& pSysDevEnum);
	if (FAILED(hr)) 
		return false;

	// Obtain a class enumerator for the video compressor category.

	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
		&pEnumCat, 0);
	if (hr != S_OK)
		return false;

	// Enumerate the monikers.
	int camd = 0;
	while (pEnumCat->Next(1, &pMoniker, NULL) == S_OK)
	{
		IPropertyBag *pPropBag;
		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);

		if (SUCCEEDED(hr))
		{
			// To retrieve the filter's friendly name, do the following:
			VARIANT varName;
			VariantInit(&varName);
			hr = pPropBag->Read(L"FriendlyName", &varName, 0);

			if (SUCCEEDED(hr))
			{
				char devname[255];
				//Converting UNICODE [BSTR=WCHAR] to ANSI [C] string
				WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, varName.bstrVal,
					-1, devname, sizeof(devname), NULL, NULL);

				printf("%d - %s\n", camd, devname);
			}
			VariantClear(&varName);
			pPropBag->Release();
		}

		++camd;
	}

	SafeRelease(pMoniker);
	SafeRelease(pEnumCat);
	SafeRelease(pSysDevEnum);

	return true;
}

// Query whether a pin is connected to another pin.
// Note: This function does not return a pointer to the connected pin.
HRESULT Capture::IsPinConnected(IPin *pPin, BOOL *pResult)
{
	IPin *pTmp = NULL;
	HRESULT hr = pPin->ConnectedTo(&pTmp);
	if (SUCCEEDED(hr))
	{
		*pResult = TRUE;
	}
	else if (hr == VFW_E_NOT_CONNECTED)
	{
		// The pin is not connected. This is not an error for our purposes.
		*pResult = FALSE;
		hr = S_OK;
	}

	SafeRelease(pTmp);
	return hr;
}

// Query whether a pin has a specified direction (input / output)
HRESULT Capture::IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult)
{
	PIN_DIRECTION pinDir;
	HRESULT hr = pPin->QueryDirection(&pinDir);
	if (SUCCEEDED(hr))
	{
		*pResult = (pinDir == dir);
	}
	return hr;
}

// Match a pin by pin direction and connection state.
HRESULT Capture::MatchPin(IPin *pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult)
{
	if (pResult == NULL)
	{
		printf("\nMatchPin pResult==NULL. ABORT\n");
		_getch();
		abort();
	}

	BOOL bMatch = FALSE;
	BOOL bIsConnected = FALSE;

	HRESULT hr = IsPinConnected(pPin, &bIsConnected);
	if (SUCCEEDED(hr))
	{
		if (bIsConnected == bShouldBeConnected)
		{
			hr = IsPinDirection(pPin, direction, &bMatch);
		}
	}

	if (SUCCEEDED(hr))
	{
		*pResult = bMatch;
	}
	return hr;
}

// Return the first unconnected input pin or output pin.
HRESULT Capture::FindUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
	IEnumPins *pEnum = NULL;
	IPin *pPin = NULL;
	BOOL bFound = FALSE;

	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		goto done_fup;
	}

	while (S_OK == pEnum->Next(1, &pPin, NULL))
	{
		hr = MatchPin(pPin, PinDir, FALSE, &bFound);
		if (FAILED(hr))
		{
			goto done_fup;
		}
		if (bFound)
		{
			*ppPin = pPin;
			(*ppPin)->AddRef();
			break;
		}
		SafeRelease(pPin);
	}

	if (!bFound)
	{
		hr = VFW_E_NOT_FOUND;
	}

done_fup:
	SafeRelease(pPin);
	SafeRelease(pEnum);

	return hr;
}

// Connect output pin to filter.
HRESULT Capture::ConnectFilters(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest)
{
	IPin *pIn = NULL;

	// Find an input pin on the downstream filter.
	HRESULT hr = FindUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
	if (SUCCEEDED(hr))
	{
		// Try to connect them.
		hr = pGraph->Connect(pOut, pIn);
		pIn->Release();
	}
	return hr;
}

// Connect filter to filter
HRESULT Capture::ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest)
{
	IPin *pOut = NULL;

	// Find an output pin on the first filter.
	HRESULT hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
	if (SUCCEEDED(hr))
	{
		hr = ConnectFilters(pGraph, pOut, pDest);
		pOut->Release();
	}
	return hr;
}

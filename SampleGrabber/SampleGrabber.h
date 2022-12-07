#ifndef SAMPLEGRABBER_H
#define SAMPLEGRABBER_H

#include "stdafx.h"
#include "qedit.h"

//Safe releasing of DirectShow object
#define SafeRelease(x) { if (x) x->Release(); x = NULL; }

class Capture{

public:
	Capture();
	~Capture();

	bool CaptureImage(int cur_cam, char *FileName, int width_, int height_);
	bool OutVideoDeviceList();


private:

	int m_nWidth;
	int m_nHeight;
	int mInitialized;

	HRESULT WriteBitmap(char* pszFileName, BITMAPINFOHEADER *pBMI, size_t cbBMI,BYTE *pData, size_t cbData);
	void DeleteMediaType(AM_MEDIA_TYPE *pmt);

	HRESULT IsPinConnected(IPin *pPin, BOOL *pResult);
	HRESULT IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult);
	HRESULT MatchPin(IPin *pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult);
	HRESULT FindUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
	HRESULT ConnectFilters(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest);
	HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest);
	HRESULT SetResolution(int width_, int height_, IPin *pPin);
	void CameraProperties(IBaseFilter *cameraF);

};

#endif
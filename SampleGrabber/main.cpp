#include "stdafx.h"

#include <stdio.h>
#include <conio.h>
#include <clocale>
#include <dshow.h>

#include "SampleGrabber.h"

Capture my_capture;

int main()
{
	int camera = 0;
	char filename[50];
	HRESULT hr;
	bool result = false;

	// Initialize COM
	//CoInitialize(NULL);

	printf("Available video devices:\n");
	result = my_capture.OutVideoDeviceList();
	if (!result){
		printf("No video devices available\n");
		return 0;
	}

	printf("\nChoose one [0-9]:");
	scanf_s("%d", &camera);


	//printf("Initialize camera filters\n");
	//my_capture.CaptureImage(camera, "");

	//printf("Setting camera resolution\n");
	//hr = my_capture.SetResolution(640,480,my_capture.pPin);
	//if (FAILED(hr)){
		//printf("Cannot set given resolution\n");
		//return 0;
	//}

	//printf("Displaying camera properties page\n");
	//my_capture.CameraProperties(my_capture.CameraF);

	printf("Start shooting\n");
	
	sprintf_s(filename, sizeof(filename), "output.bmp");
	printf("Save frame to file: %s\n", filename);
	my_capture.CaptureImage(camera, filename,1280,720);
			
	printf("End shooting\n");

	_getch();

	return 0;
}

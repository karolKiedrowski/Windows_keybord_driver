/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    KBFTEST.C

Abstract:


Environment:

    usermode console application

--*/


#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <initguid.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ntddkbd.h>

#pragma warning(disable:4201)

#include <setupapi.h>
#include <winioctl.h>

#pragma warning(default:4201)

#include "..\sys\public.h"

//-----------------------------------------------------------------------------
// 4127 -- Conditional Expression is Constant warning
//-----------------------------------------------------------------------------
#define WHILE(constant) \
__pragma(warning(disable: 4127)) while(constant); __pragma(warning(default: 4127))

DEFINE_GUID(GUID_DEVINTERFACE_KBFILTER,
0x3fb7299d, 0x6847, 0x4490, 0xb0, 0xc9, 0x99, 0xe0, 0x98, 0x6a, 0xb8, 0x86);
// {3FB7299D-6847-4490-B0C9-99E0986AB886}


int
_cdecl
main(
    _In_ int argc,
    _In_ char *argv[]
    )
{
    HDEVINFO                            hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
    ULONG                               predictedLength = 0;
    ULONG                               requiredLength = 0, bytes=0;
    HANDLE                              file;
    ULONG                               i =0;
    KEYBOARD_ATTRIBUTES                 kbdattrib;
    KEYBOARD_INPUT_DATA                 kb_key_buffer[30];
    int                                 counter = 0;
    

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    //
    // Open a handle to the device interface information set of all
    // present toaster class interfaces.
    //

    hardwareDeviceInfo = SetupDiGetClassDevs (
                       (LPGUID)&GUID_DEVINTERFACE_KBFILTER,
                       NULL, // Define no enumerator (global)
                       NULL, // Define no
                       (DIGCF_PRESENT | // Only Devices present
                       DIGCF_DEVICEINTERFACE)); // Function class devices.
    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        printf("SetupDiGetClassDevs failed: %x\n", GetLastError());
        return 0;
    }

    deviceInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

    //printf("\nList of KBFILTER Device Interfaces\n");
    //printf("---------------------------------\n");

    i = 0;

    //
    // Enumerate devices of toaster class
    //

    do {
        if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                 0, // No care about specific PDOs
                                 (LPGUID)&GUID_DEVINTERFACE_KBFILTER,
                                 i, //
                                 &deviceInterfaceData)) {

            if(deviceInterfaceDetailData) {
                free (deviceInterfaceDetailData);
                deviceInterfaceDetailData = NULL;
            }

            //
            // Allocate a function class device data structure to
            // receive the information about this particular device.
            //

            //
            // First find out required length of the buffer
            //

            if(!SetupDiGetDeviceInterfaceDetail (
                    hardwareDeviceInfo,
                    &deviceInterfaceData,
                    NULL, // probing so no output buffer yet
                    0, // probing so output buffer length of zero
                    &requiredLength,
                    NULL)) { // not interested in the specific dev-node
                if(ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
                    printf("SetupDiGetDeviceInterfaceDetail failed %d\n", GetLastError());
                    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
                    return FALSE;
                }

            }

            predictedLength = requiredLength;

            deviceInterfaceDetailData = malloc (predictedLength);

            if(deviceInterfaceDetailData) {
                deviceInterfaceDetailData->cbSize =
                                sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
            } else {
                printf("Couldn't allocate %d bytes for device interface details.\n", predictedLength);
                SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
                return FALSE;
            }


            if (! SetupDiGetDeviceInterfaceDetail (
                       hardwareDeviceInfo,
                       &deviceInterfaceData,
                       deviceInterfaceDetailData,
                       predictedLength,
                       &requiredLength,
                       NULL)) {
                printf("Error in SetupDiGetDeviceInterfaceDetail\n");
                SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
                free (deviceInterfaceDetailData);
                return FALSE;
            }
            //printf("%d) %s\n", ++i, deviceInterfaceDetailData->DevicePath);
            ++i;
        }
        else if (ERROR_NO_MORE_ITEMS != GetLastError()) {
            free (deviceInterfaceDetailData);
            deviceInterfaceDetailData = NULL;
            continue;
        }
        else
            break;

    } WHILE (TRUE);


    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);

    if(!deviceInterfaceDetailData)
    {
        printf("No device interfaces present\n");
        return 0;
    }

    //
    // Open the last toaster device interface
    //

    //printf("\nOpening the last interface:\n %s\n", deviceInterfaceDetailData->DevicePath);

    file = CreateFile ( deviceInterfaceDetailData->DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, // no SECURITY_ATTRIBUTES structure
                        OPEN_EXISTING, // No special create flags
                        0, // No special attributes
                        NULL);

    if (INVALID_HANDLE_VALUE == file) {
        printf("Error in CreateFile: %x", GetLastError());
        free (deviceInterfaceDetailData);
        return 0;
    }
    
    //
    // Send an IOCTL to retrive the keyboard attributes
    // These are cached in the kbfiltr
    //
    if (!DeviceIoControl (file,
                          IOCTL_KBFILTR_GET_KEYBOARD_ATTRIBUTES,
                          NULL, 0,
                          &kbdattrib, sizeof(kbdattrib),
                          &bytes, NULL)) {
        printf("Retrieve Keyboard Attributes request failed:0x%x\n", GetLastError());
        free (deviceInterfaceDetailData);
        CloseHandle(file);
        return 0;
    }

    bytes = 0;
    printf("\n\nFirst 30 key buffer.\n");

    if (!DeviceIoControl(file,
                         IOCTL_KBFILTR_GET_KEYBOARD_INPUT_DATA,
                         NULL, 0,
                         &kb_key_buffer, sizeof(kb_key_buffer),
                         &bytes, NULL)) {
        printf("Retrieve get Keys Buffer request failed:0x%x\n", GetLastError());
        free(deviceInterfaceDetailData);
        CloseHandle(file);
        return 0;
    }

    free (deviceInterfaceDetailData);
    CloseHandle(file);

    // Writing data to a text file. 
    FILE* key_buffer_file = fopen("E:\\sterowniki_inzynierka\\kbfiltr\\key_buffer.txt", "w");
    if (key_buffer_file == NULL)
    {
        printf("The file is not opened. The program will now exit.\n");
        exit(0);
    }
    
    printf("The file is created successfully.\n");

    // Informations about keyboard attribiutes.
    fprintf(key_buffer_file,
        "Keyboard Attributes:\n"
        " KeyboardMode:          0x%x\n"
        " NumberOfFunctionKeys:  0x%x\n"
        " NumberOfIndicators:    0x%x\n"
        " NumberOfKeysTotal:     0x%x\n"
        " InputDataQueueLength:  0x%x\n",
        (int)kbdattrib.KeyboardMode,
        (int)kbdattrib.NumberOfFunctionKeys,
        (int)kbdattrib.NumberOfIndicators,
        (int)kbdattrib.NumberOfKeysTotal,
        (int)kbdattrib.InputDataQueueLength);

    // Inormations about keyboard input datas.
    fprintf(key_buffer_file, "30 Keyboard Input Datas.\n");
    
    for (counter = 0; counter < 30; counter++)
    {
        fprintf(key_buffer_file, "%d", counter + 1);
        fprintf(key_buffer_file, " MakeCode %d Flag", (int)kb_key_buffer[counter].MakeCode);
        switch (kb_key_buffer[counter].Flags)
        {
        case KEY_MAKE:
            fprintf(key_buffer_file, " KEY_MAKE\n");
            break;
        case KEY_BREAK:
            fprintf(key_buffer_file, " KEY_BREAK\n");
            break;
        case KEY_E0:
            fprintf(key_buffer_file, " KEY_E0\n");
            break;
        case KEY_E1:
            fprintf(key_buffer_file, " KEY_E1\n");
            break;
        default:
            fprintf(key_buffer_file, " No Flag was set\n");
            break;
        }
    }

    fclose(key_buffer_file);

    return 0;
}




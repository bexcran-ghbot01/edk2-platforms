/** @file

  Copyright (c) 2020, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_CONFIG_H_
#define CPU_CONFIG_H_

#include <Uefi.h>

#include <Guid/CpuConfigHii.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/PlatformManagerHii.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NVParamLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <NVParamDef.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>

#include "NVDataStruc.h"

//
// This is the generated IFR binary data for each formset defined in VFR.
//
extern UINT8 VfrBin[];

//
// This is the generated String package data for all .UNI files.
//
extern UINT8 CpuConfigDxeStrings[];

#define CPU_CONFIG_PRIVATE_SIGNATURE SIGNATURE_32 ('C', 'P', 'U', '_')

typedef struct {
  UINTN Signature;

  EFI_HANDLE        DriverHandle;
  EFI_HII_HANDLE    HiiHandle;
  CPU_VARSTORE_DATA Configuration;

  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;

  EFI_HII_CONFIG_ACCESS_PROTOCOL ConfigAccess;
} CPU_CONFIG_PRIVATE_DATA;

#define CPU_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, CPU_CONFIG_PRIVATE_DATA, ConfigAccess, CPU_CONFIG_PRIVATE_SIGNATURE)

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH       VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

#endif /* CPU_CONFIG_H_ */

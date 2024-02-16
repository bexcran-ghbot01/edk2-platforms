/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef IPMI_COMMAND_LIB_EXT_
#define IPMI_COMMAND_LIB_EXT_

#include <IndustryStandard/Ipmi.h>

#pragma pack (1)

//
// Max BMC channel
//
#define BMC_MAX_CHANNEL 0xF // 4 bits

//
// IPMI Boot Option CompletionCode
//
#define IPMI_COMP_CODE_PARAM_UNSUPPORTED  0x80
#define IPMI_COMP_CODE_SET_IN_PROGRESS    0x81
#define IPMI_COMP_CODE_READ_ONLY          0x82

//
// IPMI Boot Flag clear cmos
//
#define IPMI_BOOT_FLAG_CLEAR_CMOS_YES             0x01
#define IPMI_BOOT_FLAG_CLEAR_CMOS_NO              0x00

//
// IPMI Boot Option param 5 (Boot Flags) info
//
typedef struct {
  BOOLEAN IsBootFlagsValid;
  BOOLEAN IsPersistent;
  BOOLEAN IsCmosClear;
  UINT8   DeviceSelector;
  UINT8   InstanceSelector;
} IPMI_BOOT_FLAGS_INFO;

//
// Structure definition for IPMI "Get System GUID" response data
//
typedef struct {
  UINT8  CompletionCode;
  UINT8  Guid[16];
} IPMI_GET_SYSTEM_GUID_RESPONSE;

//
// Structure definition for BMC LAN info
//
typedef struct {
  IPMI_LAN_IP_ADDRESS IpAddress;
  IPMI_LAN_IP_ADDRESS SubnetMask;
} BMC_LAN_INFO;

/**
  Get BMC LAN Information of specific channel.

  @param[in]     BmcChannel            BMC Channel to retrieve LAN information.
  @param[out]    Info                  Pointer to return buffer.

  @retval EFI_SUCCESS                  The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_INVALID_PARAMETER        Info was NULL.
  @retval other                        Failed to get BMC LAN info.
**/
EFI_STATUS
EFIAPI
IpmiGetBmcLanInfo (
  IN  UINT8          BmcChannel,
  OUT BMC_LAN_INFO   *Info
  );

/**
  Set Boot Info Acknowledge to notify BMC that the Boot Flags has been handled by UEFI.

  @retval EFI_SUCCESS                           The command byte stream was successfully submit to the device and a response was successfully received.
  @retval other                                 Failed to write data to the device.
**/
EFI_STATUS
EFIAPI
IpmiSetBootInfoAck (
  VOID
  );

/**
  Retrieve Boot Info Acknowledge from BMC.

  @param[out] BootInitiatorAcknowledgeData      Pointer to returned buffer.

  @retval EFI_SUCCESS                           The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_INVALID_PARAMETER                 BootInitiatorAcknowledgeData was NULL.
  @retval other                                 Failed to write data to the device.
**/
EFI_STATUS
EFIAPI
IpmiGetBootInfoAck (
  OUT UINT8 *BootInitiatorAcknowledgeData
  );

/**
  Send command to clear BMC Boot Flags parameter.

  @retval EFI_SUCCESS                           The command byte stream was successfully submit to the device and a response was successfully received.
  @retval other                                 Failed to write data to the device.
**/
EFI_STATUS
EFIAPI
IpmiClearBootFlags (
  VOID
  );

/**
  Retrieve Boot Flags parameter from BMC.

  @param[out] BootFlags                         Pointer to returned buffer.

  @retval EFI_SUCCESS                           The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_INVALID_PARAMETER                 BootFlags was NULL.
  @retval other                                 Failed to write data to the device.
**/
EFI_STATUS
EFIAPI
IpmiGetBootFlags (
  OUT IPMI_BOOT_FLAGS_INFO *BootFlags
  );

/**
  Send command to clear BMC Clear Cmos in Boot Flags parameter.

  @retval EFI_SUCCESS      The command byte stream was successfully submit
                           to the device and a response was successfully received.
  @retval other            Failed to write data to the device.
**/
EFI_STATUS
EFIAPI
IpmiClearCmosBootFlags (
  VOID
  );

#pragma pack()
#endif /* IPMI_COMMAND_LIB_EXT_ */

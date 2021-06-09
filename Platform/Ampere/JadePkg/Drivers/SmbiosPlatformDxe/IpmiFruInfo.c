/** @file
  Provides functions to read FRU information from BMC via IPMI interface,
  and update FRU PCDs appropriately.

  Reference:
    - Platform Management FRU Information Storage Definition V1.0

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/IpmiNetFnApp.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiCommandLib.h>
#include <Library/IpmiCommandLibExt.h>
#include <Library/IpmiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

//
// Assume that there is only one FRU device
// and Device ID for Chassis/Board Information is 0.
//
#define FRU_DEVICE_ID_DEFAULT         0

//
// Maximum length of FRU Area Information
//
#define FRU_AREA_LENGTH_MAX           256

//
// Maximum length of response data
//
#define IPMI_FRU_RESPONSE_LENGTH_MAX  32

//
// Refer to Section 13.1 BCD PLUS Definition
//
STATIC CONST CHAR8 BcdPlus[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  ' ',
  '-',
  '.',
  ':',
  ',',
  '_'
};

//
// Refer to Section 13. TYPE/LENGTH BYTE FORMAT
//
CHAR8 *
ConvertEncodedDataToString (
  IN     UINT8      *FruData,
  IN OUT UINT16     *StartingOffset
  )
{
  UINT8  TypeCode;
  UINT8  Length;
  UINT16 Offset;
  UINT8  Index1;
  UINT8  Index2;
  UINT8  Index3;
  UINT16 DataSize;
  CHAR8  *String;
  UINT32 TempData;

  if (FruData == NULL || StartingOffset == NULL) {
    return NULL;
  }

  DataSize = 0;
  Offset = *StartingOffset;

  //
  // Type/Length Byte Format
  //
  // Bits 7:6 - type code
  //    0b00 - Binary or unspecified
  //    0b01: BCD plus
  //    0b10: 6-bit ASCII
  //    0b11: 8-bit ASCII
  //
  // Bits 5:0 - number of data bytes
  //

  TypeCode = (FruData[Offset] & 0xC0) >> 6;
  Length = FruData[Offset++] & 0x3F;

  if (Length == 0) {
    String = AllocateZeroPool (DataSize + 1);
    if (String == NULL) {
      return NULL;
    }

    String[0] = L'\0';
    *StartingOffset = Offset;
    return String;
  }

  switch (TypeCode) {
  case 0:
    // TODO: Support converting binary to string.
    DEBUG ((
      DEBUG_ERROR,
      "TypeCode 00b (Binary or unspecified) is unsupported!\n"
      ));
    return NULL;
    break;

  case 2:
    DataSize = ((((Length + 2) * 4) / 3) & ~0x03);
    break;

  case 1:
  case 3:
    DataSize = Length;
    break;

  default:
    ASSERT (FALSE);
    break;
  }

  String = AllocateZeroPool (DataSize + 1);
  if (String == NULL) {
    return NULL;
  }

  switch (TypeCode) {
  case 0:
    // TODO: Support converting binary to string.
    DEBUG ((
      DEBUG_ERROR,
      "TypeCode 00b (Binary or unspecified) is unsupported!\n"
      ));
    return NULL;
    break;

  case 1:
    for (Index1 = 0; Index1 < Length; Index1++) {
      String[Index1] = BcdPlus[(FruData[Offset + Index1] & 0x0F)];
    }
    String[Index1] = '\0';
    break;

  case 2:
    //
    // Convert 3 6-bit data (3 bytes) to 4 8-bit data (4 bytes) in turn.
    //
    for (Index1 = 0, Index2 = 0; Index1 < Length; Index1 += 3) {
      Index3 = ((Length - Index1) < 3) ? (Length - Index1) : 3;
      ZeroMem (&TempData, sizeof (TempData));
      CopyMem (&TempData, &FruData[Offset + Index1], Index3);
      for (Index3 = 0; Index3 < 4; Index3++) {
        //
        // Converting 6-bit ASCII to 8-bit ASCII has to be offset by 0x20
        //
        String[Index2++] = (TempData & 0x3F) + 0x20;
        TempData >>= 6;
      }
    }
    String[Index2] = '\0';
    break;

  case 3:
    CopyMem (String, &FruData[Offset], DataSize);
    String[DataSize] = '\0';
    break;

  default:
    ASSERT (FALSE);
    break;
  }

  Offset += Length;
  *StartingOffset = Offset;

  return String;
}

EFI_STATUS
InternalReadFruData (
  IN  UINT16 AreaOffset,
  IN  UINT8  Length,
  OUT UINT8  *Data
  )
{
  EFI_STATUS                  Status;
  IPMI_READ_FRU_DATA_REQUEST  FruDataRequest;
  IPMI_READ_FRU_DATA_RESPONSE *FruDataResponse;
  UINT8                       TempData[IPMI_FRU_RESPONSE_LENGTH_MAX + sizeof (IPMI_READ_FRU_DATA_RESPONSE)];
  UINT32                      ResponseSize;
  UINT16                      Offset;
  UINT16                      Finish;

  ASSERT (Data != NULL);

  Offset = AreaOffset;
  Finish = Offset + Length;

  do {
    FruDataRequest.DeviceId = FRU_DEVICE_ID_DEFAULT;
    FruDataRequest.InventoryOffset = Offset;

    if ((Finish - Offset) > IPMI_FRU_RESPONSE_LENGTH_MAX) {
      FruDataRequest.CountToRead = IPMI_FRU_RESPONSE_LENGTH_MAX;
    } else {
      FruDataRequest.CountToRead = Finish - Offset;
    }

    ResponseSize = sizeof (IPMI_READ_FRU_DATA_RESPONSE) + FruDataRequest.CountToRead;
    Status = IpmiReadFruData (&FruDataRequest, (IPMI_READ_FRU_DATA_RESPONSE *)TempData, &ResponseSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to read FRU data!\n", __FUNCTION__));
      ASSERT_EFI_ERROR (Status);
      break;
    }

    FruDataResponse = (IPMI_READ_FRU_DATA_RESPONSE *)TempData;

    if (FruDataResponse->CompletionCode > 0) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to request to read FRU data - Code Complete: 0x%x\n",
        __FUNCTION__,
        FruDataResponse->CompletionCode
        ));
      break;
    }

    CopyMem (Data, (VOID *)&(FruDataResponse->Data), FruDataResponse->CountReturned);

    Data += FruDataResponse->CountReturned;
    Offset += FruDataResponse->CountReturned;
  } while (Offset < Finish);

  return Status;
}

EFI_STATUS
UpdateFruPcds (
  VOID
  )
{
  EFI_STATUS             Status;
  UINT8                  AreaDataLength;
  UINT16                 StartingOffset;
  IPMI_FRU_COMMON_HEADER *FruCommonHeader;
  UINT8                  ChassisInfoAreaOffset;
  UINT8                  BoardInfoAreaOffset;
  UINT8                  ProductInfoAreaOffset;
  CHAR8                  *String;
  UINTN                  StringSize;
  UINT8                  FruData[FRU_AREA_LENGTH_MAX];
  UINT32                        SystemGuidResponseSize;
  IPMI_GET_SYSTEM_GUID_RESPONSE SystemGuidResponse;

  Status = InternalReadFruData (0, sizeof (IPMI_FRU_COMMON_HEADER), FruData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FruCommonHeader = (IPMI_FRU_COMMON_HEADER *)FruData;

  //
  // Area offset is multiples of 8 bytes
  //
  ChassisInfoAreaOffset = FruCommonHeader->ChassisInfoStartingOffset * 8;
  BoardInfoAreaOffset = FruCommonHeader->BoardAreaStartingOffset * 8;
  ProductInfoAreaOffset = FruCommonHeader->ProductInfoStartingOffset * 8;

  //
  // Read Chassis Info Area
  //
  ZeroMem (FruData, sizeof (FruData));
  Status = InternalReadFruData (ChassisInfoAreaOffset, 2, FruData);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    AreaDataLength = FruData[1] * 8;
    ASSERT ((AreaDataLength + sizeof (IPMI_READ_FRU_DATA_RESPONSE)) <= FRU_AREA_LENGTH_MAX);

    ZeroMem (FruData, sizeof (FruData));
    Status = InternalReadFruData (ChassisInfoAreaOffset, AreaDataLength, FruData);
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      StartingOffset = 3; /* Starting offset of Chassis Part Number */
      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruChassisPartNumber), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruChassisSerialNumber), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruChassisExtra), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }
    }
  }

  //
  // Read Board Info Area
  //
  ZeroMem (FruData, sizeof (FruData));
  Status = InternalReadFruData (BoardInfoAreaOffset, 2, FruData);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    AreaDataLength = FruData[1] * 8;
    ASSERT ((AreaDataLength + sizeof (IPMI_READ_FRU_DATA_RESPONSE)) <= FRU_AREA_LENGTH_MAX);

    ZeroMem (FruData, sizeof (FruData));
    Status = InternalReadFruData (BoardInfoAreaOffset, AreaDataLength, FruData);
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      StartingOffset = 6; /* Starting offset of Board Manufacturer */
      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruBoardManufacturerName), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruBoardProductName), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruBoardSerialNumber), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruBoardPartNumber), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }
    }
  }

  //
  // Read Product Info Area
  //
  ZeroMem (FruData, sizeof (FruData));
  Status = InternalReadFruData (ProductInfoAreaOffset, 2, FruData);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    AreaDataLength = FruData[1] * 8;
    ASSERT ((AreaDataLength + sizeof (IPMI_READ_FRU_DATA_RESPONSE)) <= FRU_AREA_LENGTH_MAX);

    ZeroMem (FruData, sizeof (FruData));
    Status = InternalReadFruData (ProductInfoAreaOffset, AreaDataLength, FruData);
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      StartingOffset = 3; /* Starting offset of Product Manufacturer */
      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruProductManufacturerName), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruProductName), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruProductPartNumber), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruProductVersion), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruProductSerialNumber), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruProductAssetTag), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruProductFruFileId), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }

      String = ConvertEncodedDataToString (FruData, &StartingOffset);
      ASSERT (String != NULL);
      if (String != NULL) {
        StringSize = AsciiStrSize (String);
        ASSERT (StringSize <= SMBIOS_STRING_MAX_LENGTH);

        CopyMem (PcdGetPtr (PcdFruProductExtra), (VOID *)String, StringSize);
        FreePool (String);
        String = NULL;
      }
    }
  }

  //
  // Read MultiRecord Area
  //
  SystemGuidResponseSize = sizeof (SystemGuidResponse);

  Status = IpmiSubmitCommand (
             IPMI_NETFN_APP,
             IPMI_APP_GET_SYSTEM_GUID,
             NULL,
             0,
             (UINT8 *)&SystemGuidResponse,
             &SystemGuidResponseSize
             );

  if (!EFI_ERROR (Status)) {
    CopyMem (PcdGetPtr (PcdFruSystemUniqueID), (VOID *)SystemGuidResponse.Guid, sizeof (EFI_GUID));
  } else {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read MultiRecord Area - %r\n", __FUNCTION__, Status));
  }

  return Status;
}

EFI_STATUS
IpmiReadFruInfo (
  VOID
  )
{
  EFI_STATUS                                Status;
  IPMI_GET_DEVICE_ID_RESPONSE               ControllerInfo;
  IPMI_GET_FRU_INVENTORY_AREA_INFO_REQUEST  GetFruInventoryAreaInfoRequest;
  IPMI_GET_FRU_INVENTORY_AREA_INFO_RESPONSE GetFruInventoryAreaInfoResponse;

  //
  //  Get all the SDR Records from BMC and retrieve the Record ID from the structure for future use.
  //
  Status = IpmiGetDeviceId (&ControllerInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: IpmiGetDeviceId Status=%x\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: FruInventorySupport=%x\n", __FUNCTION__, ControllerInfo.DeviceSupport.Bits.FruInventorySupport));

  if (ControllerInfo.DeviceSupport.Bits.FruInventorySupport) {
    GetFruInventoryAreaInfoRequest.DeviceId = FRU_DEVICE_ID_DEFAULT;
    Status = IpmiGetFruInventoryAreaInfo (&GetFruInventoryAreaInfoRequest, &GetFruInventoryAreaInfoResponse);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: IpmiGetFruInventoryAreaInfo Status=%x\n", __FUNCTION__, Status));
      return Status;
    }
    DEBUG ((DEBUG_INFO, "%a: InventoryAreaSize=%x\n", __FUNCTION__, GetFruInventoryAreaInfoResponse.InventoryAreaSize));
  }

  //
  // Retrieve FRU information from BMC via IPMI SSIF interface
  //
  Status = UpdateFruPcds ();

  return EFI_SUCCESS;
}

/** @file
  IPMI Ppi implementation follows IPMI 2.0 Specification.

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <IndustryStandard/IpmiNetFnApp.h>
#include <IndustryStandard/IpmiNetFnAppExt.h>
#include <Library/DebugLib.h>
#include <Library/SmbusLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/IpmiPpi.h>

#include "IpmiSsifCommon.h"

//
// Initialize SSIF Interface capability
//
BOOLEAN  mPecSupport         = FALSE;
UINT8    mMaxRequestSize     = IPMI_SSIF_BLOCK_LEN;
UINT8    mMaxResponseSize    = IPMI_SSIF_BLOCK_LEN;
UINT8    mTransactionSupport = SSIF_SINGLE_PART_RW;

/**
  This function enables submitting Ipmi command via Ssif interface.

  @param[in]         This              This point for PEI_IPMI_PPI structure.
  @param[in]         NetFunction       Net function of the command.
  @param[in]         Command           IPMI Command.
  @param[in]         RequestData       Command Request Data.
  @param[in]         RequestDataSize   Size of Command Request Data.
  @param[out]        ResponseData      Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize  Size of Command Response Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_NOT_FOUND          The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_NOT_READY          Ipmi Device is not ready for Ipmi command access.
  @retval EFI_DEVICE_ERROR       Ipmi Device hardware error.
  @retval EFI_TIMEOUT            The command time out.
  @retval EFI_UNSUPPORTED        The command was not successfully sent to the device.
  @retval EFI_OUT_OF_RESOURCES   The resource allcation is out of resource or data size error.
**/
EFI_STATUS
IpmiSsifCmd (
  IN     PEI_IPMI_PPI *This,
  IN     UINT8        NetFunction,
  IN     UINT8        Command,
  IN     UINT8        *RequestData,
  IN     UINT32       RequestDataSize,
  OUT    UINT8        *ResponseData,
  IN OUT UINT32       *ResponseDataSize
  )
{
  EFI_STATUS  Status;

  ASSERT (This != NULL);

  Status = IpmiSsifCommonCmd (NetFunction, Command, RequestData, RequestDataSize, ResponseData, ResponseDataSize);

  return Status;
}

//
// Interface defintion of IPMI Ppi.
//
PEI_IPMI_PPI  mIpmiSsifPpi = {
  IpmiSsifCmd
};

CONST EFI_PEI_PPI_DESCRIPTOR  mIpmiSsifPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiIpmiPpiGuid,
  &mIpmiSsifPpi
};

/**
  The user Entry Point for the Ssif driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeSsifPeim (
  IN       EFI_PEI_FILE_HANDLE FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_STATUS                                            Status;
  IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_REQUEST        Request;
  IPMI_GET_SYSTEM_INTERFACE_SSIF_CAPABILITIES_RESPONSE  SsifCap;
  UINT32                                                ResponseSize;

  Request.Uint8 = IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_INTERFACE_TYPE_SSIF;
  ResponseSize  = sizeof (SsifCap);

  //
  // Check for BMC SSIF capabilities
  //
  Status = mIpmiSsifPpi.IpmiSubmitCommand (
                          &mIpmiSsifPpi,
                          IPMI_NETFN_APP,
                          IPMI_APP_GET_SYSTEM_INTERFACE_CAPABILITIES,
                          (VOID *)&Request,
                          sizeof (Request),
                          (VOID *)&SsifCap,
                          &ResponseSize
                          );

  if (  !EFI_ERROR (Status)
     && (SsifCap.CompletionCode == 0))
  {
    mTransactionSupport = SsifCap.InterfaceCap.Bits.TransactionSupport;
    mPecSupport      = SsifCap.InterfaceCap.Bits.PecSupport;
    mMaxRequestSize  = SsifCap.InputMsgSize;
    mMaxResponseSize = SsifCap.OutputMsgSize;

    DEBUG ((
      DEBUG_INFO,
      "SSIF Capabilities transaction %d, insize %d, outsize %d, pec %d\n",
      mTransactionSupport,
      mMaxRequestSize,
      mMaxResponseSize,
      mPecSupport
      ));
  }

  //
  // TODO: Get global settings, check for Alert support.
  //

  //
  // Install IPMI Ppi
  //
  Status = PeiServicesInstallPpi (&mIpmiSsifPpiList);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** @file

  Copyright (c) 2020 - 2022, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/PlatformInfoHob.h>

#ifndef AMPERE_CPU_LIB_H_
#define AMPERE_CPU_LIB_H_

/* Ctypen, bits[3(n - 1) + 2 : 3(n - 1)], for n = 1 to 7 */
#define CLIDR_CTYPE_SHIFT(Level)    (3 * (Level - 1))
#define CLIDR_CTYPE_MASK(Level)     (7 << CLIDR_CTYPE_SHIFT(Level))
#define CLIDR_CTYPE(Clidr, Level) \
  (((Clidr) & CLIDR_CTYPE_MASK(Level)) >> CLIDR_CTYPE_SHIFT(Level))

#define CCSIDR_NUMSETS_SHIFT        13
#define CCSIDR_NUMSETS_MASK         0xFFFE000
#define CCSIDR_NUMSETS(Ccsidr) \
  (((Ccsidr) & CCSIDR_NUMSETS_MASK) >> CCSIDR_NUMSETS_SHIFT)
#define CCSIDR_ASSOCIATIVITY_SHIFT  3
#define CCSIDR_ASSOCIATIVITY_MASK   0x1FF8
#define CCSIDR_ASSOCIATIVITY(Ccsidr) \
  (((Ccsidr) & CCSIDR_ASSOCIATIVITY_MASK) >> CCSIDR_ASSOCIATIVITY_SHIFT)
#define CCSIDR_LINE_SIZE_SHIFT      0
#define CCSIDR_LINE_SIZE_MASK       0x7
#define CCSIDR_LINE_SIZE(Ccsidr) \
  (((Ccsidr) & CCSIDR_LINE_SIZE_MASK) >> CCSIDR_LINE_SIZE_SHIFT)

#define SUBNUMA_MODE_MONOLITHIC        0
#define SUBNUMA_MODE_HEMISPHERE        1
#define SUBNUMA_MODE_QUADRANT          2

#define MONOLITIC_NUM_OF_REGION        1
#define HEMISPHERE_NUM_OF_REGION       2
#define QUADRANT_NUM_OF_REGION         4

#define SOCKET_ID(CpuId)               ((CpuId) / (PLATFORM_CPU_MAX_CPM * PLATFORM_CPU_NUM_CORES_PER_CPM))
#define CLUSTER_ID(CpuId)              (((CpuId) / PLATFORM_CPU_NUM_CORES_PER_CPM) % PLATFORM_CPU_MAX_CPM)

#define MASTER_SOCKET                  0
#define SLAVE_SOCKET                   1

/**
  Get CPU Core order number.

  @return   UINT8      The order number.

**/
UINT32 *
CpuGetCoreOrder (
  VOID
  );

/**
  Get the SubNUMA mode.

  @return   UINT8      The SubNUMA mode.

**/
UINT8
EFIAPI
CpuGetSubNumaMode (
  VOID
  );

/**
  Get the number of SubNUMA region.

  @return   UINT8      The number of SubNUMA region.

**/
UINT8
EFIAPI
CpuGetNumberOfSubNumaRegion (
  VOID
  );

/**
  Get the SubNUMA node of a CPM.

  @param    SocketId    Socket index.
  @param    Cpm         CPM index.
  @return   UINT8       The SubNUMA node of a CPM.

**/
UINT8
EFIAPI
CpuGetSubNumNode (
  UINT8  Socket,
  UINT16 Cpm
  );

/**
  Get the associativity of cache.

  @param    Level       Cache level.
  @return   UINT32      Associativity of cache.

**/
UINT32
EFIAPI
CpuGetAssociativity (
  UINT32 Level
  );

/**
  Get the cache size.

  @param    Level       Cache level.
  @return   UINT32      Cache size.

**/
UINT32
EFIAPI
CpuGetCacheSize (
  UINT32 Level
  );

/**
  Get max CPU frequency.

  @param    SocketId   Socket index.
  @return   UINTN      Max CPU frequency.

**/
UINTN
EFIAPI
CpuGetMaxFreq (
  UINT8 SocketId
  );

/**
  Get current frequency of CPU.

  @return   UINTN   Current frequency of CPU.

**/
UINTN
EFIAPI
CpuGetCurrentFreq (
  VOID
  );

/**
  Get CPU voltage.

  @param    SocketId   Socket index.
  @return   UINT16     CPU voltage.

**/
UINT16
EFIAPI
CpuGetVoltage (
  UINT8 SocketId
  );

/**
  Get CPU Ecid.

  @param        SocketId   Socket index.
  @param[out]   Ecid       Pointer to contain Ecid value.

**/
VOID
EFIAPI
CpuGetEcid (
  UINT8  SocketId,
  UINT32 **Ecid
  );

/**
  Get max cores of socket.

  @param    SocketId   Socket index.
  @return   UINT8      Max cores of socket.

**/
UINT8
EFIAPI
GetSkuMaxCore (
  UINT8 SocketId
  );

/**
  Get max turbo of socket.

  @param    SocketId   Socket index.
  @return   UINT8      Max turbo of socket.

**/
UINT8
EFIAPI
GetSkuMaxTurbo (
  UINT8 SocketId
  );

/**
  Get the number of supported socket.

  @return   UINT8      Number of supported socket.

**/
UINT8
EFIAPI
GetNumberOfSupportedSockets (
  VOID
  );

/**
  Get the number of active socket.

  @return   UINT8      Number of active socket.

**/
UINT8
EFIAPI
GetNumberOfActiveSockets (
  VOID
  );

/**
  Get the number of active CPM per socket.

  @param    SocketId    Socket index.
  @return   UINT16      Number of CPM.

**/
UINT16
EFIAPI
GetNumberOfActiveCPMsPerSocket (
  UINT8 SocketId
  );

/**
  Get the number of configured CPM per socket.

  @param    SocketId    Socket index.
  @return   UINT16      Number of configured CPM.

**/
UINT16
EFIAPI
GetNumberOfConfiguredCPMs (
  UINT8 SocketId
  );

/**
  Set the number of configured CPM per socket.

  @param    SocketId        Socket index.
  @param    NumberOfCPMs    Number of CPM to be configured.
  @return   EFI_SUCCESS     Operation succeeded.
  @return   Others          An error has occurred.

**/
EFI_STATUS
EFIAPI
SetNumberOfConfiguredCPMs (
  UINT8  SocketId,
  UINT16 NumberOfCPMs
  );

/**
  Get the maximum number of core per socket. This number
  should be the same for all sockets.

  @return   UINT16      Maximum number of core.

**/
UINT16
EFIAPI
GetMaximumNumberOfCores (
  VOID
  );

/**
  Get the number of active cores of a sockets.

  @return   UINT16      Number of active core.

**/
UINT16
EFIAPI
GetNumberOfActiveCoresPerSocket (
  UINT8 SocketId
  );

/**
  Get the number of active cores of all socket.

  @return   UINT16      Number of active core.

**/
UINT16
EFIAPI
GetNumberOfActiveCores (
  VOID
  );

/**
  Get version of SCP.

  @param[out]   ScpVer   Pointer to contain version of SCP value.
**/
VOID
EFIAPI
GetScpVersion (
  UINT8 **ScpVer
  );

/**
  Get date of build release for SCP.

  @param[out]   ScpBuild   Pointer to contain date of build release for SCP.
**/
VOID
EFIAPI
GetScpBuild (
  UINT8 **ScpBuild
  );

/**
  Get information of DIMM List.

  @param[out]   DimmList   Pointer contains information of DIMM List.
**/
VOID
EFIAPI
GetDimmList (
  PLATFORM_DIMM_LIST **DimmList
  );

/**
  Get information of DRAM.

  @param[out]   DramInfo   Pointer contains information of DRAM.
**/
VOID
EFIAPI
GetDramInfo (
  PLATFORM_DRAM_INFO **DramInfo
  );

/**
  Check if the logical CPU is enabled or not.

  @param    CpuId       The logical Cpu ID. Started from 0.
  @return   BOOLEAN     TRUE if the Cpu enabled
                        FALSE if the Cpu disabled.

**/
BOOLEAN
EFIAPI
IsCpuEnabled (
  UINT16 CpuId
  );

/**
  Check if the slave socket is present

  @return   BOOLEAN     TRUE if the Slave Cpu is present
                        FALSE if the Slave Cpu is not present

**/
BOOLEAN
EFIAPI
IsSlaveSocketAvailable (
  VOID
  );

/**
  Check if the slave socket is active

  @return   BOOLEAN     TRUE if the Slave CPU Socket is active.
                        FALSE if the Slave CPU Socket is not active.

**/
BOOLEAN
EFIAPI
IsSlaveSocketActive (
  VOID
  );

/**
  Check if the CPU product ID is Ac01
  @return   BOOLEAN     TRUE if the Product ID is Ac01
                        FALSE otherwise.

**/
BOOLEAN
EFIAPI
IsAc01Processor (
  VOID
  );

#endif /* AMPERE_CPU_LIB_H_ */

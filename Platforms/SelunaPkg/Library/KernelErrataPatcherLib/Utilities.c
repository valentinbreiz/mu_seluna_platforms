/** @file

  Patches NTOSKRNL to not cause a SError when reading/writing ACTLR_EL1
  Patches NTOSKRNL to not cause a SError when reading/writing AMCNTENSET0_EL0
  Patches NTOSKRNL to not cause a bugcheck when attempting to use
  PSCI_MEMPROTECT Due to an issue in QHEE

  Based on https://github.com/SamuelTulach/rainbow

  Copyright (c) 2021 Samuel Tulach
  Copyright (c) 2022-2023 DuoWoA authors

  SPDX-License-Identifier: MIT

**/
#include "KernelErrataPatcherLib.h"

#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_NT_SIGNATURE 0x00004550

EFI_PHYSICAL_ADDRESS LocateWinloadBase(EFI_PHYSICAL_ADDRESS base, UINTN *size)
{
  if (base & (EFI_PAGE_SIZE - 1)) {
    base &= ~(EFI_PAGE_SIZE - 1);
    base += EFI_PAGE_SIZE;
  }

  // Scan backwards page by page looking for the MZ+PE header of the loader
  // image.  Stop at SCAN_MAX distance or the first physical page to avoid
  // scanning into unmapped memory and taking a Data Abort on hardware with
  // discontiguous physical memory regions (e.g. Qualcomm SoCs).
  EFI_PHYSICAL_ADDRESS scan_limit =
      (base > SCAN_MAX) ? (base - SCAN_MAX) : EFI_PAGE_SIZE;

  while (base >= EFI_PAGE_SIZE && base >= scan_limit) {
    if (*(UINT16 *)base == IMAGE_DOS_SIGNATURE) {
      UINT32               newBaseOffset = *(UINT32 *)(base + 0x3C);
      EFI_PHYSICAL_ADDRESS newBase       = base + newBaseOffset;

      if (*(UINT16 *)newBase == IMAGE_NT_SIGNATURE) {
        *size = *(UINT32 *)(newBase + 0x110);
        if (*size & (EFI_PAGE_SIZE - 1)) {
          *size &= ~(EFI_PAGE_SIZE - 1);
          *size += EFI_PAGE_SIZE;
        }
        return base;
      }
    }

    base -= EFI_PAGE_SIZE;
  }

  // No PE header found before reaching the bottom of addressable memory.
  *size = 0;
  return 0;
}

VOID CopyMemory(
    EFI_PHYSICAL_ADDRESS destination, EFI_PHYSICAL_ADDRESS source, UINTN size)
{
  UINT8 *dst = (UINT8 *)(destination);
  UINT8 *src = (UINT8 *)(source);
  for (UINTN i = 0; i < size; i++) {
    dst[i] = src[i];
  }
}

EFI_PHYSICAL_ADDRESS
FindPattern(EFI_PHYSICAL_ADDRESS baseAddress, UINTN size, const CHAR8 *pattern)
{
  EFI_PHYSICAL_ADDRESS firstMatch     = 0;
  const CHAR8         *currentPattern = pattern;

  for (EFI_PHYSICAL_ADDRESS current = baseAddress; current < baseAddress + size;
       current++) {
    UINT8 byte = currentPattern[0];
    if (!byte)
      return firstMatch;
    if (byte == '\?' ||
        *(UINT8 *)(current) == GET_BYTE(byte, currentPattern[1])) {
      if (!firstMatch)
        firstMatch = current;
      if (!currentPattern[2])
        return firstMatch;
      ((byte == '\?') ? (currentPattern += 2) : (currentPattern += 3));
    }
    else {
      currentPattern = pattern;
      firstMatch     = 0;
    }
  }

  return 0;
}
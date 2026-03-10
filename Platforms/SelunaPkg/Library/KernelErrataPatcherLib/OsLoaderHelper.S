/** @file

  Patches NTOSKRNL to not cause a SError when reading/writing ACTLR_EL1
  Patches NTOSKRNL to not cause a SError when reading/writing AMCNTENSET0_EL0
  Patches NTOSKRNL to not cause a bugcheck when attempting to use
  PSCI_MEMPROTECT Due to an issue in QHEE

  Copyright (c) 2022-2023 DuoWoA authors

  SPDX-License-Identifier: MIT

**/
// ExitBootServicesWrapper has been removed.
// This firmware only boots Limine; no ExitBootServices hook is needed.
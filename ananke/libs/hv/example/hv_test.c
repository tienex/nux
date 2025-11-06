/*++
    Module Name:

        hv_test.c

    Abstract:

        Example test program for the Ananke Hypervisor Framework.
        Demonstrates creating a VM, mapping memory, and running virtual CPUs.

--*/

#include <hv/hypervisor.h>
#include <ananke/ntrtl.h>
#include <stdio.h>

/* Test guest code (simple infinite loop with HLT) */
static const UINT8 g_TestGuestCode[] = {
    0xF4,  /* HLT */
    0xEB, 0xFD  /* JMP $-1 (infinite loop) */
};

int main(void)
{
    HRESULT hr;
    IHypervisor* pHypervisor = NULL;
    IVirtualMachine* pVM = NULL;
    IVirtualCpu* pCpu = NULL;
    IVirtualMemory* pMem = NULL;
    HV_VM_CONFIG config;
    HV_VM_EXIT_INFO exitInfo;
    HV_REGISTER_VALUE regValue;
    HV_ARCHITECTURE archs[32];
    UINT32 archCount = 32;
    UINT32 i;
    BOOLEAN supported;
    UINT8* guestMemory;

    printf("Ananke Hypervisor Framework Test\n");
    printf("=================================\n\n");

    /* Create hypervisor */
    printf("Creating hypervisor...\n");
    hr = HvCreateHypervisor(&IID_IHypervisor, (VOID**)&pHypervisor);
    if (FAILED(hr)) {
        printf("Failed to create hypervisor: 0x%08X\n", (unsigned int)hr);
        return 1;
    }

    /* Initialize hypervisor */
    printf("Initializing hypervisor...\n");
    hr = IHypervisor_Initialize(pHypervisor);
    if (FAILED(hr)) {
        printf("Failed to initialize hypervisor: 0x%08X\n", (unsigned int)hr);
        goto cleanup;
    }

    /* Enumerate supported architectures */
    printf("\nSupported architectures:\n");
    hr = IHypervisor_EnumerateArchitectures(pHypervisor, archs, &archCount);
    if (SUCCEEDED(hr)) {
        for (i = 0; i < archCount; i++) {
            const char* archName = "Unknown";
            switch (archs[i]) {
                case HV_ARCH_X86_286: archName = "Intel 80286"; break;
                case HV_ARCH_X86_32: archName = "x86 32-bit"; break;
                case HV_ARCH_X86_64: archName = "x86_64"; break;
                case HV_ARCH_RISCV32: archName = "RISC-V RV32"; break;
                case HV_ARCH_RISCV64: archName = "RISC-V RV64"; break;
                case HV_ARCH_MIPS32: archName = "MIPS 32-bit"; break;
                case HV_ARCH_MIPS64: archName = "MIPS 64-bit"; break;
                case HV_ARCH_SPARC32: archName = "SPARC 32-bit"; break;
                case HV_ARCH_SPARC64: archName = "SPARC 64-bit"; break;
                case HV_ARCH_M68K: archName = "Motorola 68K"; break;
                case HV_ARCH_VAX: archName = "DEC VAX"; break;
                case HV_ARCH_ALPHA: archName = "DEC Alpha"; break;
                case HV_ARCH_IA64: archName = "Intel IA-64"; break;
                case HV_ARCH_PPC32: archName = "PowerPC 32-bit"; break;
                case HV_ARCH_PPC64: archName = "PowerPC 64-bit"; break;
                case HV_ARCH_LOONGARCH32: archName = "LoongArch LA32"; break;
                case HV_ARCH_LOONGARCH64: archName = "LoongArch LA64"; break;
                case HV_ARCH_DLX: archName = "DLX"; break;
                case HV_ARCH_MMIX: archName = "MMIX"; break;
            }
            printf("  - %s (0x%04X)\n", archName, archs[i]);
        }
    }

    /* Check if x86_64 with software virtualization is supported */
    printf("\nChecking x86_64 software virtualization support...\n");
    hr = IHypervisor_IsSupported(pHypervisor, HV_ARCH_X86_64, HV_VIRT_SOFTWARE, &supported);
    if (SUCCEEDED(hr)) {
        printf("  x86_64 software virtualization: %s\n", supported ? "Yes" : "No");
    }

    /* Check if x86_64 with hardware virtualization is supported */
    hr = IHypervisor_IsSupported(pHypervisor, HV_ARCH_X86_64, HV_VIRT_HARDWARE, &supported);
    if (SUCCEEDED(hr)) {
        printf("  x86_64 hardware virtualization: %s\n", supported ? "Yes" : "No");
    }

    /* Check Intel 286 support */
    hr = IHypervisor_IsSupported(pHypervisor, HV_ARCH_X86_286, HV_VIRT_SOFTWARE, &supported);
    if (SUCCEEDED(hr)) {
        printf("  Intel 80286 support: %s\n", supported ? "Yes" : "No");
    }

    /* Configure VM */
    printf("\nCreating x86_64 virtual machine...\n");
    RtlZeroMemory(&config, sizeof(config));
    config.Architecture = HV_ARCH_X86_64;
    config.Endianness = HV_ENDIAN_LITTLE;
    config.VirtMode = HV_VIRT_AUTO;
    config.NumCpus = 1;
    config.MemorySize = 16 * 1024 * 1024;  /* 16MB */
    config.EnableNestedPaging = FALSE;
    config.EnableShadowPageTables = TRUE;
    config.EnableBinaryTranslation = FALSE;
    config.EnableParavirt = FALSE;

    /* Create VM */
    hr = IHypervisor_CreateVM(pHypervisor, &config, &pVM);
    if (FAILED(hr)) {
        printf("Failed to create VM: 0x%08X\n", (unsigned int)hr);
        goto cleanup;
    }
    printf("VM created successfully!\n");

    /* Get virtual memory interface */
    hr = IVirtualMachine_GetVirtualMemory(pVM, &pMem);
    if (FAILED(hr)) {
        printf("Failed to get virtual memory interface: 0x%08X\n", (unsigned int)hr);
        goto cleanup;
    }

    /* Allocate guest memory */
    guestMemory = (UINT8*)RtlAllocateMemory(NULL, 4096);
    if (guestMemory == NULL) {
        printf("Failed to allocate guest memory\n");
        goto cleanup;
    }
    RtlZeroMemory(guestMemory, 4096);

    /* Copy test code to guest memory */
    RtlCopyMemory(guestMemory, g_TestGuestCode, sizeof(g_TestGuestCode));

    /* Map guest physical memory */
    printf("Mapping guest memory...\n");
    hr = IVirtualMemory_MapMemory(pMem, 0, (UINT64)(UINTN)guestMemory, 4096,
        HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
    if (FAILED(hr)) {
        printf("Failed to map memory: 0x%08X\n", (unsigned int)hr);
        RtlFreeMemory(NULL, guestMemory);
        goto cleanup;
    }
    printf("Memory mapped at GPA 0x0000000000000000\n");

    /* Get CPU 0 */
    hr = IVirtualMachine_GetVirtualCpu(pVM, 0, &pCpu);
    if (FAILED(hr)) {
        printf("Failed to get virtual CPU: 0x%08X\n", (unsigned int)hr);
        goto cleanup;
    }

    /* Set instruction pointer to start of guest code */
    printf("\nSetting up CPU state...\n");
    hr = IVirtualCpu_SetInstructionPointer(pCpu, 0);
    if (FAILED(hr)) {
        printf("Failed to set instruction pointer: 0x%08X\n", (unsigned int)hr);
        goto cleanup;
    }

    /* Set up basic x86_64 state */
    RtlZeroMemory(&regValue, sizeof(regValue));
    regValue.u64 = 0x1000;  /* Initial stack */
    hr = IVirtualCpu_WriteRegister(pCpu, HV_X86_RSP, &regValue);

    regValue.u64 = 0x0010;  /* CR0: PE bit (protected mode) */
    hr = IVirtualCpu_WriteRegister(pCpu, HV_X86_CR0, &regValue);

    printf("CPU state initialized\n");

    /* Start VM */
    printf("\nStarting virtual machine...\n");
    hr = IVirtualMachine_Start(pVM);
    if (FAILED(hr)) {
        printf("Failed to start VM: 0x%08X\n", (unsigned int)hr);
        goto cleanup;
    }

    /* Run CPU */
    printf("Running virtual CPU...\n");
    hr = IVirtualCpu_Run(pCpu, &exitInfo);
    if (FAILED(hr)) {
        printf("Failed to run CPU: 0x%08X\n", (unsigned int)hr);
        goto cleanup;
    }

    /* Display exit information */
    printf("\nVM Exit Information:\n");
    printf("  Reason: ");
    switch (exitInfo.Reason) {
        case HV_EXIT_HLT:
            printf("HLT instruction\n");
            break;
        case HV_EXIT_IO_INSTRUCTION:
            printf("I/O instruction\n");
            break;
        case HV_EXIT_EXTERNAL_INTERRUPT:
            printf("External interrupt\n");
            break;
        case HV_EXIT_PAGE_FAULT:
            printf("Page fault\n");
            break;
        default:
            printf("Unknown (0x%08X)\n", exitInfo.Reason);
            break;
    }
    printf("  CPU ID: %u\n", exitInfo.CpuId);
    printf("  Guest Linear Address: 0x%016llX\n", (unsigned long long)exitInfo.GuestLinearAddress);
    printf("  Guest Physical Address: 0x%016llX\n", (unsigned long long)exitInfo.GuestPhysicalAddress);

    /* Read final RIP */
    UINT64 finalRip;
    hr = IVirtualCpu_GetInstructionPointer(pCpu, &finalRip);
    if (SUCCEEDED(hr)) {
        printf("  Final RIP: 0x%016llX\n", (unsigned long long)finalRip);
    }

    /* Stop VM */
    printf("\nStopping virtual machine...\n");
    IVirtualMachine_Stop(pVM);

    RtlFreeMemory(NULL, guestMemory);

cleanup:
    /* Cleanup */
    if (pMem != NULL) {
        IVirtualMemory_Release(pMem);
    }
    if (pCpu != NULL) {
        IVirtualCpu_Release(pCpu);
    }
    if (pVM != NULL) {
        IVirtualMachine_Release(pVM);
    }
    if (pHypervisor != NULL) {
        IHypervisor_Shutdown(pHypervisor);
        IHypervisor_Release(pHypervisor);
    }

    printf("\nTest completed successfully!\n");
    return 0;
}

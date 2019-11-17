#undef __STDC_VERSION__ /* Confuse efibind.h into doing the right thing. */
#include <efi.h>
#include <efilib.h>

#define EFI_MMAP_MAX 1024
static EFI_MEMORY_DESCRIPTOR mmap[EFI_MMAP_MAX];
static EFI_HANDLE image_handle;
static EFI_LOADED_IMAGE *img = NULL;

#if defined(__GNUC__)
#warning GNU C DEFINED
#endif

#if defined(__STDC_VERSION__)
#warning STDC VERSION DEFINED
#endif

unsigned long
get_page (void)
{
  EFI_STATUS efi_status;
  void *addr;
  //  printf ("get page\n");
  efi_status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiBootServicesData, 1, &addr);

  if ( EFI_ERROR(efi_status) ) {
    printf("Allocate Pages Failed: %d\n", efi_status);
    exit (-1);
  }

  memset (addr, 0, 4096);

  return (unsigned long)addr;
}

int
putchar (int c)
{
  Print(L"%c", c);
}

void *payload_start;
unsigned long payload_size;

static char buf[4 * 1024*1024];

EFI_STATUS
efi_getpayload (CHAR16 *name, void **ptr, unsigned long *size)
{
  EFI_STATUS rc;
  EFI_HANDLE hdl;
  SIMPLE_READ_FILE rdhdl;
  EFI_DEVICE_PATH *filepath;

  *ptr = buf;
  *size = sizeof(buf);

  filepath = FileDevicePath(img->DeviceHandle,L"kernel.elf");
  rc = OpenSimpleReadFile(FALSE, NULL, 0, &filepath, &hdl, &rdhdl);
  if (rc != EFI_SUCCESS)
    {
      Print ("OpenSimpleReadFile failed (%d)!\n", rc);
      return rc;
    }
  rc = ReadSimpleReadFile(rdhdl, 0, size, buf);
  if (rc != EFI_SUCCESS)
    {
      Print ("ReadSimpleReadFile failed (%d)!\n", rc);
      return rc;
    }
  CloseSimpleReadFile(rdhdl);

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  void *ptr;
  EFI_STATUS rc;
  EFI_GUID img_prot = LOADED_IMAGE_PROTOCOL;

  InitializeLib(ImageHandle, SystemTable);
  image_handle = ImageHandle;

  rc = uefi_call_wrapper(BS->OpenProtocol, 6, ImageHandle, &img_prot, &img,
			 ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if (rc != EFI_SUCCESS)
    {
      Print ("Open Protocol failed (%d)!\n", rc);
      return rc;
    }

  /*
    Get kernel payload.
  */
  rc = efi_getpayload (L"kernel.elf", &payload_start, &payload_size);

  UINTN mmap_size = EFI_MMAP_MAX;
  BS->GetMemoryMap (&mmap_size, mmap, NULL, NULL, NULL);

  apxh_main(0, 0);

  return EFI_SUCCESS;
}


void *
efi_payload_start (void)
{
  return payload_start;
}

unsigned long
efi_payload_size (void)
{
  return payload_size;
}

void 
efi_exit (int st)
{
}

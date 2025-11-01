# Zoo64 Archive Format Specification

**Version:** 1.0 Draft
**Date:** 2025-10-31
**Status:** Specification Draft

## 1. Introduction

Zoo64 is a modern archive format designed for maximum compression, data integrity, and comprehensive metadata preservation. It supports multiple compression strategies, digital signatures, and full filesystem metadata including ACLs, extended attributes, and alternate data streams.

### 1.1 Design Goals

- **Maximum Compression**: Multi-stage pipeline with seekable solid compression
- **Modern Metadata**: Preserve all filesystem metadata (ACLs, xattrs, ADS, timestamps)
- **Flexibility**: Support per-file and solid compression with seekable variants
- **Integrity**: File-level and archive-level digital signatures
- **Unicode**: Full UTF-8 path support with no length restrictions
- **Cross-Platform**: Defined binary format for portability
- **Component Model**: Designed as COM component for language interoperability

### 1.2 Key Features

- Variable-length UTF-8 paths (no 16-character limit)
- ACL (Access Control List) storage
- Extended attributes (xattrs)
- Alternate Data Streams (ADS)
- YAML metadata at archive and file level
- Solid compression with seekable blocks
- Per-file compression with seekable blocks
- File-level and archive-level digital signatures
- Multiple compression algorithms
- Block-based seeking via LEB128 offset tables

## 2. Archive Structure

### 2.1 Overall Layout

Zoo64 uses a redundant directory structure similar to ZIP format. Each file has both a local header (embedded with the file data) and a central directory entry (at the end of the archive). This provides fast scanning and recovery from corruption.

```
[Archive Header]
[Compression Mode Descriptor]
[Archive YAML Metadata] (optional)
[File Entries...]
  [Local File Header]         ← Redundant: full file metadata
  [File Metadata]
    [Binary Metadata Chunks] (ACL, xattr, ADS, etc.)
    [YAML Metadata] (optional)
  [Encryption Header] (optional)
  [File Data / Compressed Data]
  [File Signature] (optional)
[Central Directory]            ← Redundant: duplicates file metadata
  [Central Directory Header]
  [Central Directory Entries...]
[Archive Signature] (optional)
[End of Archive Marker]
```

### 2.1a Multi-Volume Support

Zoo64 supports splitting archives across multiple volumes (files) for storage on limited media or for easier distribution.

```
Volume 1:
  [Archive Header (Volume 1)]
  [Compression Mode Descriptor]
  [Archive YAML Metadata] (optional)
  [File Entries... (partial)]
  [Volume Footer]

Volume 2-N:
  [Volume Header]
  [File Entries... (continued)]
  [Volume Footer]

Last Volume:
  [Volume Header]
  [File Entries... (final)]
  [Central Directory]
  [Archive Signature] (optional)
  [End of Archive Marker]
```

### 2.2 Magic Numbers

```
Zoo64 Archive:        0x5A4F4F3634415243  ("ZOO64ARC")
Classic Zoo:          0xFDC4A7DC          (Classic Zoo 2.1 format)
Solid Block:          0x534F4C4944424C4B  ("SOLIDBLK")
File Entry:           0x46494C45454E5452  ("FILEENTR")
Local File Header:    0x4C4F43414C4844    ("LOCALHD ")
Metadata Block:       0x4D45544144415441  ("METADATA")
YAML Metadata:        0x59414D4C4D455441  ("YAMLMETA")
Encryption Header:    0x454E4352595054    ("ENCRYPT ")
Signature Block:      0x5349474E41545552  ("SIGNATUR")
Central Directory:    0x43454E5444495220  ("CENTDIR ")
Volume Header:        0x564F4C554D4548    ("VOLUMEH ")
Volume Footer:        0x564F4C554D4546    ("VOLUMEF ")
End of Archive:       0x454E444F46415243  ("ENDOFARC")
```

## 3. Archive Header

The archive header appears at the beginning of every Zoo64 archive (or first volume in multi-volume archives).

```c
typedef struct _ZOO64_ARCHIVE_HEADER {
  UINT64  Magic;              // 0x5A4F4F3634415243 ("ZOO64ARC")
  UINT16  MajorVersion;       // Format major version (1)
  UINT16  MinorVersion;       // Format minor version (0)
  UINT32  Flags;              // Archive flags
  UINT64  CreationTime;       // NTP extended format timestamp
  UINT64  ModificationTime;   // NTP extended format timestamp
  UINT32  CompressionMode;    // Compression mode identifier
  UINT32  FileCount;          // Number of files in archive (all volumes)
  UINT64  CentralDirOffset;   // Offset to central directory (in last volume)
  UINT64  ArchiveSize;        // Total archive size in bytes (all volumes)
  UINT32  BlockSize;          // Block size for seekable compression (power of 2)
  UINT64  YamlMetadataOffset; // Offset to archive YAML metadata (0 if none)
  UINT32  YamlMetadataSize;   // Size of archive YAML metadata
  UINT16  VolumeNumber;       // Volume number (0 for single archive, 1+ for multi-volume)
  UINT16  TotalVolumes;       // Total number of volumes (0 for single archive)
  UINT32  VolumeSize;         // Maximum size per volume (0 for single archive)
  UINT8   UUID[16];           // Archive UUID (same across all volumes)
} __attribute__((packed)) ZOO64_ARCHIVE_HEADER;
```

### 3.1a NTP Extended Format Timestamps

All timestamps in Zoo64 use **NTP extended format** (64-bit):

```
Bits 0-31:  Seconds since NTP epoch (1900-01-01 00:00:00 UTC)
Bits 32-63: Fraction of a second (1/2^32 second precision)
```

This provides:
- **Resolution**: ~232 picoseconds (2^-32 seconds)
- **Range**: 136 years per NTP era (wraps at 2^32 seconds)
- **Current Era**: Era 0 ends 2036-02-07, Era 1 begins 2036-02-08

**Conversion from Unix time to NTP extended format**:
```c
// Unix epoch (1970-01-01) is 2208988800 seconds after NTP epoch (1900-01-01)
#define NTP_OFFSET 2208988800ULL

UINT64 UnixToNTP(time_t unixSec, uint32_t nanoSec) {
  UINT64 ntpSec = (UINT64)unixSec + NTP_OFFSET;
  UINT64 ntpFrac = ((UINT64)nanoSec << 32) / 1000000000ULL;
  return (ntpSec << 32) | ntpFrac;
}
```

**Conversion from NTP extended format to Unix time**:
```c
void NTPToUnix(UINT64 ntpTime, time_t *unixSec, uint32_t *nanoSec) {
  UINT64 ntpSec = ntpTime >> 32;
  UINT64 ntpFrac = ntpTime & 0xFFFFFFFF;
  *unixSec = (time_t)(ntpSec - NTP_OFFSET);
  *nanoSec = (uint32_t)((ntpFrac * 1000000000ULL) >> 32);
}
```

### 3.1 Archive Flags

```
Bit 0:     Solid compression enabled
Bit 1:     Seekable blocks enabled
Bit 2:     Archive signature present
Bit 3:     Encrypted (archive-level encryption)
Bit 4:     UTF-8 strict mode
Bit 5:     Archive YAML metadata present
Bit 6:     Multi-volume archive
Bit 7:     Classic Zoo compatibility mode
Bit 8:     Extended metadata present
Bit 9:     ACLs preserved
Bit 10:    Extended attributes preserved
Bit 11:    Alternate data streams preserved
Bit 12:    YAML metadata in files
Bit 13:    Per-file encryption allowed
Bit 14:    Redundant directory (ZIP-like)
Bit 15:    Reserved
Bit 16-31: Reserved for future use
```

### 3.2 Compression Modes

```
0x0000: Uncompressed (stored)
0x0001: Per-file compression
0x0002: Per-file seekable compression
0x0100: Solid compression
0x0101: Solid seekable compression
0x0200: Adaptive (per-file for small, solid for large)
```

## 3.3 Volume Header

Appears at the start of volumes 2-N in multi-volume archives.

```c
typedef struct _ZOO64_VOLUME_HEADER {
  UINT64  Magic;              // 0x564F4C554D4548 ("VOLUMEH ")
  UINT16  VolumeNumber;       // Volume number (2, 3, 4...)
  UINT16  TotalVolumes;       // Total number of volumes
  UINT64  VolumeSize;         // Size of this volume
  UINT64  VolumeOffset;       // Offset in complete archive
  UINT32  CRC32;              // CRC32 of this volume
  UINT8   ArchiveUUID[16];    // Archive UUID (matches main header)
} __attribute__((packed)) ZOO64_VOLUME_HEADER;
```

## 3.4 Volume Footer

Appears at the end of volumes 1-(N-1) in multi-volume archives.

```c
typedef struct _ZOO64_VOLUME_FOOTER {
  UINT64  Magic;              // 0x564F4C554D4546 ("VOLUMEF ")
  UINT16  VolumeNumber;       // This volume number
  UINT16  NextVolumeNumber;   // Next volume number
  UINT64  BytesInVolume;      // Total bytes in this volume
  UINT32  CRC32;              // CRC32 of this volume
} __attribute__((packed)) ZOO64_VOLUME_FOOTER;
```

## 4. Compression Mode Descriptor

Describes the compression algorithm and parameters.

```c
typedef struct _ZOO64_COMPRESSION_DESC {
  UINT32  DescriptorSize;     // Size of this descriptor
  UINT32  Algorithm;          // Compression algorithm ID
  UINT32  WindowSize;         // Window size (4K - 1M)
  UINT32  BlockSize;          // Block size for seekable (power of 2)
  UINT32  Level;              // Compression level (0-9)
  UINT32  Flags;              // Algorithm-specific flags
  UINT8   Parameters[64];     // Algorithm-specific parameters
} __attribute__((packed)) ZOO64_COMPRESSION_DESC;
```

### 4.1 Compression Algorithms

```
0x0000: None (stored)
0x0001: BWT+MTF+RAD50RLE+LZ78+Range (default)
0x0002: LZ77
0x0003: LZ4
0x0004: ZSTD
0x0005: LZMA
0x0100: Custom (parameters in descriptor)
```

## 4.5 Archive YAML Metadata

The archive may contain YAML metadata that applies to the entire archive. This is optional and provides a flexible way to store arbitrary archive-level information.

### 4.5.1 YAML Metadata Block

```c
typedef struct _ZOO64_YAML_METADATA {
  UINT64  Magic;              // 0x59414D4C4D455441 ("YAMLMETA")
  UINT32  YamlSize;           // Size of YAML data in bytes
  UINT32  Flags;              // YAML metadata flags
  // Followed by YamlSize bytes of UTF-8 encoded YAML
} __attribute__((packed)) ZOO64_YAML_METADATA;
```

### 4.5.2 YAML Metadata Flags

```
Bit 0:     Compressed (YAML data is compressed)
Bit 1:     Schema validated
Bit 2-31:  Reserved
```

### 4.5.3 Archive YAML Examples

Archive-level YAML metadata can contain:

```yaml
# Archive description and metadata
archive:
  title: "Project Source Code Archive"
  description: "Complete source code for Project XYZ v2.0"
  author: "Development Team"
  license: "MIT"
  version: "2.0.1"
  build: 12345

# Archive-level tags and categories
tags:
  - source-code
  - release
  - production

# Custom application-specific data
custom:
  project_id: "xyz-2024"
  repository: "https://github.com/org/project"
  ci_build_url: "https://ci.example.com/build/12345"

# Archive creation context
creation:
  hostname: "build-server-01"
  username: "ci-user"
  tool: "zoo64-cli v1.0"
  environment: "production"
```

## 5. File Entry Format

Each file in the archive has the following structure:

```
[File Header]
[Variable-length UTF-8 Path]
[Metadata Block] (optional)
[Data Blocks]
[File Signature] (optional)
```

### 5.1 File Header

```c
typedef struct _ZOO64_FILE_HEADER {
  UINT64  Magic;              // 0x46494C45454E5452 ("FILEENTR")
  UINT32  HeaderSize;         // Total size of header + path
  UINT32  Flags;              // File flags
  UINT64  UncompressedSize;   // Original file size
  UINT64  CompressedSize;     // Compressed size (0 if stored)
  UINT64  DataOffset;         // Offset to file data
  UINT64  MetadataOffset;     // Offset to metadata (0 if none)
  UINT32  MetadataSize;       // Size of metadata block
  UINT16  PathLength;         // Length of UTF-8 path in bytes
  UINT16  CompressionMethod;  // Compression method for this file
  UINT32  CRC32;              // CRC32 of uncompressed data
  UINT64  SHA256[4];          // SHA-256 hash of uncompressed data
  UINT64  BirthTime;          // File birth/creation time (NTP extended format)
  UINT64  ModificationTime;   // File modification time (NTP extended format)
  UINT64  AccessTime;         // File access time (NTP extended format)
  UINT64  ChangeTime;         // File metadata change time (NTP extended format)
  UINT32  UID;                // User ID (Unix)
  UINT32  GID;                // Group ID (Unix)
  UINT32  Mode;               // File mode/permissions (Unix)
  UINT32  Attributes;         // Platform-specific attributes
} __attribute__((packed)) ZOO64_FILE_HEADER;
```

**File Timestamps**:
- **BirthTime**: File creation/birth time (when the file was first created)
- **ModificationTime**: Last data modification time (mtime)
- **AccessTime**: Last access time (atime)
- **ChangeTime**: Last metadata change time (ctime - permissions, owner, etc.)

All four timestamps use NTP extended format for maximum precision and consistency.

### 5.2 File Flags

```
Bit 0:     Compressed
Bit 1:     Seekable (has block table)
Bit 2:     Encrypted
Bit 3:     Signed
Bit 4:     Has metadata block
Bit 5:     Has ACLs
Bit 6:     Has extended attributes
Bit 7:     Has alternate data streams
Bit 8:     Symbolic link
Bit 9:     Hard link
Bit 10:    Directory
Bit 11:    Special file (device, socket, etc.)
Bit 12-15: Reserved
Bit 16-31: Platform-specific
```

### 5.3 Variable-Length UTF-8 Path

Immediately follows file header. Length specified in `PathLength` field.

```
[PathLength bytes of UTF-8 data]
```

Paths use forward slash (/) as separator, regardless of platform.

## 6. Metadata Block Format

Optional block containing extended filesystem metadata.

```c
typedef struct _ZOO64_METADATA_HEADER {
  UINT64  Magic;              // 0x4D45544144415441 ("METADATA")
  UINT32  TotalSize;          // Total size of metadata block
  UINT32  ChunkCount;         // Number of metadata chunks
} __attribute__((packed)) ZOO64_METADATA_HEADER;
```

### 6.1 Metadata Chunks

Each metadata chunk has the following format:

```c
typedef struct _ZOO64_METADATA_CHUNK {
  UINT32  ChunkType;          // Chunk type identifier
  UINT32  ChunkSize;          // Size of chunk data (excludes header)
  // Followed by chunk data
} __attribute__((packed)) ZOO64_METADATA_CHUNK;
```

### 6.2 Metadata Chunk Types

```
0x0001: ACL (Access Control List)
0x0002: Extended Attributes (xattrs)
0x0003: Alternate Data Streams
0x0004: Security Descriptor (Windows)
0x0005: Resource Fork (macOS)
0x0006: Extended timestamps
0x0007: File capabilities (Linux)
0x0008: SELinux context
0x0009: Custom metadata
0x000A: YAML metadata (file-level)
0x000B: macOS UUIDs (user/group)
0x000C: BSD flags
0x000D: Linux flags
0x000E: Windows attributes
0x000F: Hard link target
0x0010: Symbolic link target
```

### 6.3 ACL Format

Zoo64 supports multiple ACL formats in a single metadata chunk to preserve cross-platform ACL semantics.

```c
typedef struct _ZOO64_ACL_HEADER {
  UINT16  ACLType;            // ACL format type (NFS4, NT, macOS, POSIX)
  UINT16  EntryCount;         // Number of ACL entries
  UINT32  TotalSize;          // Total size of ACL data
  UINT32  Flags;              // ACL flags
  // Followed by ACL entries in format specified by ACLType
} __attribute__((packed)) ZOO64_ACL_HEADER;
```

#### 6.3.1 ACL Types

```
0x0001: POSIX ACL (POSIX.1e - user/group/other/mask)
0x0002: NFS4 ACL (NFSv4 ACLs - RFC 7530)
0x0003: NT ACL (Windows DACL/SACL with SIDs)
0x0004: macOS ACL (macOS extended ACLs - NFSv4-based)
```

**Note**: A file may have multiple ACL headers if it needs to preserve ACLs from multiple systems (e.g., when transferring between platforms). The primary ACL should be listed first.

#### 6.3.2 POSIX ACL Format

Traditional POSIX.1e ACLs (Linux, FreeBSD, etc.)

```c
typedef struct _ZOO64_POSIX_ACL_ENTRY {
  UINT16  Tag;                // ACL_USER_OBJ, ACL_USER, ACL_GROUP_OBJ, etc.
  UINT16  Permissions;        // rwx bits (3 bits: read=4, write=2, execute=1)
  UINT32  ID;                 // User/group ID (or -1 for _OBJ entries)
} __attribute__((packed)) ZOO64_POSIX_ACL_ENTRY;
```

POSIX ACL tags:
```
0x0001: ACL_USER_OBJ        // Owner permissions
0x0002: ACL_USER            // Named user
0x0004: ACL_GROUP_OBJ       // Owning group permissions
0x0008: ACL_GROUP           // Named group
0x0010: ACL_MASK            // Maximum permissions
0x0020: ACL_OTHER           // Other permissions
```

#### 6.3.3 NFS4 ACL Format

NFSv4 ACLs (RFC 7530) - used on Solaris, FreeBSD, NFSv4 exports

```c
typedef struct _ZOO64_NFS4_ACL_ENTRY {
  UINT32  Type;               // ALLOW, DENY, AUDIT, ALARM
  UINT32  Flags;              // Inheritance and other flags
  UINT32  AccessMask;         // Permission bits
  UINT16  WhoType;            // OWNER@, GROUP@, EVERYONE@, or named
  UINT16  WhoLength;          // Length of who string (0 for special)
  // Followed by:
  //   [WhoLength bytes: UTF-8 username/group] (if WhoType is named)
} __attribute__((packed)) ZOO64_NFS4_ACL_ENTRY;
```

NFS4 ACE types:
```
0x00000000: ACCESS_ALLOWED_ACE_TYPE
0x00000001: ACCESS_DENIED_ACE_TYPE
0x00000002: SYSTEM_AUDIT_ACE_TYPE
0x00000003: SYSTEM_ALARM_ACE_TYPE
```

NFS4 ACE flags:
```
0x00000001: FILE_INHERIT_ACE
0x00000002: DIRECTORY_INHERIT_ACE
0x00000004: NO_PROPAGATE_INHERIT_ACE
0x00000008: INHERIT_ONLY_ACE
0x00000010: SUCCESSFUL_ACCESS_ACE_FLAG
0x00000020: FAILED_ACCESS_ACE_FLAG
0x00000040: IDENTIFIER_GROUP
0x00000080: INHERITED_ACE
```

NFS4 access mask:
```
0x00000001: READ_DATA / LIST_DIRECTORY
0x00000002: WRITE_DATA / ADD_FILE
0x00000004: APPEND_DATA / ADD_SUBDIRECTORY
0x00000008: READ_NAMED_ATTRS
0x00000010: WRITE_NAMED_ATTRS
0x00000020: EXECUTE / TRAVERSE
0x00000040: DELETE_CHILD
0x00000080: READ_ATTRIBUTES
0x00000100: WRITE_ATTRIBUTES
0x00010000: DELETE
0x00020000: READ_ACL
0x00040000: WRITE_ACL
0x00080000: WRITE_OWNER
0x00100000: SYNCHRONIZE
```

NFS4 special identities (WhoType):
```
0x0001: OWNER@              // File owner
0x0002: GROUP@              // File group
0x0003: EVERYONE@           // All users
0x0004: NAMED_USER          // Specific user (WhoLength > 0)
0x0005: NAMED_GROUP         // Specific group (WhoLength > 0)
```

#### 6.3.4 NT ACL Format

Windows NT ACLs with Security Identifiers (SIDs)

```c
typedef struct _ZOO64_NT_ACL_ENTRY {
  UINT32  Type;               // ACCESS_ALLOWED, ACCESS_DENIED, AUDIT, etc.
  UINT32  Flags;              // Inheritance flags
  UINT32  AccessMask;         // Permission bits
  UINT16  SIDLength;          // Length of SID
  // Followed by:
  //   [SIDLength bytes: binary SID structure]
} __attribute__((packed)) ZOO64_NT_ACL_ENTRY;
```

NT ACE types:
```
0x00: ACCESS_ALLOWED_ACE_TYPE
0x01: ACCESS_DENIED_ACE_TYPE
0x02: SYSTEM_AUDIT_ACE_TYPE
0x03: SYSTEM_ALARM_ACE_TYPE (reserved)
0x04: ACCESS_ALLOWED_COMPOUND_ACE_TYPE (reserved)
0x05: ACCESS_ALLOWED_OBJECT_ACE_TYPE
0x06: ACCESS_DENIED_OBJECT_ACE_TYPE
0x07: SYSTEM_AUDIT_OBJECT_ACE_TYPE
0x08: SYSTEM_ALARM_OBJECT_ACE_TYPE (reserved)
```

NT ACE flags:
```
0x01: OBJECT_INHERIT_ACE
0x02: CONTAINER_INHERIT_ACE
0x04: NO_PROPAGATE_INHERIT_ACE
0x08: INHERIT_ONLY_ACE
0x10: INHERITED_ACE
0x40: SUCCESSFUL_ACCESS_ACE_FLAG
0x80: FAILED_ACCESS_ACE_FLAG
```

NT access mask (generic):
```
0x00000001: FILE_READ_DATA
0x00000002: FILE_WRITE_DATA
0x00000004: FILE_APPEND_DATA
0x00000008: FILE_READ_EA
0x00000010: FILE_WRITE_EA
0x00000020: FILE_EXECUTE
0x00000040: FILE_DELETE_CHILD
0x00000080: FILE_READ_ATTRIBUTES
0x00000100: FILE_WRITE_ATTRIBUTES
0x00010000: DELETE
0x00020000: READ_CONTROL
0x00040000: WRITE_DAC
0x00080000: WRITE_OWNER
0x00100000: SYNCHRONIZE
0x01000000: ACCESS_SYSTEM_SECURITY
0x10000000: GENERIC_ALL
0x20000000: GENERIC_EXECUTE
0x40000000: GENERIC_WRITE
0x80000000: GENERIC_READ
```

NT SID format (stored as-is):
```c
typedef struct _NT_SID {
  UINT8   Revision;           // Always 1
  UINT8   SubAuthorityCount;  // Number of sub-authorities (1-15)
  UINT8   Authority[6];       // 48-bit authority value
  UINT32  SubAuthority[];     // Variable number of 32-bit values
} __attribute__((packed)) NT_SID;
```

#### 6.3.5 macOS ACL Format

macOS extended ACLs (based on NFSv4 with macOS extensions)

```c
typedef struct _ZOO64_MACOS_ACL_ENTRY {
  UINT8   UUID[16];           // User/group UUID (128-bit)
  UINT32  Type;               // ALLOW, DENY
  UINT32  Flags;              // Inheritance flags
  UINT32  Permissions;        // Permission bits
  UINT32  Reserved;           // Reserved for future use
} __attribute__((packed)) ZOO64_MACOS_ACL_ENTRY;
```

macOS ACE types:
```
0x00000000: KAUTH_ACE_PERMIT
0x00000001: KAUTH_ACE_DENY
```

macOS ACE flags:
```
0x00000001: KAUTH_ACE_FILE_INHERIT
0x00000002: KAUTH_ACE_DIRECTORY_INHERIT
0x00000004: KAUTH_ACE_LIMIT_INHERIT
0x00000008: KAUTH_ACE_ONLY_INHERIT
0x00000010: KAUTH_ACE_SUCCESS
0x00000020: KAUTH_ACE_FAILURE
0x00000040: KAUTH_ACE_INHERITED
```

macOS permissions:
```
0x00000001: KAUTH_VNODE_READ_DATA
0x00000002: KAUTH_VNODE_LIST_DIRECTORY
0x00000004: KAUTH_VNODE_WRITE_DATA
0x00000008: KAUTH_VNODE_ADD_FILE
0x00000010: KAUTH_VNODE_EXECUTE
0x00000020: KAUTH_VNODE_SEARCH
0x00000040: KAUTH_VNODE_DELETE
0x00000080: KAUTH_VNODE_APPEND_DATA
0x00000100: KAUTH_VNODE_ADD_SUBDIRECTORY
0x00000200: KAUTH_VNODE_DELETE_CHILD
0x00000400: KAUTH_VNODE_READ_ATTRIBUTES
0x00000800: KAUTH_VNODE_WRITE_ATTRIBUTES
0x00001000: KAUTH_VNODE_READ_EXTATTRIBUTES
0x00002000: KAUTH_VNODE_WRITE_EXTATTRIBUTES
0x00004000: KAUTH_VNODE_READ_SECURITY
0x00008000: KAUTH_VNODE_WRITE_SECURITY
0x00010000: KAUTH_VNODE_TAKE_OWNERSHIP
0x00020000: KAUTH_VNODE_SYNCHRONIZE
0x00040000: KAUTH_VNODE_LINKTARGET
0x00080000: KAUTH_VNODE_CHECKIMMUTABLE
```

#### 6.3.6 Multiple ACL Storage

When a file has ACLs from multiple systems (e.g., during cross-platform archival), store them as separate ACL chunks or as multiple ACL headers within a single ACL chunk:

```
[ZOO64_METADATA_CHUNK: type=0x0001 (ACL)]
  [ZOO64_ACL_HEADER: type=NFS4]
    [NFS4 ACL entries...]
  [ZOO64_ACL_HEADER: type=NT]
    [NT ACL entries...]
```

This allows perfect preservation and restoration of ACLs regardless of source platform.

### 6.4 Extended Attributes Format

```c
typedef struct _ZOO64_XATTR {
  UINT16  NameLength;         // Length of attribute name
  UINT32  ValueLength;        // Length of attribute value
  UINT16  Namespace;          // Namespace (user, system, security, trusted)
  // Followed by:
  //   [NameLength bytes: UTF-8 name]
  //   [ValueLength bytes: binary value]
} __attribute__((packed)) ZOO64_XATTR;
```

### 6.5 Alternate Data Streams Format

```c
typedef struct _ZOO64_ADS {
  UINT16  StreamNameLength;   // Length of stream name
  UINT64  StreamSize;         // Size of stream data
  UINT32  StreamFlags;        // Stream flags
  // Followed by:
  //   [StreamNameLength bytes: UTF-8 stream name]
  //   [StreamSize bytes: stream data]
} __attribute__((packed)) ZOO64_ADS;
```

### 6.6 File-Level YAML Metadata Format

File-level YAML metadata provides flexible, extensible metadata for individual files.

```c
typedef struct _ZOO64_FILE_YAML {
  UINT32  YamlSize;           // Size of YAML data
  UINT32  Flags;              // YAML flags (compressed, validated, etc.)
  // Followed by YamlSize bytes of UTF-8 YAML
} __attribute__((packed)) ZOO64_FILE_YAML;
```

#### File YAML Examples

```yaml
# File-specific metadata
file:
  original_path: "/usr/local/bin/myapp"
  purpose: "Main application binary"
  version: "2.1.4"

# Build information
build:
  compiler: "gcc 11.2.0"
  flags: "-O2 -Wall"
  date: "2024-10-31"

# Custom application data
tags:
  - executable
  - production

checksums:
  md5: "abc123..."
  sha1: "def456..."
```

### 6.6a Extended Timestamps Format

For platforms that support additional timestamps beyond the standard four (birth, modification, access, change), or to store higher-precision timestamps than NTP extended format provides.

```c
typedef struct _ZOO64_EXTENDED_TIMESTAMPS {
  UINT64  BirthTime;          // File birth/creation time (NTP extended format)
  UINT64  ModificationTime;   // Data modification time (NTP extended format)
  UINT64  AccessTime;         // Last access time (NTP extended format)
  UINT64  ChangeTime;         // Metadata change time (NTP extended format)
  UINT64  BackupTime;         // Last backup time (NTP extended format)
  UINT64  ArchivedTime;       // Time archived (NTP extended format)
  UINT32  Flags;              // Timestamp flags
  UINT32  Reserved;           // Reserved for future use
} __attribute__((packed)) ZOO64_EXTENDED_TIMESTAMPS;
```

**Note**: Extended timestamps chunk is optional and used only when:
1. Additional timestamps (backup, archived) are present
2. Platform has timestamps not covered by the file header
3. Need to store original timestamps alongside normalized timestamps

Standard file header already contains the four primary timestamps (birth, modification, access, change) in NTP extended format. This metadata chunk is for edge cases requiring additional timestamp metadata.

Timestamp flags:
```
Bit 0:     BirthTime valid
Bit 1:     ModificationTime valid
Bit 2:     AccessTime valid
Bit 3:     ChangeTime valid
Bit 4:     BackupTime valid
Bit 5:     ArchivedTime valid
Bit 6-31:  Reserved
```

### 6.7 macOS UUIDs Format

macOS stores UUIDs for users and groups alongside numeric IDs.

```c
typedef struct _ZOO64_MACOS_UUID {
  UINT8   UserUUID[16];       // User UUID (128-bit)
  UINT8   GroupUUID[16];      // Group UUID (128-bit)
  UINT32  Flags;              // Reserved
} __attribute__((packed)) ZOO64_MACOS_UUID;
```

### 6.8 BSD Flags Format

BSD systems use file flags for immutability, append-only, etc.

```c
typedef struct _ZOO64_BSD_FLAGS {
  UINT32  UserFlags;          // User-settable flags
  UINT32  SystemFlags;        // System/super-user flags
} __attribute__((packed)) ZOO64_BSD_FLAGS;
```

#### BSD Flag Definitions

```
User Flags:
  UF_NODUMP      0x00000001  // Do not dump file
  UF_IMMUTABLE   0x00000002  // File may not be changed
  UF_APPEND      0x00000004  // Writes to file may only append
  UF_OPAQUE      0x00000008  // Directory is opaque (union)
  UF_HIDDEN      0x00008000  // File is hidden (macOS)

System Flags:
  SF_ARCHIVED    0x00010000  // File is archived
  SF_IMMUTABLE   0x00020000  // File may not be changed
  SF_APPEND      0x00040000  // Writes to file may only append
```

### 6.9 Linux Flags Format

Linux file attributes (chattr/lsattr).

```c
typedef struct _ZOO64_LINUX_FLAGS {
  UINT32  Flags;              // Linux file attributes
  UINT32  Version;            // File version (for ext2/3/4)
} __attribute__((packed)) ZOO64_LINUX_FLAGS;
```

#### Linux Flag Definitions

```
FS_SECRM_FL        0x00000001  // Secure deletion
FS_UNRM_FL         0x00000002  // Undelete
FS_COMPR_FL        0x00000004  // Compress file
FS_SYNC_FL         0x00000008  // Synchronous updates
FS_IMMUTABLE_FL    0x00000010  // Immutable file
FS_APPEND_FL       0x00000020  // Append only
FS_NODUMP_FL       0x00000040  // Do not dump file
FS_NOATIME_FL      0x00000080  // Do not update atime
FS_NOCOW_FL        0x00800000  // No copy-on-write (Btrfs)
```

### 6.10 Windows Attributes Format

Extended Windows file attributes.

```c
typedef struct _ZOO64_WINDOWS_ATTR {
  UINT32  FileAttributes;     // Windows file attributes
  UINT32  ReparseTag;         // Reparse point tag (if applicable)
  UINT32  EaSize;             // Extended attributes size
  // Followed by EA data if EaSize > 0
} __attribute__((packed)) ZOO64_WINDOWS_ATTR;
```

#### Windows Attribute Definitions

```
FILE_ATTRIBUTE_READONLY             0x00000001
FILE_ATTRIBUTE_HIDDEN               0x00000002
FILE_ATTRIBUTE_SYSTEM               0x00000004
FILE_ATTRIBUTE_DIRECTORY            0x00000010
FILE_ATTRIBUTE_ARCHIVE              0x00000020
FILE_ATTRIBUTE_DEVICE               0x00000040
FILE_ATTRIBUTE_NORMAL               0x00000080
FILE_ATTRIBUTE_TEMPORARY            0x00000100
FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
FILE_ATTRIBUTE_COMPRESSED           0x00000800
FILE_ATTRIBUTE_OFFLINE              0x00001000
FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
FILE_ATTRIBUTE_ENCRYPTED            0x00004000
```

### 6.11 Hard Link Target Format

Stores information about hard links. Multiple files in the archive can point to the same inode.

```c
typedef struct _ZOO64_HARDLINK {
  UINT64  InodeNumber;        // Inode number (for grouping hard links)
  UINT64  DeviceId;           // Device ID (for uniqueness)
  UINT16  TargetPathLength;   // Length of target path
  // Followed by UTF-8 target path (first occurrence of this inode in archive)
} __attribute__((packed)) ZOO64_HARDLINK;
```

**Note**: The first file with a given inode contains the actual data. Subsequent hard links reference the first file's path and contain no data themselves.

### 6.12 Symbolic Link Target Format

Stores symbolic link target path.

```c
typedef struct _ZOO64_SYMLINK {
  UINT16  TargetPathLength;   // Length of target path
  UINT32  Flags;              // Symlink flags
  // Followed by UTF-8 target path
} __attribute__((packed)) ZOO64_SYMLINK;
```

#### Symbolic Link Flags

```
Bit 0:     Absolute path (vs relative)
Bit 1:     Directory target
Bit 2:     Broken link (target doesn't exist)
Bit 3-31:  Reserved
```

## 6a. Encryption Support

Zoo64 supports both archive-level and per-file encryption using modern authenticated encryption algorithms.

### 6a.1 Encryption Header

Appears before encrypted file data (per-file encryption) or after compression descriptor (archive-level encryption).

```c
typedef struct _ZOO64_ENCRYPTION_HEADER {
  UINT64  Magic;              // 0x454E4352595054 ("ENCRYPT ")
  UINT32  HeaderSize;         // Size of this header including all fields
  UINT16  EncryptionMethod;   // Encryption algorithm
  UINT16  KeyDerivation;      // Key derivation function
  UINT32  Iterations;         // KDF iteration count
  UINT16  SaltLength;         // Length of salt
  UINT16  IVLength;           // Length of IV/nonce
  UINT32  TagLength;          // Length of authentication tag
  UINT32  EncryptedSize;      // Size of encrypted data
  UINT32  Flags;              // Encryption flags
  // Followed by:
  //   [SaltLength bytes: random salt for KDF]
  //   [IVLength bytes: initialization vector/nonce]
  //   [Encrypted data]
  //   [TagLength bytes: authentication tag]
} __attribute__((packed)) ZOO64_ENCRYPTION_HEADER;
```

### 6a.2 Encryption Methods

```
0x0000: None (not encrypted)
0x0001: AES-256-GCM (recommended)
0x0002: AES-256-CBC + HMAC-SHA256
0x0003: ChaCha20-Poly1305
0x0004: AES-128-GCM
0x0005: Twofish-256-GCM
0x0006: Serpent-256-GCM
```

### 6a.3 Key Derivation Functions

```
0x0000: None (use key directly - not recommended)
0x0001: PBKDF2-HMAC-SHA256 (compatible, moderate security)
0x0002: PBKDF2-HMAC-SHA512
0x0003: Argon2id (recommended - memory-hard)
0x0004: scrypt
0x0005: bcrypt
```

### 6a.4 Encryption Flags

```
Bit 0:     Compress before encrypt (default)
Bit 1:     Encrypt then compress (not recommended)
Bit 2:     Store password hint (followed by hint string)
Bit 3:     Use key file (in addition to password)
Bit 4:     Header encryption (encrypt file names/metadata)
Bit 5-31:  Reserved
```

### 6a.5 Key Derivation Parameters

For **PBKDF2**:
- Salt: 16-32 bytes random
- Iterations: 600,000+ (OWASP 2023 recommendation)
- Output: 256 bits for AES-256, 128 bits for AES-128

For **Argon2id** (recommended):
- Salt: 16 bytes random
- Memory: 64 MB (65536 KB)
- Iterations: 3-4
- Parallelism: 4 threads
- Output: 256 bits

For **scrypt**:
- Salt: 32 bytes random
- N: 2^17 (131072) - CPU/memory cost
- r: 8 - block size
- p: 1 - parallelization
- Output: 256 bits

### 6a.6 Archive-Level vs File-Level Encryption

**Archive-Level Encryption:**
- Entire archive (except headers) encrypted as single stream
- Central directory encrypted
- File names encrypted (if header encryption enabled)
- Most secure against metadata leakage
- Cannot extract individual files without full decryption

**File-Level Encryption:**
- Each file encrypted independently
- Central directory not encrypted (file names visible)
- Allows selective file extraction
- Better for large archives where partial access is needed

**Hybrid Mode:**
- Files encrypted individually
- Central directory encrypted separately
- Balances security and selective access

### 6a.7 Password Verification

To verify password without full decryption:

```c
typedef struct _ZOO64_PASSWORD_VERIFY {
  UINT32  VerifyMethod;       // Verification method
  UINT16  VerifyDataLength;   // Length of verification data
  // Followed by verification data
} __attribute__((packed)) ZOO64_PASSWORD_VERIFY;
```

Verification methods:
```
0x0001: Encrypted known plaintext (16 bytes zeros encrypted)
0x0002: HMAC of password + salt
0x0003: Argon2 hash of password
```

## 7. Seekable Compression

### 7.1 Block Table Format

For seekable compression, a block table precedes the compressed data.

```c
typedef struct _ZOO64_BLOCK_TABLE {
  UINT32  Magic;              // 0x424C4B54424C ("BLKTBL")
  UINT32  BlockCount;         // Number of blocks
  UINT32  BlockSize;          // Uncompressed block size (power of 2)
  UINT32  Flags;              // Block table flags
  // Followed by BlockCount entries of block offsets in LEB128 format
} __attribute__((packed)) ZOO64_BLOCK_TABLE;
```

### 7.2 LEB128 Offset Encoding

Block offsets are stored in LEB128 (Little Endian Base 128) format for space efficiency.

Each offset is the **delta** from the previous offset, stored as LEB128.

Example:
```
Block 0: offset 1000  → LEB128(1000)
Block 1: offset 1500  → LEB128(500)
Block 2: offset 2100  → LEB128(600)
```

### 7.3 Block Format

Each compressed block:
```
[Block Header]
[Compressed Data]
```

```c
typedef struct _ZOO64_BLOCK_HEADER {
  UINT32  UncompressedSize;   // Size before compression
  UINT32  CompressedSize;     // Size after compression
  UINT32  CRC32;              // CRC32 of uncompressed block
} __attribute__((packed)) ZOO64_BLOCK_HEADER;
```

## 8. Solid Compression

### 8.1 Solid Block Format

```c
typedef struct _ZOO64_SOLID_BLOCK {
  UINT64  Magic;              // 0x534F4C4944424C4B ("SOLIDBLK")
  UINT32  FileCount;          // Number of files in solid block
  UINT64  UncompressedSize;   // Total uncompressed size
  UINT64  CompressedSize;     // Total compressed size
  UINT32  WindowSize;         // Compression window size
  UINT32  Flags;              // Solid block flags
  // Followed by:
  //   [File offsets table] - LEB128 encoded offsets
  //   [Compressed data]
} __attribute__((packed)) ZOO64_SOLID_BLOCK;
```

### 8.2 Solid Seekable Format

Combines solid compression with block table for random access:

```
[Solid Block Header]
[File Offsets Table] (LEB128)
[Block Table] (LEB128 block offsets)
[Compressed Blocks...]
```

## 9. Digital Signatures

### 9.1 Signature Block

```c
typedef struct _ZOO64_SIGNATURE {
  UINT64  Magic;              // 0x5349474E41545552 ("SIGNATUR")
  UINT32  SignatureSize;      // Total size of signature block
  UINT16  SignatureType;      // Signature algorithm
  UINT16  HashAlgorithm;      // Hash algorithm used
  UINT64  SigningTime;        // Signing timestamp (NTP extended format)
  UINT32  CertificateSize;    // Size of certificate (0 if none)
  UINT32  SignatureDataSize;  // Size of signature data
  // Followed by:
  //   [CertificateSize bytes: X.509 certificate]
  //   [SignatureDataSize bytes: signature data]
} __attribute__((packed)) ZOO64_SIGNATURE;
```

### 9.2 Signature Types

```
0x0001: RSA-2048
0x0002: RSA-4096
0x0003: ECDSA-P256
0x0004: ECDSA-P384
0x0005: Ed25519
0x0006: Ed448
```

### 9.3 Hash Algorithms

```
0x0001: SHA-256
0x0002: SHA-384
0x0003: SHA-512
0x0004: SHA3-256
0x0005: SHA3-512
0x0006: BLAKE2b
0x0007: BLAKE3
```

### 9.4 File-Level Signature

Appears after file data, hash covers uncompressed file data.

### 9.5 Archive-Level Signature

Appears at end of archive, hash covers entire archive except signature block itself.

## 10. Central Directory

The central directory provides fast access to all files in the archive.

```c
typedef struct _ZOO64_CENTRAL_DIR {
  UINT64  Magic;              // 0x43454E5444495220 ("CENTDIR ")
  UINT32  EntryCount;         // Number of entries
  UINT64  DirectorySize;      // Size of central directory
  UINT32  Flags;              // Directory flags
  // Followed by EntryCount directory entries
} __attribute__((packed)) ZOO64_CENTRAL_DIR;
```

### 10.1 Central Directory Entry

```c
typedef struct _ZOO64_CENTRAL_ENTRY {
  UINT64  FileHeaderOffset;   // Offset to file header
  UINT64  UncompressedSize;   // File size
  UINT64  CompressedSize;     // Compressed size (0 if stored)
  UINT32  CRC32;              // CRC32 of file
  UINT16  PathLength;         // Length of path
  UINT16  Flags;              // Entry flags
  // Followed by UTF-8 path
} __attribute__((packed)) ZOO64_CENTRAL_ENTRY;
```

## 11. End of Archive Marker

```c
typedef struct _ZOO64_END_OF_ARCHIVE {
  UINT64  Magic;              // 0x454E444F46415243 ("ENDOFARC")
  UINT64  ArchiveSize;        // Total archive size
  UINT64  CentralDirOffset;   // Offset to central directory
  UINT32  FileCount;          // Total files in archive
  UINT32  CRC32;              // CRC32 of central directory
  UINT8   SHA256[32];         // SHA-256 of entire archive (excl. this block)
} __attribute__((packed)) ZOO64_END_OF_ARCHIVE;
```

## 12. Compression Pipeline

### 12.1 Default Pipeline (Algorithm 0x0001)

For text/structured data:
```
Input → BWT → MTF → RAD50RLE → Windowed LZ78 → Range Encoding → Output
```

### 12.2 Pipeline Stages

1. **BWT (Burrows-Wheeler Transform)**: Groups similar characters
2. **MTF (Move-To-Front)**: Converts repeated chars to small values
3. **RAD50RLE**: RAD-50 encoding + RLE + LEB128 + bit transposition
4. **Windowed LZ78**: Dictionary compression with configurable window
5. **Range Encoding**: Adaptive arithmetic encoding

### 12.3 Window Sizes

Supported window sizes (power of 2):
- 4K, 8K, 16K, 32K, 64K (default), 128K, 256K, 512K, 1M

### 12.4 Block Sizes for Seekable

Supported block sizes (power of 2):
- 4K, 8K, 16K, 32K, 64K, 128K (default), 256K, 512K, 1M, 2M, 4M

## 13. COM Component Interface

Zoo64 is designed as a COM component for cross-language interoperability.

### 13.1 IZoo64Archive Interface

```cpp
interface IZoo64Archive : IUnknown {
  // Archive operations
  HRESULT Open([in] BSTR path, [in] DWORD mode);
  HRESULT Create([in] BSTR path, [in] ZOO64_ARCHIVE_OPTIONS* options);
  HRESULT Close();

  // File operations
  HRESULT AddFile([in] BSTR sourcePath, [in] BSTR archivePath,
                  [in] ZOO64_FILE_OPTIONS* options);
  HRESULT ExtractFile([in] BSTR archivePath, [in] BSTR destPath);
  HRESULT RemoveFile([in] BSTR archivePath);

  // Enumeration
  HRESULT GetFileCount([out] DWORD* count);
  HRESULT GetFileInfo([in] DWORD index, [out] ZOO64_FILE_INFO* info);

  // Compression
  HRESULT SetCompressionMode([in] DWORD mode);
  HRESULT SetWindowSize([in] DWORD size);
  HRESULT SetBlockSize([in] DWORD size);

  // Signatures
  HRESULT SignArchive([in] IZoo64SigningKey* key);
  HRESULT VerifyArchive([out] BOOL* valid);
  HRESULT SignFile([in] BSTR archivePath, [in] IZoo64SigningKey* key);
  HRESULT VerifyFile([in] BSTR archivePath, [out] BOOL* valid);

  // Metadata
  HRESULT GetFileMetadata([in] BSTR archivePath,
                          [out] IZoo64Metadata** metadata);
  HRESULT SetFileMetadata([in] BSTR archivePath,
                          [in] IZoo64Metadata* metadata);

  // YAML Metadata
  HRESULT GetArchiveYaml([out] BSTR* yaml);
  HRESULT SetArchiveYaml([in] BSTR yaml);
  HRESULT GetFileYaml([in] BSTR archivePath, [out] BSTR* yaml);
  HRESULT SetFileYaml([in] BSTR archivePath, [in] BSTR yaml);

  // Encryption
  HRESULT SetPassword([in] BSTR password);
  HRESULT SetKeyFile([in] BSTR keyFilePath);
  HRESULT SetEncryptionMethod([in] WORD method, [in] WORD kdf);
  HRESULT EncryptArchive([in] BOOL headerEncryption);
  HRESULT EncryptFile([in] BSTR archivePath);
  HRESULT DecryptFile([in] BSTR archivePath, [in] BSTR password);
  HRESULT VerifyPassword([out] BOOL* valid);

  // Multi-Volume
  HRESULT SetVolumeSize([in] UINT64 sizeBytes);
  HRESULT GetVolumeCount([out] DWORD* count);
  HRESULT GetVolumeInfo([in] DWORD volumeNumber, [out] ZOO64_VOLUME_INFO* info);
  HRESULT MergeVolumes([in] BSTR outputPath);

  // Classic Zoo Compatibility
  HRESULT ConvertFromClassicZoo([in] BSTR classicZooPath, [in] BSTR outputPath);
  HRESULT ConvertToClassicZoo([in] BSTR outputPath, [in] BOOL dualFormat);
  HRESULT IsClassicZoo([in] BSTR path, [out] BOOL* isClassic);
};
```

### 13.2 IZoo64Metadata Interface

```cpp
interface IZoo64Metadata : IUnknown {
  // ACLs
  HRESULT GetACL([out] IZoo64ACL** acl);
  HRESULT SetACL([in] IZoo64ACL* acl);

  // Extended Attributes
  HRESULT GetXattr([in] BSTR name, [out] VARIANT* value);
  HRESULT SetXattr([in] BSTR name, [in] VARIANT value);
  HRESULT EnumXattrs([out] IEnumVARIANT** enumerator);

  // Alternate Data Streams
  HRESULT GetADS([in] BSTR streamName, [out] IStream** stream);
  HRESULT SetADS([in] BSTR streamName, [in] IStream* stream);
  HRESULT EnumADS([out] IEnumVARIANT** enumerator);

  // Timestamps
  HRESULT GetTimestamps([out] ZOO64_TIMESTAMPS* timestamps);
  HRESULT SetTimestamps([in] ZOO64_TIMESTAMPS* timestamps);

  // Permissions
  HRESULT GetPermissions([out] DWORD* mode);
  HRESULT SetPermissions([in] DWORD mode);

  // YAML Metadata
  HRESULT GetYaml([out] BSTR* yaml);
  HRESULT SetYaml([in] BSTR yaml);

  // Platform-specific
  HRESULT GetMacOSUUIDs([out] ZOO64_MACOS_UUID* uuids);
  HRESULT SetMacOSUUIDs([in] ZOO64_MACOS_UUID* uuids);
  HRESULT GetBSDFlags([out] ZOO64_BSD_FLAGS* flags);
  HRESULT SetBSDFlags([in] ZOO64_BSD_FLAGS* flags);
  HRESULT GetLinuxFlags([out] ZOO64_LINUX_FLAGS* flags);
  HRESULT SetLinuxFlags([in] ZOO64_LINUX_FLAGS* flags);
  HRESULT GetWindowsAttributes([out] ZOO64_WINDOWS_ATTR* attr);
  HRESULT SetWindowsAttributes([in] ZOO64_WINDOWS_ATTR* attr);

  // Links
  HRESULT GetHardLinkTarget([out] BSTR* targetPath);
  HRESULT SetHardLinkTarget([in] BSTR targetPath, [in] UINT64 inodeNumber);
  HRESULT GetSymbolicLinkTarget([out] BSTR* targetPath, [out] DWORD* flags);
  HRESULT SetSymbolicLinkTarget([in] BSTR targetPath, [in] DWORD flags);
};
```

## 14. Platform-Specific Considerations

### 14.1 Unix/Linux

- Store full POSIX permissions in `Mode` field
- Store UID/GID
- Preserve xattrs (user, system, security, trusted namespaces)
- SELinux contexts in metadata
- File capabilities

### 14.2 Windows

- Convert NTFS permissions to ACL metadata
- Store security descriptors
- Preserve alternate data streams
- Store file attributes (hidden, system, archive, etc.)
- Store reparse point information for symlinks

### 14.3 macOS

- Store resource forks in ADS metadata
- Preserve xattrs (com.apple.* namespace)
- Store Finder info
- HFS+ compression flags

## 15. Error Handling

### 15.1 Error Codes

```
ZOO64_OK                    = 0x00000000
ZOO64_E_INVALID_MAGIC       = 0x80040001
ZOO64_E_UNSUPPORTED_VERSION = 0x80040002
ZOO64_E_CORRUPT_HEADER      = 0x80040003
ZOO64_E_CORRUPT_DATA        = 0x80040004
ZOO64_E_DECOMPRESSION_ERROR = 0x80040005
ZOO64_E_SIGNATURE_INVALID   = 0x80040006
ZOO64_E_ENCRYPTION_ERROR    = 0x80040007
ZOO64_E_FILE_NOT_FOUND      = 0x80040008
ZOO64_E_INVALID_PATH        = 0x80040009
ZOO64_E_ACCESS_DENIED       = 0x8004000A
```

## 16. Implementation Notes

### 16.1 Byte Order

All multi-byte integers are stored in **little-endian** format.

### 16.2 Alignment

Structures are packed (no padding). Use `__attribute__((packed))` or `#pragma pack(1)`.

### 16.3 String Encoding

All paths and text metadata use **UTF-8** encoding.

### 16.4 Checksums

- CRC32 uses IEEE polynomial (0xEDB88320)
- SHA-256 for file integrity
- Additional hash in signature blocks

### 16.5 Compression Reset Points

For seekable compression, compressor state is reset at each block boundary to enable independent decompression.

## 17. Classic Zoo Format Compatibility

Zoo64 can read and convert classic Zoo 2.1 archives while providing bidirectional compatibility options.

### 17.1 Classic Zoo Format Overview

The original Zoo format (created by Rahul Dhesi, 1986-1991) used:
- Magic: 0xFDC4A7DC (little-endian: 0xDCA7C4FD)
- 13-character filenames (MS-DOS 8.3 + path)
- LZH and LZD compression
- Generation numbers for versioning
- Directory entries with deleted file tracking

### 17.2 Reading Classic Zoo Archives

When Zoo64 encounters classic Zoo magic (0xFDC4A7DC):

```c
typedef struct _CLASSIC_ZOO_HEADER {
  UINT32  Magic;              // 0xFDC4A7DC
  UINT32  FirstEntryOffset;   // Offset to first file entry
  UINT32  MinusOffset;        // Negative offset to last entry (-)
  UINT8   MajorVersion;       // Major version (2)
  UINT8   MinorVersion;       // Minor version (1)
  // Additional header fields...
} __attribute__((packed)) CLASSIC_ZOO_HEADER;
```

### 17.3 Classic Zoo Directory Entry

```c
typedef struct _CLASSIC_ZOO_ENTRY {
  UINT32  Magic;              // 0xFDC4A7DC
  UINT8   CompressionMethod;  // 0=stored, 1=LZD, 2=LZH
  UINT8   NextEntryOffset;    // Offset to next entry (variable)
  UINT32  OriginalSize;       // Uncompressed size
  UINT32  CompressedSize;     // Compressed size
  UINT16  Date;               // MS-DOS date format
  UINT16  Time;               // MS-DOS time format
  UINT16  CRC;                // CRC-16
  UINT32  OriginalPosition;   // Original file position
  UINT16  Attributes;         // File attributes
  UINT8   Generation;         // Generation number
  UINT8   Deleted;            // Deletion marker
  char    FileName[13];       // Null-terminated filename
  char    Comment[?];         // Variable-length comment
  // Followed by compressed data
} __attribute__((packed)) CLASSIC_ZOO_ENTRY;
```

### 17.4 Conversion: Classic Zoo → Zoo64

When converting classic Zoo to Zoo64:

1. **Archive Header**: Create Zoo64 header with compatibility flag set
2. **Filenames**: Convert 13-char filenames to UTF-8 paths
3. **Compression**:
   - **Option 1**: Decompress LZH/LZD, recompress with Zoo64 pipeline
   - **Option 2**: Store compressed data as-is with compression method marker
4. **Timestamps**: Convert MS-DOS date/time to NTP extended format
5. **Metadata**: Store generation numbers in YAML metadata
6. **CRC**: Upgrade CRC-16 to CRC-32 + SHA-256
7. **Attributes**: Convert to appropriate platform metadata

#### 17.4.1 MS-DOS to NTP Timestamp Conversion

Classic Zoo stores timestamps in MS-DOS date/time format (16-bit date + 16-bit time):

```c
// MS-DOS date format (16 bits)
// Bits 0-4:   Day (1-31)
// Bits 5-8:   Month (1-12)
// Bits 9-15:  Year (0 = 1980, 127 = 2107)

// MS-DOS time format (16 bits)
// Bits 0-4:   Seconds/2 (0-29)
// Bits 5-10:  Minutes (0-59)
// Bits 11-15: Hours (0-23)

UINT64 DOSTimeToNTP(UINT16 dosDate, UINT16 dosTime) {
  // Extract fields
  int day   = dosDate & 0x1F;
  int month = (dosDate >> 5) & 0x0F;
  int year  = ((dosDate >> 9) & 0x7F) + 1980;
  int sec   = (dosTime & 0x1F) * 2;
  int min   = (dosTime >> 5) & 0x3F;
  int hour  = (dosTime >> 11) & 0x1F;

  // Convert to Unix timestamp (use mktime or equivalent)
  struct tm t = {0};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  time_t unixTime = mktime(&t);

  // Convert to NTP extended format
  return UnixToNTP(unixTime, 0);
}
```

**Note**: MS-DOS timestamps have 2-second precision and no timezone information (assumed local time). When converting to Zoo64, record both ModificationTime and BirthTime as the same value since classic Zoo doesn't distinguish creation vs modification time.

### 17.5 Conversion: Zoo64 → Classic Zoo (Limited)

For compatibility with legacy tools:

**Restrictions:**
- Filenames truncated to 13 characters (8.3 DOS format)
- Paths flattened (directory separators converted to underscores)
- Compression converted to LZH or stored
- Metadata discarded (except what fits in comment field)
- File size limit: 4 GB (32-bit size fields)
- Timestamps truncated to 2-second precision (MS-DOS format)
- Timestamps after 2107 cannot be represented

**Process:**
```
1. Check constraints (filename length, file size, compression type)
2. Warn user about metadata loss
3. Decompress Zoo64 data
4. Recompress with LZH (if beneficial) or store
5. Convert NTP timestamps to MS-DOS date/time
6. Generate classic Zoo directory entries
7. Write classic Zoo format
```

#### 17.5.1 NTP to MS-DOS Timestamp Conversion

```c
void NTPToDOSTime(UINT64 ntpTime, UINT16 *dosDate, UINT16 *dosTime) {
  // Convert NTP to Unix time
  time_t unixSec;
  uint32_t nanoSec;
  NTPToUnix(ntpTime, &unixSec, &nanoSec);

  // Convert to local time
  struct tm *t = localtime(&unixSec);

  // Validate range (1980-2107)
  if (t->tm_year + 1900 < 1980) {
    // Clamp to minimum (1980-01-01 00:00:00)
    *dosDate = 0x0021;  // 1980-01-01
    *dosTime = 0x0000;  // 00:00:00
    return;
  }
  if (t->tm_year + 1900 > 2107) {
    // Clamp to maximum (2107-12-31 23:59:58)
    *dosDate = 0xFF9F;  // 2107-12-31
    *dosTime = 0xBF7D;  // 23:59:58
    return;
  }

  // Encode MS-DOS date
  *dosDate = ((t->tm_year + 1900 - 1980) << 9) |
             ((t->tm_mon + 1) << 5) |
             t->tm_mday;

  // Encode MS-DOS time (2-second precision)
  *dosTime = (t->tm_hour << 11) |
             (t->tm_min << 5) |
             (t->tm_sec / 2);
}
```

### 17.6 Dual-Format Archives

Zoo64 supports creating "dual-format" archives readable by both Zoo64 and classic Zoo:

```
[Classic Zoo Header]
[Classic Zoo Entries...] (limited 13-char filenames)
[Classic Zoo Data]
[Zoo64 Extended Header] (magic: 0x5A4F4F3634415243)
[Zoo64 Entries...] (full metadata, long filenames)
[Zoo64 Central Directory]
[End of Archive]
```

**Behavior:**
- Classic Zoo tools: Read first header, see classic format archive
- Zoo64 tools: Detect extended header, use Zoo64 format with full metadata
- File data stored once, referenced by both formats

### 17.7 Classic Zoo Compression Methods

For compatibility, Zoo64 can optionally support classic compression:

```
Method 0: Stored (no compression)
Method 1: LZD (Lempel-Ziv with dynamic Huffman)
Method 2: LZH (Lempel-Ziv + static Huffman, similar to LHA)
```

Implemented via compatibility shims:
```c
BOOLEAN ClassicZooDecompress(UINT8 method, const UINT8 *input, ...);
BOOLEAN ClassicZooCompress(UINT8 method, const UINT8 *input, ...);
```

### 17.8 Migration Strategy

Recommended migration path from classic Zoo to Zoo64:

1. **Phase 1**: Read-only compatibility
   - Zoo64 tools can extract classic Zoo archives
   - No modification of classic archives

2. **Phase 2**: Conversion tool
   - Provide `zoo64conv` utility
   - Convert classic → Zoo64 with full metadata preservation
   - Optionally create dual-format archives

3. **Phase 3**: Native Zoo64
   - Create new archives in Zoo64 format
   - Maintain classic Zoo reading capability for legacy data

### 17.9 Compatibility Mode Flag

When archive header flag bit 7 (Classic Zoo compatibility mode) is set:
- Enforce 13-character filename limit
- Disable UTF-8 strict mode
- Store filenames in uppercase (DOS convention)
- Generate both classic and Zoo64 directory entries
- Use MS-DOS timestamp formats

## 18. Version History

- **1.0** (2025-10-31): Initial specification draft
  - Basic archive structure with redundant directories
  - BWT+MTF+RAD50RLE+LZ78+Range compression pipeline
  - Variable-length UTF-8 paths
  - ACL, xattr, ADS metadata support
  - YAML metadata (archive and file level)
  - Digital signatures (file and archive level)
  - Seekable compression with block tables
  - Solid compression support
  - COM component interfaces

- **1.1** (2025-11-01): Enhanced features
  - Platform-specific metadata (macOS UUIDs, BSD/Linux flags, Windows attributes)
  - Hard link and symbolic link support
  - Encryption support (AES-256-GCM, ChaCha20-Poly1305, Argon2id KDF)
  - Multi-volume archive support
  - Classic Zoo format compatibility
  - Redundant directory structure (ZIP-like)
  - Archive-level and file-level encryption
  - Password verification mechanisms
  - Dual-format archives (Zoo64 + Classic Zoo)

## 19. Future Extensions

Reserved areas for future features:
- Delta compression (binary diff between versions)
- Content-based deduplication (hash-based)
- Error correction codes (Reed-Solomon, LDPC)
- Compression algorithm plugins (dynamic loading)
- Streaming compression (no seekable table)
- Archive repair and recovery tools
- Incremental backups with change tracking
- Network-transparent archives (HTTP range requests)
- Archive virtualization (mount as filesystem)
- Smart compression (ML-based algorithm selection)

---

**End of Specification**

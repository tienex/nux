# DCL Shell - Digital Command Language Shell for NUX

## Overview

The DCL (Digital Command Language) Shell is a VMS-style command interpreter implemented for the NUX kernel using the ananke COM infrastructure. It features syntax coloring, a client/server architecture for extensible command parsing, and portable message-based communication.

## Features

- **Syntax Coloring**: Real-time ANSI escape code-based syntax highlighting
- **COM Architecture**: Built entirely on ananke's COM interfaces (IUnknown, HRESULT, etc.)
- **Client/Server Design**: Applications can register custom syntax providers via message bus
- **Portable**: Abstract messaging layer allows for cross-platform communication
- **Extensible**: Plugin-based syntax registration system
- **VMS DCL Compatible**: Supports classic DCL keywords and commands

## Architecture

### COM Interfaces

The DCL shell is built using five main COM interfaces:

#### 1. IDcl - Main Shell Interface
The aggregator interface that provides access to all subsystems:
```c
extern IDcl *gpDcl;  // Global shell instance

HRESULT Initialize();
HRESULT Shutdown();
HRESULT Run();
HRESULT ExecuteCommand(const CHAR8 *Command);
IDclRenderer* GetRendererInterface();
IDclSyntaxProvider* GetSyntaxProviderInterface();
IDclParser* GetParserInterface();
IDclMessageBus* GetMessageBusInterface();
```

#### 2. IDclRenderer - Rendering Backend
Abstract rendering interface with ANSI escape code implementation:
```c
HRESULT Initialize();
HRESULT SetColor(DCL_COLOR Foreground, DCL_COLOR Background);
HRESULT ResetColor();
HRESULT WriteText(const CHAR8 *Text, UINTN Length);
HRESULT WriteToken(const DCL_SYNTAX_TOKEN *Token);
HRESULT NewLine();
HRESULT ClearScreen();
HRESULT MoveCursor(UINTN X, UINTN Y);
```

#### 3. IDclSyntaxProvider - Syntax Registration
Client/server syntax provider system:
```c
HRESULT RegisterSyntax(const CHAR8 *Name, VOID *SyntaxData);
HRESULT UnregisterSyntax(const CHAR8 *Name);
HRESULT GetSyntax(const CHAR8 *Name, VOID **SyntaxData);
HRESULT EnumerateSyntaxes(UINTN *Count, CHAR8 ***Names);
```

#### 4. IDclParser - Command Parser
DCL command parsing with syntax analysis:
```c
HRESULT ParseLine(const CHAR8 *Line, UINTN Length,
                  DCL_SYNTAX_TOKEN **Tokens, UINTN *TokenCount);
HRESULT FreeTokens(DCL_SYNTAX_TOKEN *Tokens, UINTN TokenCount);
HRESULT SetSyntaxProvider(IDclSyntaxProvider *Provider);
```

#### 5. IDclMessageBus - Client/Server Communication
Portable message bus for inter-component communication:
```c
HRESULT Initialize();
HRESULT SendMessage(const DCL_MESSAGE *Message, DCL_MESSAGE **Response);
HRESULT RegisterHandler(DCL_MESSAGE_TYPE MessageType,
                        HRESULT (*Handler)(const DCL_MESSAGE*, DCL_MESSAGE**));
HRESULT UnregisterHandler(DCL_MESSAGE_TYPE MessageType);
```

### Syntax Elements

The parser recognizes these syntax types with color coding:

| Type | Color | Example |
|------|-------|---------|
| `DCL_SYNTAX_KEYWORD` | Bright Cyan | `IF`, `THEN`, `ELSE` |
| `DCL_SYNTAX_COMMAND` | Bright Green | `SHOW`, `SET`, `DIR` |
| `DCL_SYNTAX_STRING` | Bright Yellow | `"text"` |
| `DCL_SYNTAX_NUMBER` | Bright Magenta | `123`, `456` |
| `DCL_SYNTAX_COMMENT` | Bright Black | `! comment` |
| `DCL_SYNTAX_OPERATOR` | Bright Red | `=`, `+`, `-` |
| `DCL_SYNTAX_VARIABLE` | Bright Blue | `$variable` |
| `DCL_SYNTAX_ERROR` | Red | Parse errors |

### Built-in Keywords

```
IF, THEN, ELSE, ENDIF, GOTO, GOSUB, RETURN, EXIT, ON, CALL,
READ, WRITE, INQUIRE, OPEN, CLOSE
```

### Built-in Commands

```
HELP, DIR, DIRECTORY, SHOW, SET, CREATE, DELETE, COPY, RENAME,
TYPE, PRINT, RUN, SPAWN, LOGOUT, DEFINE, ASSIGN, DEASSIGN,
MOUNT, DISMOUNT, ALLOCATE, DEALLOCATE, SUBMIT, PURGE, SEARCH,
EDIT, DUMP, ANALYZE, BACKUP, RESTORE
```

## Usage

### Basic Usage

```c
#include <dcl/dcl.h>

/* Method 1: Using legacy API */
dcl_initialize();
dcl_run();
dcl_shutdown();

/* Method 2: Using COM interfaces */
IDcl *pDcl;
DclCreateShell(&pDcl);
pDcl->lpVtbl->Initialize(pDcl);
pDcl->lpVtbl->Run(pDcl);
pDcl->lpVtbl->Shutdown(pDcl);
pDcl->lpVtbl->Release((IUnknown *)pDcl);
```

### Executing Commands

```c
/* Execute a command with syntax coloring */
dcl_execute_command("SHOW DIRECTORY sys$login");
```

### Registering Custom Syntax

Applications can register custom syntax providers for domain-specific languages:

```c
IDclSyntaxProvider *pProvider;
IDclMessageBus *pMessageBus;

/* Get interfaces */
gpDcl->lpVtbl->GetSyntaxProviderInterface(gpDcl, &pProvider);
gpDcl->lpVtbl->GetMessageBusInterface(gpDcl, &pMessageBus);

/* Register custom syntax */
MY_SYNTAX_DATA *customSyntax = ...;
pProvider->lpVtbl->RegisterSyntax(pProvider, "mysyntax", customSyntax);

/* Or use message bus for remote registration */
DCL_MESSAGE msg = {
    .MessageType = DCL_MSG_REGISTER_SYNTAX,
    .DataSize = sizeof(MY_SYNTAX_DATA),
    .Data = customSyntax
};

pMessageBus->lpVtbl->SendMessage(pMessageBus, &msg, NULL);
```

## Build System Integration

The DCL library is built as part of the NUX kernel library collection:

### Directory Structure
```
nux/libs/dcl/
├── include/
│   ├── public/dcl/
│   │   └── dcl.h              # Public COM interfaces
│   └── private/dcl/
│       └── internal.h         # Internal implementation details
├── sources/
│   ├── renderer/
│   │   └── ansi.c             # ANSI escape code renderer
│   ├── syntax/
│   │   └── provider.c         # Syntax provider implementation
│   ├── parser/
│   │   └── parser.c           # DCL command parser
│   ├── messaging/
│   │   └── messagebus.c       # Message bus for client/server
│   └── shell/
│       ├── helpers.c          # Helper functions and keywords
│       └── shell.c            # Main shell implementation
├── Makefile.in                # Build configuration
└── README.md                  # This file
```

### Makefile.in
```makefile
@COMPILE_LIBNUX@

LIBDIR=lib
LIBRARY=dcl

SRCS+= sources/renderer/ansi.c
SRCS+= sources/syntax/provider.c
SRCS+= sources/parser/parser.c
SRCS+= sources/messaging/messagebus.c
SRCS+= sources/shell/helpers.c
SRCS+= sources/shell/shell.c

INCDIR=include/public/dcl
INCS+=$(addprefix include/public/dcl/,dcl.h)
```

## Architecture Benefits

### Portability
- **Abstract Messaging**: Message bus allows different communication backends
- **Platform-Independent**: Core logic doesn't depend on specific HAL implementation
- **Configurable Rendering**: Renderer interface can be replaced (VGA, serial, framebuffer)

### Extensibility
- **Plugin Architecture**: Applications register syntax providers dynamically
- **Message-Based**: Client/server design allows remote syntax registration
- **Modular**: Each component (renderer, parser, syntax) is independently replaceable

### Performance
- **Zero-Copy Tokens**: Parser returns pointers into original command buffer
- **Static Allocation**: Uses fixed-size buffers (configurable via internal.h)
- **Atomic Operations**: Thread-safe reference counting via ananke atomics

## Configuration

Key limits can be adjusted in `include/private/dcl/internal.h`:

```c
#define DCL_MAX_COMMAND_LENGTH 1024       // Maximum command line length
#define DCL_MAX_TOKENS 256                // Maximum tokens per command
#define DCL_MAX_SYNTAX_PROVIDERS 64       // Maximum registered syntaxes
#define DCL_MAX_MESSAGE_HANDLERS 32       // Maximum message handlers
```

## Integration with NUX Kernel

The DCL shell integrates with NUX through:

1. **HAL Interface**: Uses `IHalCpu::PutChar()` for output via `gpHal`
2. **Memory Management**: Static allocation (can be extended to use `INuxKmem`)
3. **COM Infrastructure**: Built entirely on ananke's COM types and interfaces

### Example Integration

See `nux/example/kern/dcl_demo.c` for a complete demonstration.

## Message Bus Protocol

Applications communicate with the shell using structured messages:

### Message Types
- `DCL_MSG_REGISTER_SYNTAX` - Register new syntax provider
- `DCL_MSG_UNREGISTER_SYNTAX` - Unregister syntax provider
- `DCL_MSG_PARSE_REQUEST` - Request command parsing
- `DCL_MSG_PARSE_RESPONSE` - Return parsed tokens
- `DCL_MSG_RENDER_REQUEST` - Request rendering
- `DCL_MSG_RENDER_RESPONSE` - Rendering complete

### Message Structure
```c
typedef struct _DCL_MESSAGE {
    UINT32 MessageId;
    UINT32 MessageType;
    UINTN DataSize;
    VOID *Data;
} DCL_MESSAGE;
```

## Future Enhancements

Potential areas for expansion:

1. **Command History**: Add readline-style history with up/down arrows
2. **Tab Completion**: Implement command and filename completion
3. **Script Execution**: Support DCL script (.COM) file execution
4. **Variables**: Full DCL symbol/variable support
5. **Procedures**: GOSUB/RETURN and procedure execution
6. **I/O Redirection**: Support for pipes and redirections
7. **Networking**: Remote shell access via TCP/IP
8. **Security**: Add authentication and command authorization

## License

SPDX-License-Identifier: BSD-2-Clause

Copyright (C) 2025 NUX Project

## References

- VMS DCL Documentation: https://vmssoftware.com/docs/dcl/
- ANSI Escape Codes: https://en.wikipedia.org/wiki/ANSI_escape_code
- NUX COM Architecture: See `/COM_ARCHITECTURE.md` in repository root
- Ananke Foundation: See `/ananke/include/ananke/ananke.h`

# YAML Reader/Writer Component

A lightweight, COM-based YAML parser and serializer for the Ananke foundation library.

## Features

- **COM Interface-Based**: Full IUnknown-based interface with QueryInterface, AddRef, Release
- **Zero External Dependencies**: Self-contained implementation using only standard C library
- **C89-C23 Compatible**: Works with legacy and modern C compilers
- **C++ Compatible**: Works seamlessly in C++ projects
- **Simple API**: Easy-to-use reader and writer interfaces

## Interfaces

### IYamlNode
Represents a YAML node (scalar, sequence, or mapping).

**Methods:**
- `GetType()` - Get node type (scalar, sequence, mapping)
- `GetScalarValue()` / `SetScalarValue()` - Access scalar values
- `GetSequenceCount()` / `GetSequenceItem()` / `AppendSequenceItem()` - Sequence operations
- `GetMappingCount()` / `GetMappingKey()` / `GetMappingValue()` / `SetMappingValue()` - Mapping operations

### IYamlReader
Parses YAML from strings or buffers.

**Methods:**
- `ParseString()` - Parse null-terminated YAML string
- `ParseBuffer()` - Parse YAML from buffer with explicit length
- `GetLastError()` - Get error information from last parse

### IYamlWriter
Serializes YAML nodes to strings.

**Methods:**
- `SetOptions()` - Configure output options (pretty print, explicit start/end)
- `WriteToString()` - Serialize node to string
- `FreeString()` - Free allocated string

## Usage Example

```c
#include <ananke/yaml.h>
#include <stdio.h>

int main(void)
{
    IYamlReader *reader = NULL;
    IYamlWriter *writer = NULL;
    IYamlNode *root = NULL;
    IYamlNode *value = NULL;
    CHAR8 *output = NULL;
    HRESULT hr;
    CONST CHAR8 *yaml = "name: Ananke\nversion: 1.0";

    /* Create reader */
    hr = YamlCreateReader(&reader);
    if (FAILED(hr)) {
        return 1;
    }

    /* Parse YAML */
    hr = IYamlReader_ParseString(reader, yaml, &root);
    if (FAILED(hr)) {
        IYamlReader_Release(reader);
        return 1;
    }

    /* Read a value */
    hr = IYamlNode_GetMappingValue(root, "name", &value);
    if (SUCCEEDED(hr)) {
        CONST CHAR8 *str;
        UINTN len;
        IYamlNode_GetScalarValue(value, &str, &len);
        printf("Name: %.*s\n", (int)len, str);
        IYamlNode_Release(value);
    }

    /* Create writer */
    hr = YamlCreateWriter(&writer);
    if (SUCCEEDED(hr)) {
        /* Write YAML to string */
        IYamlWriter_SetOptions(writer, YamlWriteOptionPrettyPrint);
        hr = IYamlWriter_WriteToString(writer, root, &output, NULL);
        if (SUCCEEDED(hr)) {
            printf("Output:\n%s", output);
            IYamlWriter_FreeString(writer, output);
        }
        IYamlWriter_Release(writer);
    }

    /* Cleanup */
    IYamlNode_Release(root);
    IYamlReader_Release(reader);

    return 0;
}
```

## Creating YAML Programmatically

```c
IYamlNode *mapping = NULL;
IYamlNode *key = NULL;
IYamlNode *value = NULL;

/* Create a mapping */
YamlCreateNode(YamlNodeTypeMapping, &mapping);

/* Create scalar nodes */
YamlCreateNode(YamlNodeTypeScalar, &value);
IYamlNode_SetScalarValue(value, "Ananke", 6, YamlScalarStylePlain);

/* Add to mapping */
IYamlNode_SetMappingValue(mapping, "library", value);
IYamlNode_Release(value);

/* Create a sequence */
IYamlNode *sequence = NULL;
YamlCreateNode(YamlNodeTypeSequence, &sequence);

YamlCreateNode(YamlNodeTypeScalar, &value);
IYamlNode_SetScalarValue(value, "COM", 3, YamlScalarStylePlain);
IYamlNode_AppendSequenceItem(sequence, value);
IYamlNode_Release(value);

IYamlNode_SetMappingValue(mapping, "features", sequence);
IYamlNode_Release(sequence);

/* Write to string */
IYamlWriter *writer;
CHAR8 *output;
YamlCreateWriter(&writer);
IYamlWriter_WriteToString(writer, mapping, &output, NULL);
printf("%s", output);
IYamlWriter_FreeString(writer, output);
IYamlWriter_Release(writer);

IYamlNode_Release(mapping);
```

## Supported YAML Features

**Currently Supported:**
- Scalars (plain, single-quoted, double-quoted)
- Sequences (arrays)
- Mappings (dictionaries)
- Flow style syntax: `[a, b, c]`, `{key: value}`
- Block style syntax with indentation
- Comments (skipped during parsing)

**Limitations:**
- No multi-document support
- No anchors/aliases
- No custom tags
- Limited escape sequence support in strings
- No block scalar styles (|, >)

## Architecture

The implementation follows Ananke's COM-based design:

1. **Reference Counting**: Automatic lifetime management via AddRef/Release
2. **Interface Segregation**: Clean separation between reader, writer, and node
3. **HRESULT Error Handling**: Industry-standard error codes
4. **Binary Compatibility**: Stable vtable-based ABI

## Files

- `include/ananke/yaml.h` - Public interface definitions
- `libs/yaml/sources/yaml_node.c` - IYamlNode implementation
- `libs/yaml/sources/yaml_reader.c` - IYamlReader implementation
- `libs/yaml/sources/yaml_writer.c` - IYamlWriter implementation

## Building

The YAML library is automatically built as part of Ananke. No special configuration needed.

```bash
cd nux
mkdir build && cd build
../configure ARCH=amd64
make
```

The YAML interfaces will be available via:
```c
#include <ananke/yaml.h>
```

## License

Same as Ananke foundation library (BSD-2-Clause).

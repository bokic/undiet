# undiet

`undiet` is a performance-oriented C utility and library designed to decompress files packed with the **DIET** algorithm, a popular executable compression format from the MS-DOS era.

## Features

- **Robust Decompression**: Accurate implementation of the DIET bitstream unpacking logic.
- **Integrity Checks**: Built-in CRC16 verification to ensure data consistency.
- **Header Validation**: Automatic detection of DIET signatures and metadata.
- **Fast Execution**: Optimized bit-refill and match-copy routines.
- **Modern C**: Built leveraging the **C23** standard for improved safety and performance.

## Getting Started

### Prerequisites

- **CMake** (version 3.5 or higher)
- **C23 Compatible Compiler** (e.g., GCC 13+, Clang 17+)

### Building from Source

```bash
mkdir build
cd build
cmake ..
make
```

The build process produces two targets:
- `libundiet`: A shared library containing the decompression logic.
- `undiet`: A command-line utility for decompressing files.

## Usage

### Command Line Interface

The `undiet` tool takes a compressed file and an output path:

```bash
./undiet <compressed_file> <output_file>
```

**Example:**
```bash
./undiet DATA.CMP DATA.BIN
```

### Library API (`libundiet`)

You can integrate `libundiet` into your own projects by including `undiet.h`.

```c
#include "undiet.h"

// Check if a file is a valid DIET compressed file
if (undiet_isvalid(data, size)) {
    // Get expected uncompressed size
    uint32_t out_size = undiet_get_uncompressed_size(data, size);
    
    // Allocate buffer and unpack
    uint8_t *buffer = malloc(out_size);
    int32_t result = undiet_unpack(data, buffer);
}
```

#### Key Functions

| Function | Description |
| :--- | :--- |
| `undiet_unpack` | Decompresses the source data into the destination buffer. |
| `undiet_isvalid` | Validates the DIET header and signature. |
| `undiet_get_uncompressed_size` | Extracts the expected uncompressed size from the header. |
| `undiet_get_crc` | Retrieves the stored CRC16 from the header. |
| `undiet_calc_crc16` | Calculates the CRC16 for the compressed data stream. |

## Technical Details

### DIET Format
The DIET algorithm uses a bit-oriented LZ-style compression. The `undiet` implementation handles:
- **Literal Bytes**: Single-byte copies.
- **Short Matches**: 2-byte matches with small offsets.
- **Long Matches**: 3+ byte matches with larger offsets.
- **CRC16-IBM**: Verification using a 256-entry lookup table.

### Limitations
- Support is currently focused on the raw compressed data stream found in DIET-packed files.
- Segment refresh codes (found in some EXE stubs) are recognized but skipped in the current implementation.

## See Also

- **[Deark](https://github.com/jsummers/deark)**: A versatile file extraction utility by Jason Summers. The DIET decompression logic in `undiet` is based on the [DIET module](https://github.com/jsummers/deark/blob/master/modules/diet.c) implementation.

## License

*Refer to the project repository for licensing information.*

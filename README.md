# STM32F407 Custom Bootloader

A robust, interrupt-driven bootloader for STM32F407 microcontroller enabling secure firmware updates via UART communication with CRC-32 integrity checking and ACK/NACK handshake protocol.

## Features

- **Interrupt-driven UART communication** (115200 baud, 8N1)
- **CRC-32 integrity verification** for all message packets
- **ACK/NACK handshake protocol** with error codes
- **State machine-based message processing** ensuring proper command sequence
- **Flash sector management** with automatic erase/write operations
- **Timeout protection** (7-second packet timeout with automatic recovery)
- **LED status indication** (solid = waiting, blinking = transfer active)
- **Modular architecture** with clean separation of transport, application, and state management layers

## Hardware Requirements

- STM32F407VGT6 microcontroller
- UART4 connection (115200 baud)
- User button (PA0) for bootloader activation
- Status LED (PD13) for visual feedback

## Flash Memory Layout

STM32F407 flash memory is divided into 12 sectors of varying sizes:

```
Sector 0 (16KB): 0x08000000 - 0x08003FFF  (Bootloader/Application)
Sector 1 (16KB): 0x08004000 - 0x08007FFF  (Application)
Sector 2 (16KB): 0x08008000 - 0x0800BFFF  (Application)
Sector 3 (16KB): 0x0800C000 - 0x0800FFFF  (Application)
Sector 4 (64KB): 0x08010000 - 0x0801FFFF  (Application)
Sector 5-11:    0x08020000 - 0x080FFFFF   (128KB each, Application)
```

**Note**: This bootloader allows dynamic sector selection. Users can choose any sector (0-11) for firmware storage. However, it's recommended to follow traditional bootloader architecture by keeping Sector 0 for the bootloader and using sectors 1-11 for applications.

## Boot Sequence

1. **System Reset**: MCU starts execution from bootloader
2. **Button Check**: 1-second window to detect user button press
3. **Mode Selection**:
   - **Button pressed**: Enter bootloader mode (LED solid ON)
   - **No button**: Continue to normal application execution
4. **Bootloader Mode**: Wait for UART commands, LED blinks during transfer

## Communication Protocol

### Message Format
All messages are fixed 21-byte packets with different payload structures:

#### CMD_WRITE (0x01)
```
Byte 0:   0x01 (Message Type)
Byte 1:   Sector Number (0-11)
Byte 2-16: 0x00 (Padding)
Byte 17-20: CRC-32 (little-endian)
```

#### CMD_ERASE (0x02)
```
Byte 0:   0x02 (Message Type)
Byte 1:   Sector Number (0-11)
Byte 2-16: 0x00 (Padding)
Byte 17-20: CRC-32 (little-endian)
```

#### DATA (0x03)
```
Byte 0:   0x03 (Message Type)
Byte 1-16: Firmware Data (16 bytes)
Byte 17-20: CRC-32 (little-endian)
```

#### FINISH (0x04)
```
Byte 0:   0x04 (Message Type)
Byte 1-16: 0x00 (Padding)
Byte 17-20: CRC-32 (little-endian)
```

### Message Types
- `0x01`: CMD_WRITE - Write firmware to specified sector
- `0x02`: CMD_ERASE - Erase specified sector only
- `0x03`: DATA - Firmware data packet
- `0x04`: FINISH - Complete operation

### Command Sequences

#### Firmware Update Sequence
1. `CMD_WRITE` → ACK (sector erased)
2. `DATA` packets → ACK (firmware written)
3. `FINISH` → ACK (jump to application)

#### Sector Erase Sequence
1. `CMD_ERASE` → ACK (sector erased)
2. `FINISH` → ACK (return to command wait)

### Response Protocol
- **ACK**: Single byte `0xAA`
- **NACK**: Two bytes `0x55` + error code
  - `0x02`: Invalid CRC
  - `0x04`: Invalid command/sector
  - `0x05`: Sequence error

## State Machine

```
STATE_WAIT_CMD:
  ├── CMD_WRITE → STATE_WAIT_DATA_OR_FINISH
  └── CMD_ERASE → STATE_WAIT_FINISH

STATE_WAIT_DATA_OR_FINISH:
  ├── DATA → STATE_WAIT_DATA_OR_FINISH
  └── FINISH → STATE_WAIT_CMD

STATE_WAIT_FINISH:
  └── FINISH → STATE_WAIT_CMD
```

## Error Handling

### Timeout Protection
- **Packet timeout**: 7 seconds
- **Recovery**: Automatic sector re-erase and state reset
- **No NACK transmission**: Silent recovery to prevent protocol confusion

### CRC Validation
- CRC-32 polynomial: `0xEDB88320`
- Initial value: `0xFFFFFFFF`
- Final XOR: `0xFFFFFFFF`
- Validates first 17 bytes of each packet

## Application Jump Mechanism

The bootloader performs a clean jump to the application by:
1. Disabling interrupts
2. Deinitializing HAL peripherals
3. Setting vector table offset (VTOR)
4. Setting main stack pointer (MSP)
5. Jumping to application reset handler

## Design Decisions

### No Automatic Application Detection
Unlike traditional bootloaders that scan flash for valid applications, this implementation requires explicit user interaction (button press) to enter bootloader mode. This prevents:
- Accidental execution of outdated firmware in other sectors
- Bootloader confusion when multiple applications exist
- Unpredictable behavior during development

### Alternative: BootInfo Structure
For production systems, consider implementing a BootInfo structure at a fixed address (e.g., Sector 1) containing:
```c
typedef struct {
    uint8_t  activeSector;
    uint32_t appCrc;
    uint32_t version;
} BootInfo;
```
This would enable automatic application selection while maintaining control over which firmware runs.

## Building and Flashing

### Prerequisites
- STM32CubeIDE or compatible toolchain
- ARM GCC toolchain
- ST-Link or compatible programmer

### Build Commands
```bash
make -j20 all
```

### Flash to Device
```bash
# Using ST-Link
st-flash write Bootloader.bin 0x08000000
```

## Project Structure

```
Bootloader/
├── Core/
│   ├── Inc/           # Main application headers
│   └── Src/           # Main application source
├── UserInterface/
│   ├── Application/   # Message processing logic
│   └── Transport/     # UART protocol implementation
├── Drivers/           # STM32 HAL drivers
└── Debug/             # Build outputs
```

## Usage Example

1. **Enter Bootloader**: Press and hold user button during power-up
2. **Send Commands**: Use GUI or terminal to send firmware packets
3. **Monitor LED**: 
   - Solid = Ready for commands
   - Blinking = Transfer in progress
4. **Complete Update**: Send FINISH command to jump to new firmware

## Security Considerations

- CRC-32 provides data integrity but not authenticity
- No encryption implemented (add for production use)
- Consider implementing digital signatures for firmware validation
- Bootloader sectors should be write-protected in production

## Integration with FirmwareUploaderApp

- This bootloader is fully compatible with the custom desktop GUI tool:
🔗 https://github.com/semadilber/FirmwareUploaderApp

⚠️ Important Note:
The user-provided firmware must be linked correctly to match the target flash sector. This means the .ld (linker script) file must define the starting flash address according to the specific sector where the bootloader is designed to write the firmware.
If this address is incorrect, the uploaded firmware may overwrite unintended memory regions or fail to run properly after flashing.

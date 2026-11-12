#ifndef DISK_H
#define DISK_H

#include <stdint.h>

uint16_t GetSector(uint32_t dap, uint32_t drive);
uint16_t GetDriveParameters(uint32_t paramBuffer, uint32_t drive);
inline uint32_t getSegmentOffset(void* ptr);

struct DiskAddressPacket {
    uint8_t size;
    uint8_t reserved;
    uint16_t count;
    uint16_t bufferOffset;
    uint16_t bufferSegment;
    uint32_t lbaLow;
    uint32_t lbaHigh;
} __attribute__((packed));

static_assert(sizeof(DiskAddressPacket) == 0x10, "DiskAddressPacket size must be 16 bytes");

struct DriveParameters {
    uint16_t size;
    uint16_t flags;
    uint32_t cylinders;
    uint32_t heads;
    uint32_t sectorsPerTrack;
    uint64_t totalSectors;
    uint16_t bytesPerSector;
    uint32_t EDDConfigurationParameters;
    uint8_t reserved[36]; // Space used for INT 13 2.0/3.0 which is not relevant for this implementation
} __attribute__((packed));

static_assert(sizeof(DriveParameters) == 0x42, "DriveParameters size must be 66 bytes");

struct Disk {
    uint8_t driveNumber;
    uint8_t initialized;
    DriveParameters params;
};

uint16_t InitializeDisk(Disk* disk, uint8_t drive) {
    if (disk == nullptr) return 0xFFFF; // Invalid disk pointer
    disk->initialized = false;
    disk->driveNumber = drive;
    disk->params = {};
    disk->params.size = sizeof(DriveParameters);
    uint16_t result = GetDriveParameters(getSegmentOffset(&(disk->params)), drive);
    if (result == 0) disk->initialized = true;
    return result;
}


// Used for reading 128+ sectors at once, a process that is not supported
// by certain BIOSes.
uint16_t getSectorsLarge(Disk* disk, uint64_t lba, uint16_t count, void* buffer) {
    // unimplemented
}

uint16_t GetSectors(Disk* disk, uint64_t lba, uint16_t count, void* buffer) {
    if (count == 0) return 0; // No sectors to read, so we're done technically.
    if (buffer == nullptr || disk == nullptr || disk->initialized == false || disk->params.bytesPerSector == 0) return 1; // Invalid parameters
    // Theoretically the maximum amount of sectors that can be read in one call is 65,535
    // (because that's the uint16_t limit for this function) but certain Phoenix BIOSes
    // will reject requests for more than 127 sectors at once.
    // As such, a max read is limited to 127 sectors.
    // - the same developer who straight-up rejected CHS support
    if (count > 127) return 2; // Count exceeds maximum allowed sectors (127)
    if (lba >= disk->params.totalSectors || count > disk->params.totalSectors - lba) return 4; // Requested sectors exceed total sectors on the drive
    DiskAddressPacket dap;
    dap.size = sizeof(DiskAddressPacket);
    dap.reserved = 0;
    dap.count = count;

    uint32_t start = (uint32_t)(uintptr_t)buffer;
    uint32_t bytes = (uint32_t)count * disk->params.bytesPerSector;
    uint32_t last = start + bytes - 1;

    if (last > 0x9FFFF) return 5; // Buffer exceeds 1MB limit for real mode addressing
    if ((start >> 16) != (last >> 16)) return 6; // Buffer crosses segment boundary

    uint32_t address = getSegmentOffset(buffer);
    if ((address & 0xFFFF) + bytes > 0x10000) return 3; // Buffer crosses segment boundary

    dap.bufferOffset = address & 0xFFFF; // Offset within the segment
    dap.bufferSegment = address >> 16; // Segment address
    dap.lbaLow = lba & 0xFFFFFFFF;
    dap.lbaHigh = lba >> 32;
    uint16_t result = GetSector(getSegmentOffset(&dap), disk->driveNumber);
    return result == 0 ? 0 : result | 0x100; // Return the result code, offset by 256 to avoid clashing with our own error codes, or 0 if successful
}

inline uint32_t getSegmentOffset(void* ptr) {
    return ((uint32_t)ptr >> 4) << 16 | ((uint32_t)ptr & 0xF);
}

#endif // DISK_H
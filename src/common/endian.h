#pragma once

#define BIG_TO_LITTLE16(num) ((num >> 8) | (num << 8))
#define BIG_TO_LITTLE32(num)                              \
    ((/* byte 3 to byte 0 */ (num >> 24) & (0xFF << 0)) | \
     (/* byte 2 to byte 1 */ (num >> 8) & (0xFF << 8)) |  \
     (/* byte 1 to byte 2 */ (num << 8) & (0xFF << 16)) | \
     (/* byte 0 to byte 3 */ (num << 24) & (0XFF << 24)))

#define BIG_TO_LITTLE64(num)                                 \
    ((/* byte 7 to byte 0 */ (num >> 56) & (0xFFLL << 0)) |  \
     (/* byte 6 to byte 1 */ (num >> 40) & (0xFFLL << 8)) |  \
     (/* byte 5 to byte 2 */ (num >> 24) & (0xFFLL << 16)) | \
     (/* byte 4 to byte 3 */ (num >> 8) & (0xFFLL << 24)) |  \
     (/* byte 3 to byte 4 */ (num << 8) & (0xFFLL << 32)) |  \
     (/* byte 2 to byte 5 */ (num << 24) & (0xFFLL << 40)) | \
     (/* byte 1 to byte 6 */ (num << 40) & (0xFFLL << 48)) | \
     (/* byte 0 to byte 7 */ (num << 56) & (0xFFLL << 56)))

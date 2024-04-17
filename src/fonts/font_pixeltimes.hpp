#pragma once

#include <stdint.h>

static uint8_t const FONT_PIXELTIMES[] =
{
    1, 0, 0, 0, 0, 1, 8, 1, 0, 0, 1, 0, 1, 8, 1, 0,
    0, 2, 0, 1, 8, 1, 0, 0, 3, 0, 1, 8, 1, 0, 0, 4,
    0, 1, 8, 1, 0, 0, 5, 0, 1, 8, 1, 0, 0, 6, 0, 1,
    8, 1, 0, 0, 7, 0, 1, 8, 1, 0, 0, 8, 0, 1, 8, 1,
    0, 0, 9, 0, 1, 8, 1, 0, 0, 10, 0, 1, 8, 1, 0, 0,
    11, 0, 1, 8, 1, 0, 0, 12, 0, 1, 8, 1, 0, 0, 13, 0,
    1, 8, 1, 0, 0, 14, 0, 1, 8, 1, 0, 0, 15, 0, 1, 8,
    1, 0, 0, 16, 0, 1, 8, 1, 0, 0, 17, 0, 1, 8, 1, 0,
    0, 18, 0, 1, 8, 1, 0, 0, 19, 0, 1, 8, 1, 0, 0, 20,
    0, 1, 8, 1, 0, 0, 21, 0, 1, 8, 1, 0, 0, 22, 0, 1,
    8, 1, 0, 0, 23, 0, 1, 8, 1, 0, 0, 24, 0, 1, 8, 1,
    0, 0, 25, 0, 1, 8, 1, 0, 0, 26, 0, 1, 8, 1, 0, 0,
    27, 0, 1, 8, 1, 0, 0, 28, 0, 1, 8, 1, 0, 0, 29, 0,
    1, 8, 1, 0, 0, 30, 0, 1, 8, 1, 0, 0, 31, 0, 1, 8,
    3, 0, 0, 32, 0, 1, 8, 2, 0, 248, 33, 0, 1, 8, 4, 0,
    248, 34, 0, 4, 8, 7, 0, 248, 38, 0, 7, 8, 6, 0, 246, 45,
    0, 5, 16, 10, 0, 248, 55, 0, 10, 8, 9, 0, 248, 65, 0, 8,
    8, 2, 0, 248, 73, 0, 1, 8, 4, 0, 248, 74, 0, 4, 16, 4,
    0, 248, 82, 0, 4, 16, 6, 0, 248, 90, 0, 5, 8, 6, 0, 249,
    95, 0, 5, 8, 3, 0, 255, 100, 0, 2, 8, 4, 0, 252, 102, 0,
    4, 8, 2, 0, 255, 106, 0, 1, 8, 4, 0, 248, 107, 0, 4, 8,
    6, 0, 248, 111, 0, 5, 8, 4, 0, 248, 116, 0, 4, 8, 6, 0,
    248, 120, 0, 5, 8, 6, 0, 248, 125, 0, 5, 8, 6, 0, 248, 130,
    0, 5, 8, 6, 0, 248, 135, 0, 5, 8, 6, 0, 248, 140, 0, 5,
    8, 6, 0, 248, 145, 0, 5, 8, 6, 0, 248, 150, 0, 5, 8, 6,
    0, 248, 155, 0, 5, 8, 2, 0, 251, 160, 0, 1, 8, 3, 0, 251,
    161, 0, 2, 8, 6, 0, 249, 163, 0, 5, 8, 6, 0, 251, 168, 0,
    5, 8, 6, 0, 249, 173, 0, 5, 8, 5, 0, 248, 178, 0, 4, 8,
    10, 0, 248, 182, 0, 10, 16, 8, 0, 248, 202, 0, 7, 8, 7, 0,
    248, 209, 0, 7, 8, 7, 0, 248, 216, 0, 7, 8, 8, 0, 248, 223,
    0, 7, 8, 7, 0, 248, 230, 0, 7, 8, 7, 0, 248, 237, 0, 7,
    8, 8, 0, 248, 244, 0, 7, 8, 8, 0, 248, 251, 0, 7, 8, 4,
    0, 248, 2, 1, 4, 8, 5, 0, 248, 6, 1, 4, 8, 9, 0, 248,
    10, 1, 8, 8, 7, 0, 248, 18, 1, 7, 8, 10, 0, 248, 25, 1,
    10, 8, 8, 0, 248, 35, 1, 7, 8, 8, 0, 248, 42, 1, 7, 8,
    7, 0, 248, 49, 1, 7, 8, 8, 0, 248, 56, 1, 7, 16, 8, 0,
    248, 70, 1, 7, 8, 5, 0, 248, 77, 1, 4, 8, 8, 0, 248, 81,
    1, 7, 8, 8, 0, 248, 88, 1, 7, 8, 8, 0, 248, 95, 1, 7,
    8, 12, 0, 248, 102, 1, 11, 8, 8, 0, 248, 113, 1, 7, 8, 8,
    0, 248, 120, 1, 7, 8, 7, 0, 248, 127, 1, 7, 8, 4, 0, 248,
    134, 1, 4, 16, 4, 0, 248, 142, 1, 4, 8, 4, 0, 248, 146, 1,
    4, 16, 5, 0, 248, 154, 1, 4, 8, 7, 0, 1, 158, 1, 7, 8,
    3, 0, 248, 165, 1, 2, 8, 6, 0, 251, 167, 1, 5, 8, 5, 0,
    248, 172, 1, 4, 8, 5, 0, 251, 176, 1, 4, 8, 6, 0, 248, 180,
    1, 5, 8, 5, 0, 251, 185, 1, 4, 8, 6, 0, 248, 189, 1, 5,
    8, 6, 0, 251, 194, 1, 5, 8, 7, 0, 248, 199, 1, 7, 8, 4,
    0, 248, 206, 1, 4, 8, 3, 0, 248, 210, 1, 2, 16, 6, 0, 248,
    214, 1, 5, 8, 4, 0, 248, 219, 1, 4, 8, 10, 0, 251, 223, 1,
    10, 8, 7, 0, 251, 233, 1, 7, 8, 6, 0, 251, 240, 1, 5, 8,
    6, 0, 251, 245, 1, 5, 8, 6, 0, 251, 250, 1, 5, 8, 5, 0,
    251, 255, 1, 4, 8, 4, 0, 251, 3, 2, 4, 8, 4, 0, 248, 7,
    2, 4, 8, 7, 0, 251, 11, 2, 7, 8, 7, 0, 251, 18, 2, 7,
    8, 8, 0, 251, 25, 2, 7, 8, 6, 0, 251, 32, 2, 5, 8, 7,
    0, 251, 37, 2, 7, 8, 6, 0, 251, 44, 2, 5, 8, 4, 0, 248,
    49, 2, 4, 16, 2, 0, 248, 57, 2, 1, 16, 4, 0, 248, 59, 2,
    4, 16, 6, 0, 252, 67, 2, 5, 8, 1, 0, 0, 72, 2, 1, 8,
    1, 0, 0, 73, 2, 1, 8, 1, 0, 0, 74, 2, 1, 8, 1, 0,
    0, 75, 2, 1, 8, 1, 0, 0, 76, 2, 1, 8, 1, 0, 0, 77,
    2, 1, 8, 1, 0, 0, 78, 2, 1, 8, 1, 0, 0, 79, 2, 1,
    8, 1, 0, 0, 80, 2, 1, 8, 1, 0, 0, 81, 2, 1, 8, 1,
    0, 0, 82, 2, 1, 8, 1, 0, 0, 83, 2, 1, 8, 1, 0, 0,
    84, 2, 1, 8, 1, 0, 0, 85, 2, 1, 8, 1, 0, 0, 86, 2,
    1, 8, 1, 0, 0, 87, 2, 1, 8, 1, 0, 0, 88, 2, 1, 8,
    1, 0, 0, 89, 2, 1, 8, 1, 0, 0, 90, 2, 1, 8, 1, 0,
    0, 91, 2, 1, 8, 1, 0, 0, 92, 2, 1, 8, 1, 0, 0, 93,
    2, 1, 8, 1, 0, 0, 94, 2, 1, 8, 1, 0, 0, 95, 2, 1,
    8, 1, 0, 0, 96, 2, 1, 8, 1, 0, 0, 97, 2, 1, 8, 1,
    0, 0, 98, 2, 1, 8, 1, 0, 0, 99, 2, 1, 8, 1, 0, 0,
    100, 2, 1, 8, 1, 0, 0, 101, 2, 1, 8, 1, 0, 0, 102, 2,
    1, 8, 1, 0, 0, 103, 2, 1, 8, 1, 0, 0, 104, 2, 1, 8,
    3, 0, 0, 105, 2, 1, 8, 2, 0, 251, 106, 2, 1, 8, 5, 0,
    249, 107, 2, 4, 16, 5, 0, 248, 115, 2, 4, 8, 6, 0, 249, 119,
    2, 5, 8, 8, 0, 248, 124, 2, 7, 8, 2, 0, 248, 131, 2, 1,
    16, 5, 0, 248, 133, 2, 4, 16, 4, 0, 248, 141, 2, 4, 8, 9,
    0, 248, 145, 2, 8, 8, 4, 0, 248, 153, 2, 4, 8, 6, 0, 251,
    157, 2, 5, 8, 6, 0, 251, 162, 2, 5, 8, 4, 0, 252, 167, 2,
    4, 8, 9, 0, 248, 171, 2, 8, 8, 7, 0, 246, 179, 2, 7, 8,
    4, 0, 248, 186, 2, 4, 8, 6, 0, 249, 190, 2, 5, 8, 4, 0,
    248, 195, 2, 4, 8, 4, 0, 248, 199, 2, 4, 8, 3, 0, 248, 203,
    2, 2, 8, 6, 0, 251, 205, 2, 5, 8, 6, 0, 248, 210, 2, 5,
    16, 2, 0, 252, 220, 2, 1, 8, 3, 0, 0, 221, 2, 2, 8, 4,
    0, 248, 223, 2, 4, 8, 4, 0, 248, 227, 2, 4, 8, 6, 0, 251,
    231, 2, 5, 8, 9, 0, 248, 236, 2, 8, 8, 9, 0, 248, 244, 2,
    8, 8, 9, 0, 248, 252, 2, 8, 8, 5, 0, 251, 4, 3, 4, 8,
    8, 0, 245, 8, 3, 7, 16, 8, 0, 245, 22, 3, 7, 16, 8, 0,
    245, 36, 3, 7, 16, 8, 0, 245, 50, 3, 7, 16, 8, 0, 246, 64,
    3, 7, 16, 8, 0, 246, 78, 3, 7, 16, 13, 0, 248, 92, 3, 13,
    8, 7, 0, 248, 105, 3, 7, 16, 7, 0, 245, 119, 3, 7, 16, 7,
    0, 245, 133, 3, 7, 16, 7, 0, 245, 147, 3, 7, 16, 7, 0, 246,
    161, 3, 7, 16, 4, 0, 245, 175, 3, 4, 16, 4, 0, 245, 183, 3,
    4, 16, 4, 0, 245, 191, 3, 4, 16, 4, 0, 246, 199, 3, 4, 16,
    8, 0, 248, 207, 3, 7, 8, 8, 0, 245, 214, 3, 7, 16, 8, 0,
    245, 228, 3, 7, 16, 8, 0, 245, 242, 3, 7, 16, 8, 0, 245, 0,
    4, 7, 16, 8, 0, 245, 14, 4, 7, 16, 8, 0, 246, 28, 4, 7,
    16, 6, 0, 249, 42, 4, 5, 8, 8, 0, 248, 47, 4, 7, 8, 8,
    0, 245, 54, 4, 7, 16, 8, 0, 245, 68, 4, 7, 16, 8, 0, 245,
    82, 4, 7, 16, 8, 0, 246, 96, 4, 7, 16, 8, 0, 245, 110, 4,
    7, 16, 6, 0, 248, 124, 4, 5, 8, 5, 0, 248, 129, 4, 4, 8,
    6, 0, 248, 133, 4, 5, 8, 6, 0, 248, 138, 4, 5, 8, 6, 0,
    248, 143, 4, 5, 8, 6, 0, 248, 148, 4, 5, 8, 6, 0, 249, 153,
    4, 5, 8, 6, 0, 246, 158, 4, 5, 16, 8, 0, 251, 168, 4, 7,
    8, 5, 0, 251, 175, 4, 4, 8, 5, 0, 248, 179, 4, 4, 8, 5,
    0, 248, 183, 4, 4, 8, 5, 0, 248, 187, 4, 4, 8, 5, 0, 249,
    191, 4, 4, 8, 4, 0, 248, 195, 4, 4, 8, 4, 0, 248, 199, 4,
    4, 8, 4, 0, 248, 203, 4, 4, 8, 4, 0, 249, 207, 4, 4, 8,
    6, 0, 248, 211, 4, 5, 8, 7, 0, 248, 216, 4, 7, 8, 6, 0,
    248, 223, 4, 5, 8, 6, 0, 248, 228, 4, 5, 8, 6, 0, 248, 233,
    4, 5, 8, 6, 0, 248, 238, 4, 5, 8, 6, 0, 249, 243, 4, 5,
    8, 6, 0, 249, 248, 4, 5, 8, 7, 0, 251, 253, 4, 7, 8, 7,
    0, 248, 4, 5, 7, 8, 7, 0, 248, 11, 5, 7, 8, 7, 0, 248,
    18, 5, 7, 8, 7, 0, 249, 25, 5, 7, 8, 7, 0, 248, 32, 5,
    7, 16, 6, 0, 248, 46, 5, 5, 16, 7, 0, 249, 56, 5, 7, 16,
    13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 191, 7, 0, 7, 0, 232, 60, 43, 232, 60, 43, 0, 24, 36,
    254, 68, 136, 1, 2, 7, 2, 1, 6, 137, 70, 32, 24, 4, 98, 145,
    96, 0, 112, 144, 142, 185, 69, 163, 152, 72, 7, 252, 2, 1, 0, 0,
    1, 2, 0, 1, 2, 252, 0, 2, 1, 0, 0, 10, 4, 31, 4, 10,
    8, 8, 62, 8, 8, 5, 3, 2, 2, 2, 0, 1, 192, 60, 3, 0,
    126, 129, 129, 129, 126, 129, 255, 128, 0, 130, 193, 161, 145, 206, 130, 129,
    137, 137, 118, 48, 44, 34, 255, 32, 128, 134, 133, 137, 113, 120, 150, 138,
    137, 113, 2, 1, 193, 57, 7, 102, 153, 137, 153, 102, 142, 145, 81, 113,
    30, 17, 81, 48, 8, 20, 20, 20, 34, 5, 5, 5, 5, 5, 34, 20,
    20, 20, 8, 2, 177, 9, 6, 248, 4, 114, 137, 133, 117, 77, 66, 60,
    0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 128, 224, 188, 35, 188,
    224, 128, 129, 255, 137, 137, 137, 118, 0, 60, 66, 129, 129, 129, 67, 0,
    129, 255, 129, 129, 129, 66, 60, 129, 255, 137, 157, 129, 195, 0, 129, 255,
    137, 29, 1, 3, 0, 60, 66, 129, 129, 145, 115, 16, 129, 255, 137, 8,
    137, 255, 129, 129, 255, 129, 0, 128, 129, 127, 1, 129, 255, 137, 20, 163,
    193, 129, 128, 129, 255, 129, 128, 128, 192, 0, 129, 255, 135, 56, 192, 56,
    135, 255, 129, 0, 129, 255, 130, 12, 17, 255, 1, 60, 66, 129, 129, 129,
    66, 60, 129, 255, 145, 17, 17, 14, 0, 60, 66, 129, 129, 129, 66, 60,
    0, 0, 0, 0, 1, 2, 2, 129, 255, 9, 25, 105, 134, 128, 198, 137,
    145, 99, 3, 1, 129, 255, 129, 1, 3, 1, 127, 129, 128, 129, 127, 1,
    1, 7, 57, 192, 57, 7, 1, 1, 7, 57, 192, 49, 15, 49, 192, 57,
    7, 1, 129, 195, 165, 24, 165, 195, 129, 1, 3, 141, 240, 141, 3, 1,
    195, 161, 153, 133, 131, 193, 0, 255, 1, 1, 0, 3, 2, 2, 0, 3,
    60, 192, 0, 1, 1, 255, 0, 2, 2, 3, 0, 12, 3, 3, 12, 1,
    1, 1, 1, 1, 1, 0, 1, 2, 8, 21, 21, 30, 16, 127, 136, 136,
    112, 14, 17, 17, 19, 112, 136, 137, 255, 128, 14, 21, 21, 22, 136, 254,
    137, 1, 3, 58, 85, 85, 83, 33, 129, 255, 144, 8, 248, 128, 0, 136,
    249, 128, 0, 8, 249, 2, 1, 129, 255, 32, 216, 136, 129, 255, 128, 0,
    17, 31, 18, 1, 31, 18, 1, 31, 16, 0, 17, 31, 18, 1, 31, 16,
    0, 14, 17, 17, 17, 14, 65, 127, 81, 17, 14, 14, 17, 81, 127, 64,
    17, 31, 18, 1, 18, 21, 13, 0, 4, 255, 132, 0, 1, 15, 16, 17,
    31, 16, 0, 1, 7, 24, 5, 3, 1, 0, 7, 24, 5, 7, 24, 7,
    1, 17, 27, 4, 27, 17, 65, 71, 56, 13, 3, 1, 0, 19, 25, 21,
    19, 25, 16, 238, 1, 0, 0, 1, 2, 0, 255, 3, 1, 238, 16, 0,
    2, 1, 0, 0, 3, 1, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 125, 56, 228, 92, 39,
    1, 0, 0, 0, 232, 94, 137, 193, 45, 18, 18, 18, 45, 1, 43, 173,
    248, 173, 43, 1, 207, 3, 26, 37, 73, 178, 1, 2, 2, 1, 1, 0,
    1, 0, 60, 66, 153, 165, 165, 165, 66, 60, 5, 5, 7, 0, 4, 10,
    21, 10, 17, 1, 1, 1, 1, 3, 2, 2, 2, 0, 60, 66, 197, 189,
    149, 169, 66, 60, 2, 2, 2, 2, 2, 2, 0, 2, 5, 2, 0, 72,
    72, 126, 72, 72, 10, 13, 11, 0, 9, 11, 5, 0, 2, 1, 127, 16,
    16, 31, 16, 14, 255, 1, 255, 1, 0, 3, 0, 3, 0, 1, 2, 3,
    10, 15, 8, 0, 2, 5, 2, 0, 17, 10, 21, 10, 4, 10, 15, 200,
    48, 76, 99, 240, 64, 10, 143, 104, 24, 4, 163, 208, 176, 9, 139, 101,
    16, 76, 99, 240, 64, 48, 72, 69, 32, 0, 0, 225, 26, 224, 0, 0,
    4, 7, 5, 1, 5, 7, 4, 0, 0, 224, 26, 225, 0, 0, 4, 7,
    5, 1, 5, 7, 4, 0, 0, 226, 25, 226, 0, 0, 4, 7, 5, 1,
    5, 7, 4, 0, 0, 227, 25, 226, 3, 0, 4, 7, 5, 1, 5, 7,
    4, 0, 128, 241, 140, 241, 128, 0, 2, 3, 2, 0, 2, 3, 2, 0,
    128, 242, 141, 242, 128, 0, 2, 3, 2, 0, 2, 3, 2, 128, 192, 160,
    24, 21, 147, 255, 137, 137, 157, 131, 64, 0, 60, 66, 129, 129, 129, 67,
    0, 0, 0, 2, 3, 0, 0, 0, 8, 248, 73, 234, 8, 24, 0, 4,
    7, 4, 4, 4, 6, 0, 8, 248, 72, 234, 9, 24, 0, 4, 7, 4,
    4, 4, 6, 0, 8, 248, 74, 233, 10, 24, 0, 4, 7, 4, 4, 4,
    6, 0, 4, 252, 37, 116, 5, 12, 0, 2, 3, 2, 2, 2, 3, 0,
    9, 250, 8, 0, 4, 7, 4, 0, 8, 250, 9, 0, 4, 7, 4, 0,
    10, 249, 10, 0, 4, 7, 4, 0, 5, 252, 5, 0, 2, 3, 2, 0,
    137, 255, 137, 137, 129, 66, 60, 8, 248, 35, 193, 10, 251, 8, 4, 7,
    4, 0, 1, 7, 0, 224, 16, 9, 10, 8, 16, 224, 1, 2, 4, 4,
    4, 2, 1, 224, 16, 8, 10, 9, 16, 224, 1, 2, 4, 4, 4, 2,
    1, 224, 16, 10, 9, 10, 16, 224, 1, 2, 4, 4, 4, 2, 1, 224,
    19, 9, 10, 11, 16, 224, 1, 2, 4, 4, 4, 2, 1, 240, 8, 5,
    4, 5, 8, 240, 0, 1, 2, 2, 2, 1, 0, 34, 20, 8, 20, 34,
    188, 66, 177, 137, 133, 66, 61, 8, 248, 9, 2, 8, 248, 8, 0, 3,
    4, 4, 4, 3, 0, 8, 248, 8, 2, 9, 248, 8, 0, 3, 4, 4,
    4, 3, 0, 8, 248, 10, 1, 10, 248, 8, 0, 3, 4, 4, 4, 3,
    0, 4, 252, 5, 0, 5, 252, 4, 0, 1, 2, 2, 2, 1, 0, 8,
    24, 104, 130, 105, 24, 8, 0, 0, 4, 7, 4, 0, 0, 129, 255, 165,
    36, 24, 254, 1, 137, 118, 64, 169, 170, 240, 128, 64, 168, 170, 241, 128,
    64, 170, 169, 242, 128, 67, 169, 170, 243, 128, 32, 85, 84, 121, 64, 0,
    164, 170, 196, 0, 1, 2, 2, 3, 2, 24, 21, 21, 15, 21, 21, 22,
    14, 81, 113, 19, 112, 169, 170, 176, 112, 170, 169, 176, 112, 170, 169, 178,
    56, 85, 84, 89, 137, 250, 128, 0, 136, 250, 129, 0, 138, 249, 130, 0,
    69, 124, 65, 0, 112, 139, 138, 141, 120, 136, 251, 145, 10, 251, 128, 0,
    112, 137, 138, 136, 112, 112, 136, 138, 137, 112, 112, 138, 137, 138, 112, 112,
    139, 137, 138, 115, 56, 69, 68, 69, 56, 8, 8, 42, 8, 8, 46, 25,
    21, 19, 14, 1, 0, 8, 120, 129, 138, 248, 128, 0, 8, 120, 130, 137,
    248, 128, 0, 8, 122, 129, 138, 248, 128, 0, 4, 61, 64, 69, 124, 64,
    0, 8, 56, 194, 105, 24, 8, 0, 2, 2, 1, 0, 0, 0, 0, 1,
    255, 136, 136, 112, 2, 3, 2, 0, 0, 4, 29, 224, 53, 12, 4, 0,
    1, 1, 0, 0, 0, 0, 0, 
};

#ifdef RGB_MATRIX_ENABLE
led_config_t g_led_config = { {
    // Key-to-LED index mapping
    {  0,  1,  2,  3 },
    {  4,  5,  6,  7 },
    {  8,  9, 10, 11 },
    { 12, 13, 14, 15 }
}, {
    // Physical position of each LED (x, y) in 0–255 range
    {  0,   0 },   // LED 0 at (0,0)
    { 64,   0 },   // LED 1 at (1,0)
    {128,   0 },   // LED 2 at (2,0)
    {192,   0 },   // LED 3 at (3,0)
    {  0,  64 },   // LED 4 at (0,1)
    { 64,  64 },   // LED 5 at (1,1)
    {128,  64 },   // LED 6 at (2,1)
    {192,  64 },   // LED 7 at (3,1)
    {  0, 128 },   // LED 8 at (0,2)
    { 64, 128 },   // LED 9 at (1,2)
    {128, 128 },   // LED10 at (2,2)
    {192, 128 },   // LED11 at (3,2)
    {  0, 192 },   // LED12 at (0,3)
    { 64, 192 },   // LED13 at (1,3)
    {128, 192 },   // LED14 at (2,3)
    {192, 192 }    // LED15 at (3,3)
}, {
    // LED flags — all set to 4
    4, 4, 4, 4,
    4, 4, 4, 4,
    4, 4, 4, 4,
    4, 4, 4, 4
} };
#endif

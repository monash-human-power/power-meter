/**
 * @file lights.cpp
 * @author Jotham Gates
 * @brief Displays a fancy Christmas light pattern.
 * @version 0.0.0
 * @date 2025-12-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "lights.h"
#include "power_meter.h"
extern SemaphoreHandle_t serialMutex;
extern PowerMeter powerMeter;
extern portMUX_TYPE spinlock;

LedMatrix christmasLeds;

// Generated using https://jasoncoon.github.io/led-mapper/
uint16_t angles[NUM_LEDS] = {
    15866, 17422, 18150, 18537, 18925, 19728, 21432, 23004, 23351,
    23337, 23722, 24523, 26666, 23472, 25643, 26641, 27101, 27099,
    27739, 29432, 31095, 31700, 31928, 32230, 32966, 34619, 31581,
    33749, 34938, 35348, 35445, 35757, 37199, 38946, 40033, 40068,
    40544, 41258, 42647, 42775, 41667, 43358, 43602, 43842, 43858,
    44960, 46668, 48033, 48246, 48793, 49323, 50463, 53681, 50180,
    50902, 51502, 51988, 52001, 52411, 54037, 55706, 56453, 56673,
    56959, 57694, 59248, 58797, 58394, 59207, 59930, 59929, 60093,
    61361, 63045, 64320, 64634, 64910, 65288, 568, 3182, 64834,
    1533, 2043, 2469, 2532, 2887, 4709, 6313, 7102, 7232,
    7473, 8020, 8589, 9533, 10333, 10627, 10857, 11664, 13308,
    14880};
uint16_t radii[NUM_LEDS] = {
    18165, 27965, 38503, 48880, 60095, 1654, 20, 64056, 53814,
    42672, 31550, 20642, 10041, 14584, 25479, 36083, 46194, 57684,
    953, 63699, 312, 56290, 45901, 35055, 24181, 13438, 9432,
    20559, 31148, 42028, 52839, 63378, 994, 65149, 62148, 50845,
    40315, 29229, 18408, 8140, 17201, 27988, 39201, 50189, 61590,
    2506, 856, 65368, 53857, 44283, 33096, 21883, 12052, 12979,
    24425, 35630, 46482, 57958, 3262, 3095, 2277, 60737, 49278,
    37984, 27244, 16622, 8780, 20627, 31571, 42488, 53430, 65174,
    4333, 2197, 65089, 53726, 42825, 31936, 23254, 12683, 13853,
    25129, 36637, 46945, 57451, 3148, 2189, 1700, 59123, 48520,
    37705, 26371, 19828, 30613, 40944, 52012, 62570, 2453, 65346,
    345};

CRGB leds[NUM_LEDS];

void LedPatternBase::update(float position)
{
    // LOGD("LED", "Pos: %f", position);
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        LedPosition pos;
        m_transform(i, position, pos);
        m_updateLed(i, pos);
    }
}

void LedPatternBase::m_transform(uint8_t index, float angleOffset, LedPosition &position)
{
    uint32_t scaledAngle = angleOffset / (2 * PI) * ((1 << 16) - 1);
    position.angle = angles[index] + scaledAngle; // This will wrap around as needed.
    // position.angle = angles[index];
    position.radii = radii[index];
    position.x = (uint32_t)(cos16(position.angle) * position.radii) / (1 << 15);
    position.y = (uint32_t)(sin16(position.angle) * position.radii) / (1 << 15);
}

void VerticalLedPattern::m_updateLed(uint8_t index, LedPosition &position)
{
    CRGBPalette16 currentPalete = RainbowStripesColors_p;
    leds[index] = ColorFromPalette(currentPalete, (position.y >> 8) + index);
}


void RedGreenWedges::m_updateLed(uint8_t index, LedPosition &position)
{
    const uint16_t segments = 8;
    uint16_t segment = position.angle / ((1<<16)/segments);
    if (segment & 0x1)
    {
        // Odd segments.
        leds[index] = CRGB(0, 255, 0);
    }
    else
    {
        leds[index] = CRGB(255, 0, 0);
    }
}

void LedMatrix::begin()
{
    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE, PIN_LEDS, COLOR_ORDER>(leds, NUM_LEDS)
        .setCorrection(TypicalLEDStrip)
        .setDither(BRIGHTNESS < 255);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);
}

void LedMatrix::update()
{
    taskENTER_CRITICAL(&spinlock);
    Matrix<2, 1, float> state = powerMeter.imuManager.kalman.getState();
    taskEXIT_CRITICAL(&spinlock);
    float position = state(0, 0);
    activePattern->update(position);
    FastLED.show();
}

void taskChristmasLeds(void *pvParameters)
{
    LOGD("Christmas", "Starting the Christmas lights task");
    // VerticalLedPattern pattern;
    RedGreenWedges pattern;
    christmasLeds.activePattern = &pattern;
    christmasLeds.begin();
    while (true)
    {
        christmasLeds.update();
        delay(10); // Wait a little while.
    }
}
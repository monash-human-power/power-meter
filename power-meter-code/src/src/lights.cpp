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
    15852, 17051, 17602, 17892, 18205, 18940, 20550, 22030, 22386,
    22425, 22868, 23761, 26027, 23008, 24697, 25519, 25900, 25869,
    26449, 28029, 29573, 30141, 30356, 30639, 31307, 32720, 30150,
    32001, 33102, 33501, 33606, 33907, 35253, 36879, 37888, 37889,
    38285, 38855, 39918, 39351, 38997, 40772, 41116, 41408, 41467,
    42522, 44128, 45417, 45573, 46034, 46430, 47294, 49881, 46579,
    47781, 48536, 49093, 49164, 49594, 51149, 52749, 53449, 53619,
    53831, 54441, 55767, 54747, 55016, 55981, 56758, 56794, 56978,
    58212, 59843, 61078, 61379, 61644, 62012, 575, 3399, 61515,
    1544, 2019, 2418, 2466, 2800, 4566, 6119, 6901, 7061,
    7353, 8007, 8709, 9433, 10111, 10331, 10511, 11269, 12851,
    14349};
uint16_t radii[NUM_LEDS] = {
    17243, 26736, 36830, 46733, 57432, 64271, 62921, 61667, 51975,
    41391, 30867, 20587, 10696, 14745, 25272, 35416, 45051, 55965,
    64369, 61804, 63895, 54825, 44958, 34657, 24329, 14104, 10310,
    20882, 30919, 41243, 51511, 61511, 64442, 63022, 60088, 49349,
    39307, 28715, 18313, 8584, 17264, 27328, 37948, 48355, 59182,
    65183, 63406, 62259, 51300, 42137, 31448, 20663, 10948, 12271,
    23016, 33571, 43813, 54710, 64954, 64599, 63634, 56836, 45930,
    35176, 24908, 14694, 7304, 18564, 28889, 39205, 49598, 60743,
    65129, 63029, 60487, 49688, 39330, 28983, 20737, 10763, 11807,
    22531, 33475, 43280, 53263, 63946, 63120, 62763, 55120, 45059,
    34808, 24096, 17942, 28275, 38170, 48714, 58767, 64008, 61698,
    62406};

CRGB leds[NUM_LEDS];

void LedPatternBase::update(float position, float velocity)
{
    uint32_t now = millis();
    // LOGD("LED", "Pos: %f", position);
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        LedPosition pos;
        m_transform(i, position, pos);
        m_updateLed(i, pos, velocity, now);
    }
}

void LedPatternBase::m_transform(uint8_t index, float angleOffset, LedPosition &position)
{
    uint32_t scaledAngle = angleOffset / (2 * PI) * ((1 << 16) - 1);
    position.angle = angles[index] + scaledAngle; // This will wrap around as needed.
    // position.angle = angles[index];
    position.radii = radii[index];
    position.x = (cos16(position.angle) * (position.radii - 1)) >> 16;
    position.y = (sin16(position.angle) * (position.radii - 1)) >> 16;
}

inline uint8_t LedPatternBase::m_pulsedSine(uint8_t index)
{
    return index < 128 ? 255 : cos8(2 * (index - 128));
}

CRGBPalette16 rainbowPalete = RainbowColors_p;
CRGBPalette16 rainbowStripePalete = RainbowStripeColors_p;

void VerticalLedPattern::m_updateLed(uint8_t index, LedPosition &position, float velocity, uint32_t time)
{
    // leds[index] = ColorFromPalette(currentPalete, (position.y >> 8) + index);
    leds[index] = ColorFromPalette(rainbowPalete, (position.radii >> 8) + ((time * 255L / 3000L) * (abs(velocity) / 10 + 1)));
}

void RedGreenWedges::m_updateLed(uint8_t index, LedPosition &position, float velocity, uint32_t time)
{
    const uint16_t segments = 8;
    uint16_t segment = position.angle / ((1 << 16) / segments);
    // uint8_t brightness = m_pulsedSine((time * 255L / 3000L - (position.radii >> 8))); // * (abs(velocity)/28 + 1));
    uint8_t brightness = m_pulsedSine((time * 255L / 3000L) * (abs(velocity) / 10 + 1) - (position.radii >> 8)); // * (abs(velocity)/28 + 1));
    int32_t thresholdRadii = (uint32_t)abs(velocity) * 2293L;
    if ((int32_t)position.radii > thresholdRadii)
    {
        // Normal red and green.
        if (segment & 0x1)
        {
            // Odd segments.
            leds[index] = CRGB(0, brightness, 0);
        }
        else
        {
            leds[index] = CRGB(brightness, 0, 0);
        }
    }
    else if ((int32_t)position.radii > thresholdRadii - 3000)
    {
        // White line.
        leds[index] = CRGB(brightness, brightness, brightness);
    }
    else
    {
        // Blue and red
        if (segment & 0x1)
        {
            // Odd segments.
            leds[index] = CRGB(brightness, 0, 0);
        }
        else
        {
            leds[index] = CRGB(0, 0, brightness);
        }
    }
}

void LedMatrix::begin()
{
    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE, PIN_LEDS, COLOR_ORDER>(leds, NUM_LEDS);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);
}

void LedMatrix::update()
{
    taskENTER_CRITICAL(&spinlock);
    Matrix<2, 1, float> state = powerMeter.imuManager.kalman.getState();
    taskEXIT_CRITICAL(&spinlock);
    patterns[activePattern]->update(state(0, 0), state(1, 0));
    FastLED.show();

    // Move to the next pattern if needed.
    uint32_t now = millis();
    if (now - m_lastUpdateTime >= PATTERN_DISPLAY_TIME)
    {
        m_lastUpdateTime = now;
        if (++activePattern == NUM_PATTERNS)
        {
            activePattern = 0;
        }
    }
}

void taskChristmasLeds(void *pvParameters)
{
    LOGD("Christmas", "Starting the Christmas lights task");
    // LineSegment starSegments[] = {
    //     // LineSegment(0, 0, 1, 32767, true)};
    //     LineSegment(0, -32768, 2, 8509, true),
    //     LineSegment(8509, -11712, 0, 32767, true),
    //     LineSegment(32767, -11712, -1, 13768, false),
    //     LineSegment(13768, 4473, 3, 22278, true),
    //     LineSegment(22278, 30662, 1, 0, false),
    //     LineSegment(0, 14477, -1, -22277, false),
    //     LineSegment(-22277, 30662, -3, -13767, true),
    //     LineSegment(-13767, 4473, 1, -32768, false),
    //     LineSegment(-32768, -11712, 0, -8508, true),
    //     LineSegment(-8508, -11712, -2, 1, true)};
    // LedShape star = LedShape(starSegments, sizeof(starSegments) / sizeof(LineSegment));
    // YellowShape starPattern(star);
    // christmasLeds.patterns[0] = &starPattern;
    DiagonalPattern diagonalPattern;
    christmasLeds.patterns[0] = &diagonalPattern;
    RedGreenWedges patternWedges;
    christmasLeds.patterns[1] = &patternWedges;
    VerticalLedPattern patternVerticalRainbow;
    christmasLeds.patterns[2] = &patternVerticalRainbow;

    christmasLeds.begin();
    while (true)
    {
        christmasLeds.update();
        delay(1); // Wait a little while.
    }
}

const PointInside LineSegment::isInside(const LedPosition &position) const
{
    if (position.x < x0 || position.x > x1)
    {
        // This section isn't relevent.
        return PointInside::IRRELEVENT;
    }
    else if (position.y > (uint32_t)m * (position.x - x0) + y0)
    {
        // We are above the line.
        return includeAbove ? PointInside::INSIDE : PointInside::OUTSIDE;
    }
    else
    {
        // We are below the line.
        includeAbove ? PointInside::OUTSIDE : PointInside::INSIDE;
    }
}

bool LedShape::isInShape(const LedPosition &position) const
{
    bool result = true;
    bool anyRelevent = false;
    for (uint8_t i = 0; i < m_segmentCount; i++)
    {
        PointInside lineResult = m_segments[i].isInside(position);
        if (lineResult != PointInside::IRRELEVENT)
        {
            anyRelevent = true;
            result &= lineResult == PointInside::INSIDE;
        }
    }
    result &= anyRelevent;
    // LOGV("S", "%d,%d: %d", position.x, position.y, result);
    return result;
}

void ShapePattern::m_updateLed(uint8_t index, LedPosition &position, float velocity, uint32_t time)
{
    if (m_shape.isInShape(position))
    {
        leds[index] = m_insideUpdate(index, position, velocity, time);
    }
    else
    {
        leds[index] = m_outsideUpdate(index, position, velocity, time);
    }
}

CRGB ShapePattern::m_outsideUpdate(uint8_t index, LedPosition &position, float velocity, uint32_t time)
{
    // Return black by default.
    return CRGB(0, 0, 0);
}

CRGB YellowShape::m_insideUpdate(uint8_t index, LedPosition &position, float velocity, uint32_t time)
{
    return CRGB(128, 128, 0);
}

void DiagonalPattern::m_updateLed(uint8_t index, LedPosition &position, float velocity, uint32_t time)
{
    uint8_t brightness = m_pulsedSine((time * 255L / 3000L) * (abs(velocity) / 10 + 1) - (position.radii >> 8)); // * (abs(velocity)/28 + 1));
    leds[index] = ColorFromPalette(rainbowPalete, (position.x >> 8) + (position.y >> 8), brightness); // + (time * 255L / 2000L)));
    // uint8_t red = 0;
    // uint8_t green = 0;
    // uint8_t blue = 0;
    // if (position.x > 0)
    // {
    //     red = 255;
    // }
    // else
    // {
    //     green = 255;
    // }
    // if (position.y > 0)
    // {
    //     blue = 255;
    // }
    // leds[index] = CRGB(red, green, blue);
}

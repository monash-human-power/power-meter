/**
 * @file lights.h
 * @author Jotham Gates
 * @brief Displays a fancy Christmas light pattern.
 * @version 0.0.0
 * @date 2025-12-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include "defines.h"
#include <FastLED.h>

#define NUM_LEDS 100
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS 128
#define NUM_PATTERNS 3
#define PATTERN_DISPLAY_TIME 10000

/**
 * @brief Structure that represents the position of a LED.
 *
 */
struct LedPosition
{
    int16_t x;
    int16_t y;
    uint16_t angle;
    uint16_t radii;
};

/**
 * @brief Base class for a LED pattern that can be drawn.
 *
 */
class LedPatternBase
{
public:
    void update(float position, float velocity);

protected:
    /**
     * @brief Transforms the positions to match the current angle offset.
     *
     * @param index the index of the LED to retrieve.
     * @param angleOffset the angle offset.
     * @param position the position object to fill.
     */
    void m_transform(uint8_t index, float angleOffset, LedPosition &position);
    virtual void m_updateLed(uint8_t index, LedPosition &position, float velocity, uint32_t time) {}
    inline uint8_t m_pulsedSine(uint8_t index);
};

class RadialLedPattern : public LedPatternBase
{
protected:
    void m_updateLed(uint8_t index, LedPosition &position, float velocity, uint32_t time) override;
};

class RedGreenWedges : public LedPatternBase
{
protected:
    void m_updateLed(uint8_t index, LedPosition &position, float velocity, uint32_t time) override;
};

enum PointInside
{
    IRRELEVENT,
    INSIDE,
    OUTSIDE
};

/**
 * @brief Class for a line segment in a shape.
 * 
 */
class LineSegment
{
public:
    LineSegment(const int16_t x0, const int16_t y0, const int16_t m, const int16_t x1, const bool includeAbove) : x0(x0), y0(y0), m(m), x1(x1), includeAbove(includeAbove) {}
    const int16_t x0;
    const int16_t y0;
    const int16_t m;
    const int16_t x1;
    const bool includeAbove;

    const PointInside isInside(const LedPosition &position) const;
};

/**
 * @brief Class for a list of line segments that represent a shape.
 * 
 */
class LedShape
{
public:
    LedShape(const LineSegment *segments, const uint8_t segmentCount) : m_segments(segments), m_segmentCount(segmentCount) {}
    bool isInShape(const LedPosition &position) const;

private:
    const LineSegment *m_segments;
    const uint8_t m_segmentCount;
};

/**
 * @brief Base class for a pattern that is comprised of a single shape.
 * There is an issue somewhere, but I ran out of patience to debug it.
 * 
 */
class ShapePattern : public LedPatternBase
{
public:
    ShapePattern(const LedShape &shape) : m_shape(shape) {}

protected:
    const LedShape &m_shape;
    void m_updateLed(uint8_t index, LedPosition &position, float velocity, uint32_t time) override;

    virtual CRGB m_insideUpdate(uint8_t index, LedPosition &position, float velocity, uint32_t time) {}
    virtual CRGB m_outsideUpdate(uint8_t index, LedPosition &position, float velocity, uint32_t time);
};

/**
 * @brief Shape where the foreground is yellow and background is black.
 * There is an issue somewhere, but I ran out of patience to debug it.
 * 
 */
class YellowShape : public ShapePattern
{
    using ShapePattern::ShapePattern;
protected:
    CRGB m_insideUpdate(uint8_t index, LedPosition &position, float velocity, uint32_t time) override;
    // CRGB m_outsideUpdate(uint8_t index, LedPosition &position, float velocity, uint32_t time) override;
};

class DiagonalPattern : public LedPatternBase
{
protected:
    void m_updateLed(uint8_t index, LedPosition &position, float velocity, uint32_t time) override;
};

/**
 * @brief Class the controls the LEDs.
 * 
 */
class LedMatrix
{
public:
    void begin();
    void update();
    LedPatternBase *patterns[NUM_PATTERNS];
    int activePattern = 0;

private:
    uint32_t m_lastUpdateTime;
};

void taskChristmasLeds(void *pvParameters);
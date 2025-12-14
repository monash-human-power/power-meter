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
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS  255

/**
 * @brief Structure that represents the position of a LED.
 * 
 */
struct LedPosition
{
    uint16_t x;
    uint16_t y;
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
    void update(float position);
protected:
    /**
     * @brief Transforms the positions to match the current angle offset.
     * 
     * @param index the index of the LED to retrieve.
     * @param angleOffset the angle offset.
     * @param position the position object to fill.
     */
    void m_transform(uint8_t index, float angleOffset, LedPosition &position);
    virtual void m_updateLed(uint8_t index, LedPosition &position) {}
};

class VerticalLedPattern: public LedPatternBase
{
protected:
    void m_updateLed(uint8_t index, LedPosition &position) override;
};

class RedGreenWedges: public LedPatternBase
{
protected:
    void m_updateLed(uint8_t index, LedPosition &position) override;
};

class LedMatrix
{
public:
    void begin();
    void update();
    LedPatternBase* activePattern;
private:
};

void taskChristmasLeds(void *pvParameters);
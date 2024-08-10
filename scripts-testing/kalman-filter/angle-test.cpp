#include <iostream>
#include <math.h>
// #define M_PI 3.141592653589793
// #define M_PI_2 (M_PI/2)

float limitAngle(float input)
{
    // input = fmod(input, 2 * M_PI);
    // if (input > M_PI)
    // {
    //     input -= 2 * M_PI;
    // }
    // return input;
    if (input > M_PI)
    {
        input -= 2*M_PI;
    } else if (input <= -M_PI)
    {
        input += 2*M_PI;
    }
    return input;
}

float m_calculateAngle(float x, float y)
{
    if (x != 0)
    {
        // Most cases, avoiding any chance of divide by 0 errors.
        float angle = atanf(y / x);
        if (x > 0)
        {
            // First or fourth quadrants
            return angle;
        }
        else
        {
            // Second or third quadrants
            if (y >= 0)
            {
                // Second
                return M_PI + angle;
            }
            else
            {
                // Third
                return -M_PI + angle;
            }
        }
    }
    else
    {
        // Very rare case when x is 0 (avoid division by 0).
        if (y >= 0)
        {
            // Straight up.
            return M_PI_2;
        }
        else
        {
            // Straight down.
            return -M_PI_2;
        }
    }
}

int main()
{
    // float x = 0;
    // float y = 0;
    // float angle = m_calculateAngle(x, y);
    // printf("X=%10.2f\nY=%10.2f\nTheta=%6.2f (%5.2f*pi)\n", x, y, angle, angle/M_PI);

    float in = -4;
    float out = limitAngle(in);
    printf("In=%10.2f (%5.2f*pi)\nOut=%9.2f (%5.2f*pi)\n", in, in/M_PI, out, out/M_PI);
    return 0;
}
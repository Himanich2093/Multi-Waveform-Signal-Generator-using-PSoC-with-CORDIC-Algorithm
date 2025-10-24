#include <project.h>
#include <math.h>

/******************** CONSTANTS ********************/
#define CORDIC_ITERATIONS 16
#define PI 3.14159265358979323846f
#define SAMPLE_RATE 1000     // 1 kHz sampling
#define DAC_MAX 4095         // 12-bit DAC range

/******************** CORDIC FUNCTION ********************/
void cordic_sin_cos(float theta, float *sin_val, float *cos_val) {
    // Normalize theta to [0, 2π)
    theta = fmodf(theta, 2.0f * PI);
    
    // Pre-rotation to get theta into the range [-π/2, π/2]
    int quadrant = 0;
    if (theta > PI / 2.0f && theta <= PI) {
        theta = PI - theta;
        quadrant = 1;
    } else if (theta > PI && theta <= 3.0f * PI / 2.0f) {
        theta = theta - PI;
        quadrant = 2;
    } else if (theta > 3.0f * PI / 2.0f) {
        theta = 2.0f * PI - theta;
        quadrant = 3;
    }
    
    // CORDIC gain compensation factor
    const float K = 0.6072529350088812561694f; // Precalculated gain compensation
    
    float x = K;
    float y = 0.0f;
    float angle = 0.0f;
    
    static const float angles[CORDIC_ITERATIONS] = {
        0.7853981633974483f, 0.4636476090008061f, 0.24497866312686414f, 
        0.12435499454676144f, 0.06241880999595735f, 0.031239833430268277f, 
        0.015623728620476831f, 0.007812341060101111f, 0.0039062301319669718f, 
        0.0019531225164788188f, 0.0009765621895593195f, 0.0004882812111948983f, 
        0.00024414062014936177f, 0.00012207031189367021f, 6.103515617420877e-05f, 
        3.0517578115526096e-05f
    };
    
    for (int i = 0; i < CORDIC_ITERATIONS; i++) {
        float x_new, y_new;
        if (theta > angle) {
            x_new = x - (y / (1 << i));
            y_new = y + (x / (1 << i));
            angle += angles[i];
        } else {
            x_new = x + (y / (1 << i));
            y_new = y - (x / (1 << i));
            angle -= angles[i];
        }
        x = x_new;
        y = y_new;
    }
    
    // Apply quadrant correction
    switch (quadrant) {
        case 1:
            x = -x;
            break;
        case 2:
            x = -x;
            y = -y;
            break;
        case 3:
            y = -y;
            break;
    }
    
    if (sin_val) *sin_val = y;
    if (cos_val) *cos_val = x;
}

/******************** WAVEFORM GENERATION ********************/
uint16_t generate_sample(float theta, float amplitude, float phase, char waveform_type) {
    uint16_t dac_value = 0;
    
    /* Ensure theta is normalized between 0 and 2π */
    theta = fmodf(theta + phase, 2.0f * PI);
    if (theta < 0) theta += 2.0f * PI;

    /* Waveform generation */
    switch (waveform_type) {
        case 's': { // Sine
            float sin_val;
            cordic_sin_cos(theta, &sin_val, NULL);
            dac_value = (uint16_t)(2047.5f + 2047.5f * sin_val * amplitude);
            break;
        }
        case 'c': { // Cosine
            float cos_val;
            cordic_sin_cos(theta, NULL, &cos_val);
            dac_value = (uint16_t)(2047.5f + 2047.5f * cos_val * amplitude);
            break;
        }
        case 't': { // Triangle
            float normalized = theta / (2.0f * PI);
            float triangle_val;
            
            if (normalized < 0.25f)
                triangle_val = 4.0f * normalized;
            else if (normalized < 0.75f)
                triangle_val = 2.0f - 4.0f * normalized;
            else
                triangle_val = -4.0f + 4.0f * normalized;
                
            dac_value = (uint16_t)(2047.5f + 2047.5f * triangle_val * amplitude);
            break;
        }
        case 'q': { // Square
            dac_value = (theta < PI) ? 
                       (uint16_t)(2047.5f + 2047.5f * amplitude) : 
                       (uint16_t)(2047.5f - 2047.5f * amplitude);
            break;
        }
        case 'd': { // DC
            dac_value = (uint16_t)(2047.5f + 2047.5f * amplitude);
            break;
        }
    }
    
    // Ensure value is within DAC range
    if (dac_value > DAC_MAX) dac_value = DAC_MAX;
    if (dac_value < 0) dac_value = 0;
    
    return dac_value;
}

/******************** MAIN FUNCTION ********************/
int main(void) {
    CyGlobalIntEnable;
    
    /*** Hardware Initialization ***/
    USBUART_Start(0, USBUART_5V_OPERATION);
    while (!USBUART_GetConfiguration()); // Wait for USB enumeration
    USBUART_CDC_Init();
    DAC_Start();
    
    /*** Default Parameters ***/
    char waveform_type = 's';  // Default: sine wave 
    float amplitude = 1.0f;    // Default: full amplitude
    float frequency = 1.0f;    // Default: 1 Hz
    float phase = 0.0f;        // Default: 0 phase
    float theta = 0.0f;        // Current angle
    
    uint16_t dac_value;
    uint8_t receive_buffer[4];
    uint16_t bytes_read;

    while (1) {
        /*** Parameter Update from MATLAB ***/
        if (USBUART_DataIsReady()) {
            bytes_read = USBUART_GetAll(receive_buffer);
            
            if (bytes_read == 4) {  // Make sure we got all 4 bytes
                // Convert waveform index to corresponding character
                switch(receive_buffer[0]) {
                    case 0: waveform_type = 's'; break; // Sine
                    case 1: waveform_type = 'c'; break; // Cosine
                    case 2: waveform_type = 'q'; break; // Square
                    case 3: waveform_type = 't'; break; // Triangle
                    case 4: waveform_type = 'd'; break; // DC
                    default: waveform_type = 's'; break; // Default to sine
                }
                
                frequency = (float)receive_buffer[1];
                amplitude = (float)receive_buffer[2] / 255.0f;
                phase = (float)receive_buffer[3] / 255.0f * 2.0f * PI;
            }
        }
        
        /*** Generate current sample ***/
        dac_value = generate_sample(theta, amplitude, phase, waveform_type);
        
        /*** Output to DAC (oscilloscope) ***/
        DAC_SetValue(dac_value);
        
        /*** Update theta for next cycle ***/
        theta += 2.0f * PI * frequency / SAMPLE_RATE;
        if (theta >= 2*PI) theta -= 2*PI;  // Keep theta in [0, 2π)
        
        /*** Wait for next sample time ***/
        CyDelayUs(1000000/SAMPLE_RATE);
    }
}
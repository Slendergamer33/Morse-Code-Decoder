#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "includes/seven_segment.h"
#include "potentiometer.c"
#include "buzzer.c"
#include <math.h>

#define BUTTON_PIN 16	// (GPIO 16) Left Button
#define BUTTON_PIN_2 4 // (GPIO 4) Right Button
#define DOT_THRESHOLD 250 
#define BUZZER_PIN 17 // (GPIO 17) Buzzer


#define R 13 // (GPIO Pins for all RGB light channels) 
#define G 12 
#define B 11 

#define BRIGHTNESS 50 // RGB Light Config
#define LOOP_SLEEP 10
#define MAX_COLOUR_VALUE 255
#define MAX_PWM_LEVEL 65535

#define UP true
#define DOWN false

absolute_time_t button_press_start_time;  // Variable to store the time when the button was pressed
absolute_time_t button_unpress_start_time;
absolute_time_t letter_start_time;
int letter_total_time = 0;
bool button_was_pressed = false;          // Flag to track if the button was previously pressed
bool button_was_unpressed = false;
bool is_pressing = false;                 // Flag to track if the button is currently being pressed
bool found = false;
char output[100] = "";      // to record output from button presses (dot or dash or begin new letter)
int current_state;
int pressed ;
int threshold;
bool end_code = false; // boolean statement that determines if code ends
bool continued = false; //boolean statement that determines if code continues
int valid_count = 0; // Letter Counter
char valid_attempt_output[100] = ""; // Store last 4 valid letters
int roundedValue; // Rounded value of the pentiomeater
// --------------------------------------------------------------------
// declare the function definitions, e.g, decoder(...); and ther functions
// given the user input, you can decode if the input is a character
void decoder();

void setup_rgb() 
{
    // Tell the LED pins that the PWM is in charge of its value.
    gpio_set_function(R, GPIO_FUNC_PWM);
    gpio_set_function(G, GPIO_FUNC_PWM);
    gpio_set_function(B, GPIO_FUNC_PWM);

    // Figure out which slice we just connected to the LED pin, this is done for the other colors below
    uint slice_num = pwm_gpio_to_slice_num(R);
    // Get the defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, true);

    slice_num = pwm_gpio_to_slice_num(G);
    pwm_init(slice_num, &config, true);

    slice_num = pwm_gpio_to_slice_num(B);
    pwm_init(slice_num, &config, true);
}

void show_rgb(int r, int g, int b)
{
    pwm_set_gpio_level(R, ~(MAX_PWM_LEVEL * r / MAX_COLOUR_VALUE * BRIGHTNESS / 100));
    pwm_set_gpio_level(G, ~(MAX_PWM_LEVEL * g / MAX_COLOUR_VALUE * BRIGHTNESS / 100));
    pwm_set_gpio_level(B, ~(MAX_PWM_LEVEL * b / MAX_COLOUR_VALUE * BRIGHTNESS / 100));
}

void off_rgb(){
    show_rgb(0, 0, 0);
}

void seven_segment_light_all(){
    for (uint i = 0; i <8; i++){
        gpio_put(ALL_SEGMENTS[i], 0);
    }
} // Enables all parts of 7 Segment Display

void playDot() {
	buzzer_enable(0);
	sleep_ms(150);
	buzzer_enable(50000);
} // tune for dot

void playDash() {
	buzzer_enable(0);
	sleep_ms(300);
	buzzer_enable(50000);
}// tune for dash

void playError() {
    buzzer_enable(150);
    sleep_ms(50);
    buzzer_enable(50000);
    sleep_ms(50);
    buzzer_enable(150);
    sleep_ms(50);
    buzzer_enable(50000);
}// tune for error

void playSong() {
    buzzer_enable(554.37);
  sleep_ms(800);
  buzzer_enable(50000);
  sleep_ms(80);
  buzzer_enable(493.88);
  sleep_ms(700);
  buzzer_enable(50000);
  sleep_ms(70);
  buzzer_enable(466.16);
  sleep_ms(450);
  buzzer_enable(50000);
  sleep_ms(60);
  buzzer_enable(1108.73);
  sleep_ms(300);
  buzzer_enable(50000);
  sleep_ms(30);
  buzzer_enable(987.77);
  sleep_ms(200);
  buzzer_enable(50000);
  sleep_ms(20);
  buzzer_enable(932.33);
  sleep_ms(200);
  buzzer_enable(50000);

}

void continue_code() {
    continued = false;
    printf("Would you like to continue?\n");
    printf("(left button = yes, right button = no)\n");
    off_rgb();
    
    while (valid_count == 4) {
        //Left Button YES
        if (gpio_get(BUTTON_PIN) == 1) {
            printf("Continuing....\n");
            continued = true;
            valid_count = 0;
            strcpy(valid_attempt_output, "");
            show_rgb(0,255,0);
            sleep_ms(100);
            

        }
        // Right Button NO
        if (gpio_get(BUTTON_PIN_2) == 1) {
            valid_count = 0;
            strcpy(valid_attempt_output, "");
            show_rgb(255,0,0);
            sleep_ms(100);
            end_code = true;
        }
    }


} // Meathod to determine if code continues

int main() {
    
	timer_hw->dbgpause = 0;
	stdio_init_all();
    // Initialise the buzzer.
    buzzer_init();
    // Initialise the potentiometer.
    potentiometer_init();
    //setups rgb
    setup_rgb();
    off_rgb();

	// Initialise the seven segment display.
	seven_segment_init();
    
    seven_segment_light_all();

	// Turn the seven segment display off when the program starts.

	// Initialise the button's GPIO pin.
	gpio_init(BUTTON_PIN);
	gpio_set_dir(BUTTON_PIN, GPIO_IN);
	gpio_pull_down(BUTTON_PIN); // Pull the button pin towards ground (with an internal pull-down resistor).

    gpio_init(BUTTON_PIN_2);
	gpio_set_dir(BUTTON_PIN_2, GPIO_IN);
	gpio_pull_down(BUTTON_PIN_2); // Pull the button pin towards ground (with an internal pull-down resistor).

    printf("Welcome to the morse decoder!\n");
    sleep_ms(3000);


    char checkButton(int duration) {
      
        if (duration == 0) {
            current_state = 0;
            return '0'; // nothing (just to stop it repeatedly adding dots)
        } else if (duration < 250) {
            playDot();
            current_state = 1;
            return '1'; // dot
        } else if (duration < 700 && duration > 250) {
            playDash();
            current_state = 2;
            return '2'; // dash
        } else {
            return 0;
        }
    }








    void decoder(char* output){
        // turn off existing segment display
        seven_segment_off();
        bool found;
        // morse alphabet
        const char* morse_alphabet[26][2] = {
        {"A", ".-"},    // A
        {"B", "-..."},  // B
        {"C", "-.-."},  // C
        {"D", "-.."},   // D
        {"E", "."},     // E
        {"F", "..-."},  // F
        {"G", "--."},   // G
        {"H", "...."},  // H
        {"I", ".."},    // I
        {"J", ".---"},  // J
        {"K", "-.-"},   // K
        {"L", ".-.."},  // L
        {"M", "--"},    // M
        {"N", "-."},    // N
        {"O", "---"},   // O
        {"P", ".--."},  // P
        {"Q", "--.-"},  // Q
        {"R", ".-."},   // R
        {"S", "..."},   // S
        {"T", "-"},     // T
        {"U", "..-"},   // U
        {"V", "...-"},  // V
        {"W", ".--"},   // W
        {"X", "-..-"},  // X
        {"Y", "-.--"},  // Y
        {"Z", "--.."}   // Z
        };
        // checks for a match between the output and the morse codes in the list
        if (strcmp(output, "") == 0) {
            return;
        } //Base case if there is no input
        for (int i = 0; i < 26; i++) {
            if (strcmp(output, morse_alphabet[i][1]) == 0) {
                // if match found, display the letter
                found = true;
                seven_segment_show(i);
                show_rgb(0,255,0);
                valid_count += 1;
                strcat(valid_attempt_output, morse_alphabet[i][0]);
                // Count check
                if (valid_count == 4) {
                    printf("%s\n", valid_attempt_output);
                    playSong();
                    continue_code();
                }
                return;
            }
        } // If inputed morse alphabet isnt in the array
        if (!found) {
            playError();
            show_rgb(255,0,0);
            seven_segment_light_all();
        }


    }
    uint64_t duration_ms = 0;
    

    //potentiometer
    printf("Adjust the potentiometer to change the input time interval\n");
    printf("Press the left button to confirm your adjustments\n");
    while (gpio_get(BUTTON_PIN) == 0) {
            uint val = potentiometer_read_raw();

            // Convert to a double for division
            double val_divided = (double)val / 1000;

            // Rounding the integer value
            roundedValue = (uint)(val + 50) / 100 * 100; // Manual rounding for integers
            roundedValue += 2000;
            printf("Potentiometer value: %u\n", roundedValue);


    }

    // Runtime loop
    printf("Potentiometer value set to %u\n", roundedValue);
	while (true) {
        

        off_rgb();

        if (end_code) {
        return 0;
        }

        uint64_t duration_of_unpress = absolute_time_diff_us(button_unpress_start_time, get_absolute_time());  // Duration in microseconds
        uint64_t unpress_duration_ms = duration_of_unpress / 1000;  // Convert to milliseconds

        if (current_state == 0) {
            threshold = 400;
        } else if (current_state == 1) {
            threshold == 550;
        } else if (current_state == 2) {
            threshold = 700;
        }

        if (unpress_duration_ms > threshold && button_was_unpressed) {
            button_was_unpressed = false;

            decoder(output);
            
            
            strcpy(output, "");
        }

		if (gpio_get(BUTTON_PIN) == 1 && !button_was_pressed) {  // Button is pressed
            letter_start_time = get_absolute_time();
            button_press_start_time = get_absolute_time();  // Record the press start time
            button_was_pressed = true;  // Set the flag to true
        }

        if (gpio_get(BUTTON_PIN) == 0 && !button_was_unpressed) {  // Button isn't pressed
            button_unpress_start_time = get_absolute_time();  // Record when button unpressed
            button_was_unpressed = true;  // Set the flag to true
        }

        if (gpio_get(BUTTON_PIN) == 1 && button_was_unpressed) {
            
            button_was_unpressed = false;  // Reset the flag
            // CHECKS if button has been up for 700 ms. if so, pass the output string into the decoder and then reset it
        }

        // If button is released and was previously pressed
        if (gpio_get(BUTTON_PIN) == 0 && button_was_pressed) {
            uint64_t duration = absolute_time_diff_us(button_press_start_time, get_absolute_time());  // Duration in microseconds
            uint64_t duration_ms = duration / 1000;  // Convert to milliseconds
            button_was_pressed = false;  // Reset the flag
            if (duration_ms > 700) {
                printf("This input is an error.");
                seven_segment_light_all();
                playError();

            }
            if (checkButton(duration_ms) == '1') {
                strcat(output, ".");
            } else if (checkButton(duration_ms) == '2') {
                strcat(output, "-");
            }
        }
        if (continued) {
            strcmp(output, "");
            valid_count = 0;
            strcmp(valid_attempt_output, "");
            continued = false;
        }

        uint64_t letter_time = absolute_time_diff_us(letter_start_time, get_absolute_time());  // Duration in microseconds
        uint64_t letter_time_ms = letter_time / 1000;  // Convert to milliseconds
        letter_total_time += letter_time_ms;
        sleep_ms(15);
	}
}





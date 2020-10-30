//
// portmap.h
//
// Use this in your final project so that your code is independent
// of the way you mapped your ports. Your instructor also has a
// microcontroller board and his own portmap.c and portmap.h and he
// will compile your code with his portmap.c to test your project.
//
// Revision History
// 3/21/20   Initial Version   Dr. Brown


// return the state of the 8-bit DIP SWITCH
// 1 = open, 0 = closed
unsigned char get_switches();

// return the state of the pushbuttons in bit 0 and bit 1
// 1 = open, 0 = closed. other bits (2-7) are unknown
unsigned char get_buttons();

//
// turn off all LEDs
void clear_leds();

//
// turn on those LEDs where the corresponding bit in x is 1
// least significant bit of x corresponds to rightmost diode (D1)
void set_leds(int x);

//
// Initialize ports, in case your LEDs are active high and you have
// to put those port lines into a push-pull output mode.
//
void init_portmap();

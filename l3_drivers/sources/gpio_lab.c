#include "gpio_lab.h"
#include "lpc40xx.h"

/// PA.B   Port A - Pin B
// functions are able to handle multiple ports

/// Should alter the hardware registers to set the pin as input
void gpio_input(uint8_t port_num, uint8_t pin_num) {
  if (port_num == 0) {
    LPC_GPIO0->DIR &= ~(1 << pin_num);
  } else if (port_num == 1) {
    LPC_GPIO1->DIR &= ~(1 << pin_num);
  } else if (port_num == 2) {
    LPC_GPIO2->DIR &= ~(1 << pin_num);
  } else if (port_num == 3) {
    LPC_GPIO3->DIR &= ~(1 << pin_num);
  } else if (port_num == 4) {
    LPC_GPIO4->DIR &= ~(1 << pin_num);
  }
}

/// Should alter the hardware registers to set the pin as output
void gpio_output(uint8_t port_num, uint8_t pin_num) {
  if (port_num == 0) {
    LPC_GPIO0->DIR |= (1 << pin_num);
  } else if (port_num == 1) {
    LPC_GPIO1->DIR |= (1 << pin_num);
  } else if (port_num == 2) {
    LPC_GPIO2->DIR |= (1 << pin_num); /*enable the output direction*/
  } else if (port_num == 3) {
    LPC_GPIO3->DIR |= (1 << pin_num);
  } else if (port_num == 4) {
    LPC_GPIO4->DIR |= (1 << pin_num);
  }
}

/// Should alter the hardware registers to set the pin as high
void gpio_high(uint8_t port_num, uint8_t pin_num) {
  if (port_num == 0) {
    LPC_GPIO0->PIN |= (1 << pin_num);
  } else if (port_num == 1) {
    LPC_GPIO1->PIN |= (1 << pin_num);
  } else if (port_num == 2) {
    LPC_GPIO2->PIN |= (1 << pin_num); /*enable output HIGH 3.3v*/
  } else if (port_num == 3) {
    LPC_GPIO3->PIN |= (1 << pin_num);
  } else if (port_num == 4) {
    LPC_GPIO4->PIN |= (1 << pin_num);
  }
}

/// Should alter the hardware registers to set the pin as low
void gpio_low(uint8_t port_num, uint8_t pin_num) {
  if (port_num == 0) {
    LPC_GPIO0->PIN &= ~(1 << pin_num);
  } else if (port_num == 1) {
    LPC_GPIO1->PIN &= ~(1 << pin_num);
  } else if (port_num == 2) {
    LPC_GPIO2->PIN &= ~(1 << pin_num);
  } else if (port_num == 3) {
    LPC_GPIO3->PIN &= ~(1 << pin_num);
  } else if (port_num == 4) {
    LPC_GPIO4->PIN &= ~(1 << pin_num);
  }
}

/**
 * Should alter the hardware registers to set the pin as low
 *
 * @param {bool} high - true => set pin high, false => set pin low
 */
void gpio_set(uint8_t port_num, uint8_t pin_num, bool high) {
  if (high == true) {
    gpio0__set_high(port_num, pin_num);
  } else if (high == false) {
    gpio0__set_low(port_num, pin_num);
  }
}

/**
 * Should return the state of the pin (input or output, doesn't matter)
 *
 * @return {bool} level of pin high => true, low => false
 */
bool gpio_level(uint8_t port_num, uint8_t pin_num) {
  if (port_num == 0) {
    if (LPC_GPIO0->PIN & (1 << pin_num))
      return true;
  } else if (port_num == 1) {
    if (LPC_GPIO1->PIN & (1 << pin_num))
      return true;
  } else if (port_num == 2) {
    if (LPC_GPIO2->PIN & (1 << pin_num))
      return true;
  } else if (port_num == 3) {
    if (LPC_GPIO3->PIN & (1 << pin_num))
      return true;
  } else if (port_num == 4) {
    if (LPC_GPIO4->PIN & (1 << pin_num))
      return true;
  }
  return false;
}

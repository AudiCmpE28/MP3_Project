
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "lpc_peripherals.h"

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio0_callbacks_rising[32];
static function_pointer_t gpio0_callbacks_falling[32];
static function_pointer_t gpio2_callbacks_rising[32];
static function_pointer_t gpio2_callbacks_falling[32];

void gpio0__attach_interrupt(uint32_t port_num, uint32_t pin_num, gpio_interrupt_e interrupt_type,
                             function_pointer_t callback) {
  // 1) Store the callback based on the pin at gpio0_callbacks

  if (port_num == 0) { // port 0
    if (interrupt_type == GPIO_INTR__RISING_EDGE) {
      gpio0_callbacks_rising[pin_num] = callback;
      LPC_GPIOINT->IO0IntEnR |= (1 << pin_num);
      // fprintf(stderr, "\nRising Edge Port 0\n");

    } else if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
      gpio0_callbacks_falling[pin_num] = callback;
      LPC_GPIOINT->IO0IntEnF |= (1 << pin_num);
      // fprintf(stderr, "\nFailing Edge Port 2\n");
    }

  } else if (port_num == 2) { // Port 2
    if (interrupt_type == GPIO_INTR__RISING_EDGE) {
      gpio2_callbacks_rising[pin_num] = callback;
      LPC_GPIOINT->IO2IntEnR |= (1 << pin_num);
      // fprintf(stderr, "\nRising Edge Port 2\n");

    } else if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
      gpio2_callbacks_falling[pin_num] = callback;
      LPC_GPIOINT->IO2IntEnF |= (1 << pin_num);
      // fprintf(stderr, "\nFailing Edge Port 2\n");
    }
  }

  // 2) Configure GPIO 0 pin for rising or falling edge
}

void clear_pin_interrupt(int port_num, int pin_num) {
  if (port_num == 0) {
    LPC_GPIOINT->IO0IntClr |= (1 << pin_num);
  } else if (port_num == 2) {
    LPC_GPIOINT->IO2IntClr |= (1 << pin_num);
  }
}

// We wrote some of the implementation for you
void gpio0__interrupt_dispatcher(void) {
  // Check which pin generated the interrupt

  for (int loop_pin = 0; loop_pin < 32; loop_pin++) {

    if (LPC_GPIOINT->IO0IntStatR & (1 << loop_pin)) { // Port 0 Rising edge
      function_pointer_t attached_user_handler_rising_0 = gpio0_callbacks_rising[loop_pin];
      // Invoke the user registered callback, and then clear the interrupt
      attached_user_handler_rising_0();
      clear_pin_interrupt(0, loop_pin);
      // fprintf(stderr, "\ninterrupt_dispatcher Port 0 Rising\n");
      break;
    }

    if (LPC_GPIOINT->IO0IntStatF & (1 << loop_pin)) { // Port 0 when Falling edge
      function_pointer_t attached_user_handler_falling_0 = gpio0_callbacks_falling[loop_pin];
      attached_user_handler_falling_0();
      clear_pin_interrupt(0, loop_pin);
      // fprintf(stderr, "\ninterrupt_dispatcher Port 0 Falling\n");
      break;
    }

    if (LPC_GPIOINT->IO2IntStatR & (1 << loop_pin)) { // Port 2 Rising edge
      function_pointer_t attached_user_handler_rising_2 = gpio2_callbacks_rising[loop_pin];
      attached_user_handler_rising_2();
      clear_pin_interrupt(2, loop_pin);
      // fprintf(stderr, "\ninterrupt_dispatcher Port 2 Rising\n");
      break;
    }
    if (LPC_GPIOINT->IO2IntStatF & (1 << loop_pin)) { // Port 2 when Falling edge
      function_pointer_t attached_user_handler_falling_2 = gpio2_callbacks_falling[loop_pin];
      attached_user_handler_falling_2();
      clear_pin_interrupt(2, loop_pin);
      // fprintf(stderr, "\ninterrupt_dispatcher Port 2 Falling\n");
      break;
    }
  }
  // end of for
}
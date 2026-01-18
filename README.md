# Urbanite Version 1 support files

This folder contains the support files for the Version 1 of the Urbanite project.

**Attention**
The files are not organized in directories. The student must place them correctly in the appropriate `port` and `common` folders. Header files `.h` must be placed in `include` and source files `.c` must be placed in `src`.

Be careful with the `port` folder which contains the HW dependent files.

- `port_button.h` contains functions whose prototype does not depend on the microcontroller. This is the *contract* between the application and the microcontroller specific code. The implementation of these functions is in the microcontroller specific code: `stm32f4_button.c`.
- `stm32f4_button.h` contains among other things the definition of the `stm32f4_button_hw_t` structure and the prototype of the functions that receive or return microcontroller-dependent variables. The implementation of these functions is in `stm32f4_button.c`.

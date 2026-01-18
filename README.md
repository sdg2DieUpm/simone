# Urbanite Version 3 Test files

This folder contains the test files for unit testing and example of use of **Version 3 of the Urbanite** project. You can add more test items on the corresponding files at your own.

Unit tests are located in the `test` folder. Example tests are located in the `example` folder. The folders also contain the `CMakeLists.txt` files that are used to build the binary files for the test codes.

**Attention**
You must organize the test files accordingly to the project structure. The student must **save the `test` files as follows**.

- Save the file `test_port_display.c` in the `test/stm32f4` folder.
- Save the file `test_fsm_display.c` directly in the `test` folder.
- Save the file `example_v3.c` in the `example` folder.

It looks like this:

📂**example**  
 ┣ 📜CMakeLists.txt  
 ┃ 📑example_v1.c  
 ┃ 📑example_v2.c  
 ┗ 📑*example_v3.c*
 ...  
 📂**test**  
 ┣ 📂**stm32f4**  
 ┃ ┣ 📜CMakeLists.txt  
 ┃ ┃ 📑test_port_button.c  
 ┃ ┃ 📑*test_port_display.c*
 ┃ ┃ 📑test_port_ultrasound_full.c  
 ┃ ┃ 📑test_port_ultrasound_timer_trigger.c  
 ┃ ┃ 📑test_port_ultrasound_timer_echo.c  
 ┃ ┗ 📑test_port_ultrasound_timer_measurements.c  
 ┣ 📜CMakeLists.txt  
 ┃ 📑test_fsm_button.c  
 ┃ 📑*test_fsm_display.c*  
 ┗ 📑test_fsm_ultrasound.c  

## Unit tests

Unit tests depends on [`unity`](https://github.com/ThrowTheSwitch/Unity) library to create the test.

1. **Unit test of the `PORT` part must be run first.** Unit test of the `PORT` part are located in the `test/stm32f4` folder because the `PORT` module is platform dependent.

2. **Unit test of the `COMMON` part must be run afterwards.** Unit tests of the `COMMON` part are located in the `test` folder directly because the `COMMON` module is platform independent.

## Example tests

Example tests does not depend on any library to create the test. It is a small program that tests some features of the system. It is the responsibility of the developer to check if the test is correct or not.

3. **Example test of the complete `Version` must be run the last.** Examples of the each version of the project are located in the `example`.

# Working files
## ./vial-qmk/keyboards/cmtkey
- halconf.h: Enables/disables HAL (hardware abstraction layer) modules and drivers used by the firmware.
- config.h: QMK-specific configuration (USB descriptors, debounce, tapping, feature toggles) for the board/keymap.
- mcuconf.h: MCU/ChibiOS low-level settings (clock, timers, serial/I2C/SPI drivers) tailored to the target chip.
- keyboard.json: Vial/VIA layout and metadata file defining keys, matrix, lighting, and remapping UI.
- rules.mk: Build options for QMK (features, optimizations, MCU/board, bootloader, custom sources).

## ./vial-qmk/drivers
- After discussion on the vial-qmk discord, the IQS9150 is being recognised as a touch sensor by qmk, review my commit history to understand the commits and the changes made.
- `./sensors/azoteq_iqs9150.c` and `./sensors/azoteq_iqs9150.h` are the primary working files
- `./sensors/_azoteq_iqs9150.c` and `./IQS9150_init.h` are some test files
- With the current setup, the changes that you make the the working files will reflect directly when you make the the binary for the vial keymap.
- You can use `uprintf` and qmkconsole for all debug messages

# Reference Files
- https://hub.mechasystems.com/workspace/3e66fd99-ff6a-4456-bdd1-e30a8281033e/rNraR8aobuoM4_CENGG9A



# Setup and Flashing Instructions
- Connect SWDIO, SWDCLK and GND to the ST-Link
- Connect USB to the computer, and run `lsusb` to check for detection 
- There are multiple keymaps which correlate to the language/setting of the keyboard:
  - The `defaut` keymap is the bare minimum to test for any config issues, this does not incorporate all the multi touch and vial features
  - The `vial` keymap is the working keymap and can be used to modify behavior and layout
- To make run: `make cmtkey:default` to make the .bin file in the default keymap or `make cmtkey:vial` to make the .bin file in the vial keymap 
- Refer to this video for understanding this further [video](https://www.youtube.com/watch?v=O8pdUPqPG3k)
- To flash connect the st-link and run `make cmtkey:vial:st-flash` 
- Another alternative method of flashing is to make the binary file, find it in the vial-qmk root dir and use st-cube programmer to flash the code. This way if Chirag needs to flash an independent PCB he can do it. 

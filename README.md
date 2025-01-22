# coralmicro_in_tree_andon_system

## Setup

Paste the linker script info to `./coralmicro/libs/nxp/rt1176-sdk/MIMXRT1176xxxxx_cm7_ram.ld`


## Build the application

Update the project to the `app` directory CMakeLists.txt file:
```cmake
add_subdirectory(coralmicro_in_tree_andon_system)
```

Now you should be able to build the application by running the following command:
```bash
bash build.sh
```

## Upload the application

To upload the application to the Coral Dev Board, you can run the following command:

```bash
python3 scripts/flashtool.py --app coralmicro_in_tree_andon_system
```

## Run the application

I recommend using a USB to Serial adapter to connect to the Coral Dev Board. 
Using one your can view the serial output with the following command:

```bash
picocom -b 115200 /dev/ttyUSB0
```

Exit with:
```
Ctrl-a Ctrl-x
```

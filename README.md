# NRF52_Zephyr_Custom_Board_Blinky
Using Zephyr to set up a Blinky on a NRF52840DK board as well as a custom board running the nrf52840 mcu. This is the first in a line of projects to explain, how Zephyr works.

## General description
Zephyr is a software package that is more and more commonly implemented into mcu-s. It is a whole bunch of libraries as well as an integrated multithreading opertaing system, plus stacks to run WiFi and Bluetooth. It is pretty much ST’s HAL merged with FreeRTOS plus the aforementioned com stacks.

Why is it good? Well, because the hardware layers are completely detached from the application, meaning that an appropriately written app can be ported to any Zephyr-compatible device without changing the app. One merely would need to adjust the hardware layers. In some sense, this makes Zephyr similar to how FPGA hardware is set separate to the Verilog/VHDL coding.

Fine, but why do I want to bother with Zephyr?

Zephyr is the go-to environment used on Nordic’s chips, something I have been looking into to run BLE ever since I have failed to be impressed by ST’s WB5 mcu-s. Also, some nrf52’s are compatible with Arduino (Adafruit ItsyBitsy comes to mind) to do some quick tests, albeit those solutions hardly are the most efficient ways to run these devices, making them not preferred for professional desings. At any rate, I have decided to get a grip on Zephyr and implement it into some of my future projects.

This repo is the first of my notebooks on Zephyr with a small blinky project implemented on an official nordic DK board and a custom board of my own making. I am sharing the code for both, albeit the DK version should rather be done using the sample project available in Zephyr.

### Disclaimer
I am using the nrf52840DK board as the baseline for the project with all the related Zephyr board definition files already existing. A custom board running another nrf52840 will be used as an external device. This custom board can be easily "emulated" by the DK board, one merely needs to build the project for that particular board.

I have chosen the nrf52840 because it is a single core mcu which makes it a lot easier to train on compared to other chips from Nordic, such as the nrf53.

I will assume that anyone checking my notes are using VSCode with the nrf Connect plugin to program up the nrf52840. I will be referring to VSCode sections regularly.

## What makes Zephyr tick
Zephyr is an entire environment that is handled as a package. It isn’t easy to get into, mostly because one needs to get a grip of some rather unique components of Zephyr right off the bat, even for the simplest of apps, such as a basic blinky. These elements must follow a certain set of rules/syntax which, if breached, may not result in an obvious error during project build, but would still prevent us from receiving the output we were aiming for.

Anyway, let’s look at the most basic bits of Zephyr and see, how they fit together in a build. 

### Hardware definition/device tree
A device tree is a file that holds the description of the hardware, be that just the chip/soc we are using (as it is with the dtsi files) or the board we are running (the dts files).

Dtsi files are, for the most part, defined by the soc we are running on the board where they describe the peripherals (memory addresses), the memory allocation, the clocking and so on of the chip itself. Think a mix of ST’s linker file (where we define the different memory allocation addressing and MSP placement) with the device peripheral description file (usually similar to “stm32f429xx.h”) defining the memory addresses of the peripherals within the mcu.

There are multiple dtsi files sprinkled all around Zephyr:
-	“pinctrl” defines the baseline pin/peripheral connection, i.e. where – which pins - certain peripherals would be connected to.
-	“qiaa” is where we have the power regulator and the base memory allocation
-	“partition” is where we allocate memory sections to boot and app
-	“nrf52840” (or whatever our chip is called) is where the cpu’s selection is done (in case of multiple cores), the memory addresses of peripherals is defined with the size of related drivers, plus the clocking. These are all chip level values and ARE NOT adjusted to the board we are running!
-	Driver overlay dtsi files, which are there to ensure compatibility with other devices than what nordic is providing (we won’t be needing these ones since we are using the nrf52840 chip)

At the end of the day, dtsi files define our chips as they come off from the production line, giving everything t a baseline value.

Dts files on the other hand, are define for boards, not chips and modify the dtsi values of the chips to match with whatever way the board is connected up. This means adjusting voltage regulators, enabling and configuring peripherals. Think setting things up using CubeMx’s board selector as a parallel in ST’s environment. Of note, since we build every project to a particular board, selecting the right dts file is crucial to have the expected result after building and flashing our project to our device.

Pin definition will be either done in the dts file, or for peripherals, in a pinctrl dtsi file. Of note, there is a soc specific version of this file that sets the "out of the box" values for all peripherals, but there will be a proejct specific version as well that will adjust them to the project. This is necessary since many Zephyr compatible devices - such as our nrf52 - leaves the muxing of peripherals to the users, allowing is high flexbility on how we want to set up the pins. We will take a look into that in a later repo...

Anyway, what if we don’t want to use the factory device tree values and would instead want to modify the output of this existing board? That’s where “overlay” files come to play. These files are applied as – you guessed it - overlays on any existing device tree to adjust it to our need. They are NOT modifying the existing board definition code though. Overlays follow the same syntax and philosophy as dts and dtsi files and MUST be indicated within the build parameters of the project or we would end up building for the “factory” board definition, not our custom one. Overlay files can be applied on existing dts/dtsi definitions – i.e., existing board – to change the pre-set values according to what we need, such as choosing another GPIO for a LED or moving the uart/spi/i2c to an alternative set of pins (i.e., what we would do as an “init” function for the peripheral in STM32, we do it here within the overlay). Obviously, if we running a custom board, we don’t need an overlay. Instead, we need to define our own dts/dtsi files from stratch. We will only look into a simple version of this below.

#### A taxing syntax of device trees
All files above share the same syntax where we define nodes and sub-nodes in a tree-like fashion.

At the lowest level of the tree, we will have nodes that will have a set of properties that are defined for that node to function properly (think pin assignment or baud rate). The actual properties of each node are coming from the yaml file (see below) while we define the “type” of each node by setting up their compatibility (i.e., which yaml file will hold its properties).

The dtsi files will already define properties for pretty much everything, but they might not enable the peripherals. That’s the “status” line which has to be “okay” to enable the peripheral (“disabled” for turning it off).

We can give nodes “labels” which will then allow us to call that node through its label instead of the name of the actual node. Labels will be define by putting the name with a double dot before the node’s name (led0: led_0 – led0 is the label, led_0 is the node’s name).

Similarly, we can define “aliases” for nodes (referenced by their labels or node names) and organise these aliases in whatever way that we fancy (labels are individual to the original device tree). A well-defined set of aliases allow us to only reference any node through the aliases and then adjust what each means when we port our code to different devices. Changing an alias is easier and faster than digging for node labels or node names. Aliases are usually piled in the same place in a board’s dts file. (We define an alias by defining an “aliases” array and then make the alias equal to the address of the node label’s address (aliases {led = &led0;}; - led is the alias for the led0 label).

The ”chosen” can be used to assign certain functions to nodes, such as piping uart0 to the Zephyr console (think the redirection of printf to debug uart in ST devices).

Node names, labels and aliases can be whatever we want them to be as long as they are made compatible with the right node type and given the appropriate properties/values.

It is important to understand what the difference is between node names, labels and aliases though since we are using separate macros to reference the underlying nodes depending on which information we have available for the node.

#### Blueberry yaml
One file I have not touched upon yet are bindings files in format “yaml”. They are integral part of a device tree whereas they define the properties of each node through the “compatibility” section – compatibility section attaching the designated binding file to the node using the designated “compatibility” element within the binding file (usually the same as the name of the binding file). Looking at it from a different point of view, yaml files define the driver parameters we need to provide to a driver through a node. If we intend to write our own driver (i.e., module for Zephyr), we will need to write our own driver yaml file and adjust the kconfig and cmake files so Zephyr can implement it. (We won’t be doing that here to limit complexity.)

Yaml files hold the types of nodes we can have in our device. In general, it is extremely unlikely that we would need to define new nodes, even if we are to work on a custom board and do a dts file from scratch.

Of note, yaml has its own syntax.

Of note-of note, there are other (not node related) yaml files too, though these will matter for the build process only (see below).

### Macros…macros everywhere!
As mentioned above, we have multiple ways to refer to a node. What is common for all is that the macro will allow access to the node, making us capable to read whatever parameters are stored within (think pin distro, baud rate or even peripheral memory address). This step MUST be done, otherwise the peripheral drivers will not know, what parameters they should be running on.

We use DT_NODELABEL() if we have a node’s label and DT_ALIAS() if we have a node’s alias. The output in both cases is the same: an access point to the node. Technically, a pointer to the node.

We need to use this macro-extracted access point then to interact with the node, by, for instance, reading out a property of the node using DT_PROP(). An example: DT_PROP(DT_NODELABEL(node1_label), foo) will read out the “foo” property from whatever node is under the node1_label node label.

Of note, getting access to the node IS NOT the same as having the device pointer set up. We need DEVICE_DT_GET() macro with the access point provided above to have the pointer for the actual peripheral. The macro will then turn the node into a struct in c: Node references are usuaully complex structs, layered handles that hold all the configuration information for the peripheral as well as any inputs and outputs we want to have in it. The “device_is_ready()” function is then called on the pointer to check if the peripheral is properly calibrated and running. It is highly recommended to do this check on any peripheral before using it. A pointer to the node – which calibrates the peripheral – IS NOT the same as a pointer for the device which will already have the driver included already. Think that the node pointer loads the peripheral driver with the node values to actually run the peripheral.

Mind, for GPIOs, we need to use the macro GPIO_DT_SPEC_GET() to extract the pointer with the struct called “gpio_dt_spec”. The function “gpio_is_ready_dt()” is the used to check if the GPIOs are set properly.

For other peripherals, the macros might differ. What is the same is that all these macros provide a special struct that holds the access to the peripheral. These structs will reflect on the peripheral by name and content, meaning that they are not interchangeable! The exception is the “device” struct which often holds general information on all peripheral types. The structs may also hold function pointers to aid will calling the peripheral.

In general, almost all action in Zephyr is done through macros and structs. It is absolutely necessary to know, what we need to generate and how we need to use them to arrive to a working output.

### Project building blocks
The build sequence is sensitive to folder structure and file naming. If not done properly, Zephyr will not be able to find the necessary files to make the build and will throw a build error.

For starters, a high level yaml file will hold basic board information such as name and vendor. This must match with whatever name we give during the build process (plus folder structure), otherwise the build will not be wrapped around the same “board name”. This issue only comes up if we intend to define a complete custom board, otherwise we can just ignore it and use the existing board setup. In other words, the board yaml file must match the folder structure and the SOC name so during build the appropriate SOC dtsi files and drivers will be integrated into the build. (Another optional high level yaml file can be made for each cpu to use Zephyr’s test runner “Twister”, though I have not used it yet.)

The project “CmakeLists.txt” (it MUST be called as such!) will tell Zephyr, what the project’s name is, which source files (custom already built libraries) it should pull in for its build and where it should build the project. It also tells, which version of Zephyr to be used. Zephyr’s cmake files are generally simpler than original ones since many elements are integrated into Zephyr already. Of note, there are other cmake files as well for boards and libraries, though they can mostly be ignored for now (see below).
Kernel config or Kconfig is there to include/enable and customize software elements. Unlike yaml and cmake, it has its own syntax. On the most basic level, it is used by Zephry select, which SOC we are using for our build, i.e. which basic set of libraries and drivers to include in the project to simply make the chip tick over. SOCs are already set for existing board definitions such as the one for the DK, though we will have to make a matching the Kconfig file to the SOC we have on a custom board should we have one instead. Kconfig is also used to provide symbols to the “proj.conf” file, used to easily enable or disable libraries instead of going into the source code every time. Speaking of which, the “proj.conf” file is the list of drivers we want to enable for our build. Think enabling the clock registers for each peripheral in ST’s mcus. Here, it is done in a separate file. (Just in case someone is curious, the default starting Kconfig values/symbols are stored within the “.config” file in the Zephyr kernel which then can be modified using the “menuconfig” or “guiconfig” tools.)

At any rate, for any kind of simple build configuration using an existing board, we ought to give the board name and the overlay file we have defined for the devtree for that board and the rest will be sorted out by Zephyr. We may want to set the build optimized for debugging though in case we intend to do some debugging on our code. The appropriate debug flags may also need to be set to “y” in Kconfig and proj.conf to enable a proper debugging.

Don’t forget that once a build is done, it will be used as a basis for flashing the device. If we update our code, the build must be redone, or we will be flashing the same old build onto our device again and again.

It also useful to remember that Cmake and Kconfig (and the customized Kconfig files, such as proj.conf) work together and rely on each other in Zephyr, thus they should be all set up at the same time.

If we have a device connected, we will be able to see it in the “connected devices” section of VSCode.

### Flash it
Flashing of any chip will need to be done using a Segger probe and the SWD/SWCLK pins on the mcu. Personally, I have a NRF52840DK discovery board for testing and that one has an integrated Segger probe already, similar to how ST’s Nucleo and Disco boards have their on STLink included.

Now, the difference between STLink - which must be disconnected from the board’s original mcu by removing some jumpers – and the NRF52840DK board is that the DK will program any appropriate board connected to it if pin1 of either of the debug connectors (P19 or P20) is pulled HIGH - to the Vdd level of the connected external device. We don’t need to place or remove any jumpers. The caveat is though that we can’t power the external board using the DK board, unless we bridge the SB47 pads together…which would the permanently cut the connection between the original mcu and the Segger probe. All in all, best is to use the designated debug pins and power the external board, well, externally.

We can check the external board is connected up with the Segger probe by checking the “Controlled devices” section in VSCode.

## Previous relevant projects:
Ther official nrf52 training on Fundamentals level (and the Intermediate level for the custom board section):

[nRF Connect SDK Fundamentals - Nordic Developer Academy](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/)

Also, the first five and the last of Shawn Hymel’s videos are very good guidelines, albeit he is doing everything manually which makes it a lot more complicated and tedious than what it actually is when using Zephyr directly:

[Introduction to Zephyr Part 1: Getting Started - Installation and Blink | DigiKey](https://www.youtube.com/watch?v=mTJ_vKlMS_4&list=PLEBQazB0HUyTmK2zdwhaf8bLwuEaDH-52)

## To read
The Zephyr documentation (especially the board porting guide section):

[Introduction — Zephyr Project Documentation](https://docs.zephyrproject.org/latest/introduction/index.html)

And the NRF52840DK board documentation:

[nRF52840 DK Hardware](https://docs.nordicsemi.com/bundle/ug_nrf52840_dk/page/UG/dk/intro.html)

## Particularities
I will walk through all the things we need to set up to get a blinky on the NRF52840DK board, followed by an overlay on the DK board. We will finish with a custom board setup that will be compatible with the DK hardware.

### Blink where we are told
We aren’t going to reinvent the wheel here: we will open the existing “blinky” example and modify that one. We need to create a new application, copy a sample and then search for the basic blinky example. Use the second example since the first one will not copy the main.c blinky but use the “official” Zephyr one instead, something we don’t want to muck around with much. Keep in mind that there are lot of official samples stored within the “Zephyr” root in the “samples” folder and they -, as I am doing here myself - can be used as highly useful start off points for projects. Not all samples are compatible with every board though, for example, the blinky above will be defined for the “nrf54l15dk” board and not our one. Luckily, due to the simplicity of the project, it is compatible with our NRF52840DK anyway.

Once the application is copied, we need to add a build configuration for it by clicking the “Add build configuration” button. Here we need to select the board we wish to build for, the .conf file we intend to use (this will be all the Kconfig magic under the hood), select the overlay if we have one (this will be the device tree magic) and select the optimization method (i.e. if we want to debug or not). For quick test on the copied sample, we merely need to select the NRF52840DK board and select the “proj.conf” for Kconfig.

Once the first build is done, we will have every element of the project set up and we can flash the DK board with the blinky (do not forget to turn on the DK board and check if it is connected to VScode is you are doing this the first time, maybe some drivers will be missing for the Segger probe which won’t be indicated by VScode directly). Flash the code to the board using the “Flash” action and the LED1 on the board will start blinking.

#### Dissecting this blinky sample
Let’s look at what we have generated for the project:
-	main.c: this is obviously our application with the code we are running on the board. Notice that it is chuck full of macros that are driving the peripherals and interacting the devtree nodes. We are also using a lot of “return 0;” to shut off the app in case something doesn’t quite pan out the way we intend it to. It is good practice to test every macro’s result and then use the “return 0;” in case of failure.
-	CMakeList.txt: just basic project file information. We set the cmake version, point to the Zephry packages we intend to implement, give the project a nem and define only “main.c” as source. CMakeList will need to become more complex if we are to build modular projects, though in this case, we can leave it as-is.
-	Proj.conf: which drivers we are activating within our code. It will only be the “gpio” driver, thus we set it as “y”. Mind ANY other driver will have to be enabled here or we won’t be running them, no matter if we actually are setting them up in the devtree as “okay”. This is an example of a redundancy to allow portability, similar to how “alias” works in the devtree.
-	Overlay: tucked in the “boards” folder, this devtree overlay will set the “led0” node to have the properties for pin “9” and make it “ACTIVE_HIGH”. Of note, the overlay file in this example will be for nrf54l15dk, which is probably due to the sample being done for that particular board originally. We were not including this overlay file during our build.
-	Sample.yaml: the optional Twister yaml. We can ignore it.

Every other file for our project will be pulled from the Zephyr library.

Regarding the main.c, we will have the following macros:
-	DT_ALIAS will extract the node access point with the alias as a parameter
-	GPIO_DT_SPEC_GET gets the pointer struct set for the driver/node combination

Also, we will have the following driver functions:
-	the ready function tests the driver
-	the configure function then sets the underlying node parameters to what we want them to be at the moment
-	the toggle function toggles the output
-	and lastly, we k_msleep(SLEEP_TIME_MS) to time the blinking

Of note, the letters “DT” mean device tree, indicating that we need to feed the function/macro device tree information to work. Non “DT” version of functions/macros would demand direct hardware information, such as pins and ports.

### Blinky where we want it to
So, with everything set and working, how can we modify our code to make our biddings?

#### Blink here
Let’s change the LED we blink to another one on the board. What would we need to modify to get it done?

Obviously, we simply can use led1 instead of led0 for the app and it would work fine. On the other hand, we could go to our overlay file and set the led0 node up for another LED. This is the simple ”portable” solution to the problem: change led0’s “gpios” property in the overlay to be on pin 14 – P0.14 on the nrf52.

Anyway, once we have set the new led in the existing sample overlay, we need to go click on the build and edit the existing build config to include the overlay file in the build.

Once flashed, we will see that instead of the first LED, we will be blinking the second one instead. Easy.

#### Blink there
Let’s add a second LED to blink the same time along the one we already have installed on the board.

Now, I suggest connecting an external LED and NOT to use the pre-set leds since this will allow us to double-check our code’s functionality by checking the original led0.

It will also lead to an interesting realization: as long as we are connecting our external LEDs to P0.13-PP0.16 – so where the original 4 in-built DK LEDs are – we can easily toggle the output, but the moment we want to deviate from these 4 LEDs to, say, P0.06 or P0.17, the driver will not work. What gives?

It is due to three reasons, all related to how the gpio0/gpio1 nodes are set up in our sample DK board’s dts file:
- multiple gpios are already assigned and won’t change their assignment, as is with P0.06 (uart0 Rx)
- some gpios are simply “ruled out” of use through the “gpio-reserved-ranges” property. This property literally shuts off those gpios within the device tree, as it is for P0.17.
- the allowed maximum number of gpio-s is limited (“ngpios” property)
Unfortunately, neither scenarios will lead to an error during build. VSCode will underline the gpio number with a barely noticeable blue line to indicate that there is a clash though but then ignore it during build.

Anyway, if we want to unlock a specific pin, we have to “disable” any connected peripheral drivers and/or modify the gpio node to not “reserve” the gpios we want to use.

### Custom board blinky
From the discussion above, it should be clear that although pre-set boards are great, they severely limit our access to the capabilities of the board should we choose to not use them as they are set up originally (similarly how we don’t want to use the CubeMx board selector for our Nucleo boards in case we want to use a pin distribution different to the factory setup). As such, it is best to do the same blinky we have above but without using the existing board definition files.

As always, it is critical to align to the file names and extension as they are demanded, otherwise Zephyr will not find the files we need and thus fail to build the project for the custom board.

We will use the NRF52840DK board as a guideline to do our custom board. We can find all the files for this board in the zephyr driver folder, inside the “boards” folder. I recommend copy-pasting this entire folder in the folder where it is originally placed with a name adjusted to our custom board and then work from there by removing what we don’t need. Of note, this will mean that we will have the custom board indicated with a vendor “nordic”.
Let’s take a closer look of the files we will have:

#### Obligatory files
It is obligatory (!) to have a custom version of all the following files to have a custom board: 
-	custom board yaml: sets up high level meta data for board. Must match naming to Zephyr will not know, what to build for or where to find the necessary drivers (all flagged to the name of the chip)
-	custom board cmake: bare minimum to flash firmware on chip
-	custom cpu Kconfig/defconfig: default chip configuration values for the board. Of note, we have two cpu-s indicated for the board, though the one we will need to look at is just the nrf52840, the other one being an emulator for the nrf52840 to emulate a nrf52811 chip.
-	custom cpu devtree: same as before, we need to check the dts for the nrf52810 only. This is the “source” devtree we have modified using the overlay file beforehand. Now, we need a new one.
-	custom board Konfig: selects the module and/or chip on the custom board

#### Optional files
Additionally, we can have optional files (or expand on the obligatory ones) that are not necessary to have a successful build, albeit having them set up properly will help us further down the road when we want to, say, have a custom driver setup or have debugging.

The files are:
-	board cmake file: we need to add debugging enable to this
-	cpu yaml: the file to set up the Twister testing tool
-	board pinctrl dtsi: same as dts, just removed one layer from the source devtree in case we want easy porting later. We can place all data from here into our dts file if we so choose to.

#### Ignored files
We have some more files that seem useful but won’t be necessary for us. They are all related to the emulation of the nrf52811.

The files are:
-	Kconfig and Kconfig.defconfig files: Kconfig configuration values for the emulator
-	The four nrf52811 configuration files (dts, yaml, defconfig and dtsi)
-	Board CMakeLists: this is there to set up the emulator and make it look for the nrf52811 files/drivers 

## User guide
There are two folders, one for each bit of this project.

The DK version must be built using the provided DK board file without overlays.

Within the custom board definition, we are defining a complete board, so again, no overlays. The cpu defconfig was carried over (except for the name of the file) from the DK, meaning we are using the same cpu default configuration as was on the DK board. This file, independent of what you name it, MUST finish with “_defconfig”, otherwise it won’t be loaded into the project. Board Kconfig is slightly cut since we don’t need to specify the chip for the build here. Same can be said about the board cmake file where any reference to the nrf52811 was removed. Of note, in the shared dts file, I have decided to keep some unused bits such as the uart0 (status turned to “disabled”) and button0 bits since uart0 is potentially used for debugging and the designated button (on gpiote (!) peripheral) is used for resetting the DK board. Both could be removed from the example, I left them in just in case I need them. Similarly, I made a pinctrl dtsi file, through this can again be omitted for the blinky example since we are only setting up the uart0 in it. Board yaml was rewritten to align with custom board naming.

Lastly, in order to recognize the new custom board, it may be necessary to restart VsCode.

Just a note: I generally recommend making a custom board design for each project element since it is really easy to mess up an existing board and then realise that nothing works anymore. Alternatively, one can use overlays to prevent issues with the configuration, though I find this redundant in case we have a custom board that has already all physical parameters set.

## Conclusion
We have now our first Zephyr project (a modified blinky sample) running on a custom nrf52840 board. It is a very basic example where all project and board elements were intentionally kept as simple as possible.

The following projects will expand on this work and implement debugging, logging and interrupts, followed by communication busses, custom drivers controlled through Kconfig and finally, Bluetooth.

Stay tuned.

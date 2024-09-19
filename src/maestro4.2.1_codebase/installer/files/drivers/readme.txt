This folder contains the driver files for using the PCIe-6363 DAQ card under RTX64 control.

-- ni6363_rtx64.inf: This setup information file is based on the sample.inf template provided 
by IntervalZero and essentially maps the PCIe-6363 to the RTX64 generic plug-and-play driver, 
rtx64pnp.sys.

-- ni6363_rtx64.cat: This is the security catalog file created from ni6363_rtx64.inf using the
inf2cat.exe tool that comes with the Windows Driver Kit. It has NOT been digitally signed.

For Maestro's cxdriver.rtss process to communicate with the PCIe-6363, the device must be 
under RTX64 control. This is done in the Device Manager:
1) Find the PCIe-6363 device node, which should be under "Other Devices".
2) Right-click on the node and select "Update Driver Software", choose "Browse my computer
for driver software", use the browse controls to select this folder, and choose "Next". Windows
should then install the RTX64 PnP driver as the driver for the PCIe-6363. If successful, you
will find the device listed as "National Instruments PCIe-6363 (RTX64)" under the "RTX64 Drivers"
section in Device Manager.

However, for this process to work, you must temporarily disable driver signature enforcement in
Windows. To do so, consult the section "How to Install Maestro 4.x" in Maestro's online guide:

https://sites.google.com/a/srscicomp.com/maestro/installation/how-to-install-maestro4


--saruffner, 6/6/2018
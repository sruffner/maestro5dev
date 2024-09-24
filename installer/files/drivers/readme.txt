This folder contains the driver files for using the PCIe-6363 DAQ card under RTX64 control.

-- ni6363_rtx64.inf: This setup information file is based on the rtx64pnp.inf template provided 
with the RTX64 4.5 SDK from IntervalZero. It essentially maps the PCIe-6363 to the RTX64 generic 
plug-and-play driver, rtx64pnp.sys.

-- ni6363_rtx64.cat: This is the attestation-signed security catalog file created from ni6363_rtx64.inf.
IntervalZero support generated this file from the INF file.

For Maestro's cxdriver.rtss process to communicate with the PCIe-6363, the device must be 
under RTX64 control. This is done in the Device Manager:
1) Find the PCIe-6363 device node, which should be under "Other Devices".
2) Right-click on the node and select "Update Driver Software", choose "Browse my computer
for driver software", use the browse controls to select this folder, and choose "Next". Windows
should then install the RTX64 PnP driver as the driver for the PCIe-6363. If successful, you
will find the device listed as "National Instruments PCIe-6363 (RTX64)" under the "RTX64 Drivers"
section in Device Manager.

NOTE: For Maestro 4.x, the CAT file was unsigned, and you had to temporarily disable driver signature
enforcement in Windows. Now that IntervalZero generates the appropriately signed security catalog file
for us, that step is no longer necessary.


--saruffner, 9/24/2024
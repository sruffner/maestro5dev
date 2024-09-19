# Notes on building Maestro in Visual Studio

The latest **Maestro** release (v4.2.1) for the Windows 10/RTX64 platform was built in Visual Studio 2017 - Community
Edition. The RTX64 SDK from [IntervalZero](https://www.intervalzero.com/) is required in order to build the program. 
**Maestro** 4.2.1 was built using RTX64 version 3.4.

**Maestro** consists of two executable modules: `maestro.exe` is the **Maestro** user interface running in Windows, 
while `cxdriver.rtss` is the hardware controller, a separate process running in the RTX64 environment.

To assist others who would like to build Maestro -- most likely to update the program to run on Windows 11 and RTX64
version 4.5+ (since Windows 10 is approaching EOL) --, I've provided project templates for building these two
executables:
- `cxdriver/` : Visual Studio 2017 project template for cxdriver.rtss.
- `maestroGUI/` : Visual Studio 2017 project template for maestro.exe.

Keep in mind that these project templates were generated for my specific build setup. The VStudio "solution" containing
these two projects were located on a Windows 10 workstation, while the C/C++ source files and resource files were
located on a mapped network drive at "F:\projects\maestro.4.x". So these project templates will not work right out of
the box, but hopefully they will help in terms of configuring project dependencies and build settings.

**_Ultimately, though, I advise following the instructions in the RTX64 SDK to set up your Visual Studio projects to 
build both of these executables._**
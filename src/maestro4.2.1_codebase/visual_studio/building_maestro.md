# Notes on building Maestro 4.x in Visual Studio

The latest **Maestro** release (v4.2.1) for the Windows 10/RTX64 platform was built in Visual Studio 2017 - Community
Edition. The RTX64 SDK from [IntervalZero](https://www.intervalzero.com/) is required in order to build the program. 
**Maestro** 4.2.1 was built using RTX64 version 3.4.

**Maestro** consists of two executable modules: `maestro.exe` is the **Maestro** user interface running in Windows, 
while `cxdriver.rtss` is the hardware controller, a separate process running in the RTX64 environment.

I have begun the process of updating **Maestro** to run on Windows 11 and RTX64 4.5 (Windows 10 hits "end of support"
in Oct 2025). I do not plan to release any further versions of **Maestro** 4.x for Win10/RTX64 3.4. 

However, to assist others who would like to stick with Win10/RTX64 3.x and make further changes to **Maestro** 4.x, I've
provided the VS 2017 project files for building these two executables:
- `cxdriver/` : Visual Studio 2017 project files for cxdriver.rtss.
- `maestroGUI/` : Visual Studio 2017 project files for maestro.exe.

Keep in mind that these project files were generated for my specific build setup. The VStudio "solution" containing
the two projects were located on a Windows 10 workstation, while the C/C++ source files and resource files were
located on a mapped network drive at "F:\projects\maestro.4.x". So these project files will not work right out of
the box, but hopefully they will help in terms of configuring project dependencies and build settings. You can view and
edit the files in a simple text editor.

**_Ultimately, though, I advise following the instructions in the RTX64 SDK to set up your Visual Studio projects to 
build both of these executables._**

-- saruffner, 20sep2024
This folder contains a number of sample Maestro experiment documents that
have been used over the years to test new features as they were introduced,
to verify operation of a new release, and so on.

The file testing411.cxe is the primary experiment document I've used for
testing. The other testing*.cxe files are older versions that may be removed
in the future.

The two .jmx files were created by Maestro users. They are examples of the
alternative JSON experiment document format, which can be reviewed and edited
in a simple text editor (the .cxe files are a binary format). They're here 
primarily so we can verify future Maestro releases can still open them.

testJMXV110.jmx contains trial sets using the XYScope and another using
RMVideo. It was imported into Maestro 5.0, which removed all XYScope-related
stuff, then saved as assessRF.cxe.

testing411.cxe contains both XYScope and RMVideo targets and trial sets, as 
well as stimulus runs using XYScope targets ("XYseq" stimulus). It was
imported into Maestro 5.0, which removed all XYScope-related stuff, then
saved as testing500.cxe.


-- saruffner, 18oct2024
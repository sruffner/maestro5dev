Readme: READCXDATA/EDITCXDATA release dated 27mar2019

27mar2019
Scott Ruffner

Changes for this release:

Updated IAW data file format changes in Maestro 4.0.5, data file version = 22:

1) RMVideo's vertical refresh rate is now stored in the 32-bit integer field CXFILEHDR.d_framerate in micro-Hz 
(1.0e6 x rate in Hz) instead of milli-Hz. This preserves more significant digits when converting the frame rate
to a floating-point value in Hz, and from that calculating the refresh period as 1/rate.

In READCXDATA's output, the refresh rate is stored in out.key.d_framerate. In the past, this was a 32-bit integer
scalar set to the value in CXFILEHDR.d_framerate. To avoid breaking any analysis programs that depend on this field
being in milli-Hz, READCXDATA now makes out.key.d_framerate a floating-point scalar. For data file versions < 22,
it is set to the value in CXFILEHDR.d_framerate; for V>=22, it is set to CXFILEHDR.d_framerate/1000.0. Thus, the
units are still milli-Hz, and the extra precision available in V>=22 data files is preserved.

2) New field CXFILEHDR.rmvDupEvents[] contains information about any RMVideo duplicate frame events that occurred
during the trial. As of v4.0.5, an option may be selected so that Maestro allows up to 3 duplicate frames without
aborting the trial. In this scenario, this array contains up to 3 pairs of integers [N1, M1, N2, M2, N3, M3] 
defining up to 3 duplicate frame events. N is the frame index at which the event started, and M is the number of
consecutive duplicate frames comprising the event. If M = 0, then a single duplicate frame occurred at N because
RMVideo did not receive a frame update from Maestro in time. If M>0, then M duplicate frames occurred starting
at N because of a rendering delay in RMVideo. If N = 0, then no event is defined and M is meaningless.

3) If any duplicate frame events are listed in the rmvDupEvents[], then the new flag CXHF_DUPFRAME will be set in
the CXFHILEHDR.flags field.


For build procedural notes, see ../makeReleaseProcedures.txt.

# Build-confirmed QNX tee compatibility helper

Project-owned MU1440/QNX 6.5 ARMv7 logging helper implementing the subset used by the
deployment and Auto-Direct runtime: stdin -> stdout plus file outputs, with `-a` append
and `-i` ignore-SIGINT compatibility.

It replaces the historical dependency on M.I.B. `apps/sbin/tee` without redistributing
a QNX/OEM binary.

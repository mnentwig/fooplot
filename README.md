# fooplot
Heavy-duty no-nonsense 2d plotting tool. Does very little but stupidly fast. E.g. browse 4M points @ 60 FPS

# Status
Functional and sufficiently bug-free for "real" work but documentation is incomplete.

## Why?
Because existing plot tools don't perform with reasonably large amounts of data (7+ digit point count with O{N} perspective up to RAM limits)

## But gnuplot already does the job
Great. By all means, keep using it. fooplot may still 
* "nudge the envelope" wrt dataset size. Realized your "envelope" needs a "forklift"? We're here to help.
* offer a license-unencumbered option (feel free to contact me if even "MIT" causes issues)

## But Why plot this much data?
Because the eye is extremely good at catching patterns. 

Proper statistical methods will correlate a fly's heartbeat out of the roar of an exploding jet engine. Assuming I can observe long enough, the fly doesn't get nervous or suffer from cardiac arrythmia etc. 

But first, we need to spot the fly. Obviously.

Enter fooplot. 

## Example screenshot
<img src="www/screenshot1.png">

## Controls
Left mouse button: pan

Right mouse botton: Zoom area


Mouse wheel: Zoom (out/in)

CTRL-mouse wheel: Zoom (y axis only)

SHIFT-mouse wheel: Zoom (x axis only)

"a": Autoscale

CTRL-a: Autoscale (Y axis only)

SHIFT-A: Autoscale (X axis only)

Hint (typical use case): Use area zoom for the intended range on one axis, next autoscale the other axis to again see the full range

## Internals
* Markers larger than a single pixel are drawn by convolution (fixed-time algorithm in data size)
* partly multi-threaded
* using binary data does help quite a bit, too

# To be continued
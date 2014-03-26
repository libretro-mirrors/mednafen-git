<?php require("docgen.inc"); ?>

<?php BeginPage('psx', 'Sony PlayStation'); ?>

<?php BeginSection('Introduction', "", FALSE, FALSE, "Section_intro"); ?>
PlayStation 1 emulation is currently in a state of development.  Save states, rewinding, and netplay are not currently supported.
<p>
A dual-core Phenom II or Athlon II at 3GHz or higher, or rough equivalent(in terms of single-core IPC), is recommended for
running Mednafen's PlayStation 1 emulation on.  For better performance, the binary should be compiled for a 64-bit
target(for example, x86_64) rather than 32-bit, if available.<br>
</p>

<p>
Enabling CD image preloading into memory via the <a href="mednafen.html#cd.image_memcache">cd.image_memcache</a> setting is recommended, to
avoid short emulator pauses and audio pops due to waiting for disk accesses to complete when the emulated CD is accessed.
</p>

<p>
<a href="http://www.neillcorlett.com/psf/">PSF1</a> playback is supported.
</p>

<?php EndSection(); ?>


<?php BeginSection("Firmware/BIOS", "", FALSE, FALSE, "Section_firmware_bios"); ?>
<p>
Place the correct BIOS image files in the <a href="mednafen.html#Section_firmware_bios">correct location</a>.  <b>DO NOT</b> rename other revisions/regions of the BIOS to match the expected filenames, or you'll likely
cause emulation glitches.
</p>

<p>
The filenames listed below are per default psx.bios_* settings.
</p>
<table border>
 <tr><th>Filename:</th><th>Purpose:</th></tr>
 <tr><td>scph5500.bin</td><td>SCPH-5500 BIOS image.  Required for Japan-region games.</td></tr>
 <tr><td>scph5501.bin</td><td>SCPH-5501 BIOS image.  Required for North America/US-region games.  Reportedly the same as the SCPH-7003 BIOS image.</td></tr>
 <tr><td>scph5502.bin</td><td>SCPH-5502 BIOS image.  Required for Europe-region games.</td></tr>
</table>
<?php EndSection(); ?>


<?php BeginSection("Multitap Usage", "", FALSE, FALSE, "Section_multitap"); ?>
<p>
By default, no multitap is enabled.  Be aware that if you enable multitap on PSX port 1, game view mapping will be
inconsistent between games that support multitap and those that do not.
</p>

<p>
Enabling multitap on either PSX port <b>may</b> cause slight game slowdown.  Some 1-and-2-player-only games half-support the
multitap, but are apparently not coded with the multitap in mind, and <b>may</b> suffer from graphical glitches like screen tearing if multitap
is enabled when running them.
</p>

<table border="1">
<tr><th colspan="4">(Virtual) Port Index to Game View Mappings for Multitap only on PSX Port 1</th></tr>
<tr><th>Port Index:</th><th>Multitap-Compatible-Game:</th><th>Multitap-Incompatible-Game:</th><th>Physical Port Name:</th></tr>
<tr><td>1</td><td>1</td><td>1</td><td>1A</td></tr>
<tr><td>2</td><td>2</td><td>-</td><td>1B</td></tr>
<tr><td>3</td><td>3</td><td>-</td><td>1C</td></tr>
<tr><td>4</td><td>4</td><td>-</td><td>1D</td></tr>
<tr><td>5</td><td>5</td><td>2</td><td>2</td></tr>
<tr><td>6</td><td>-</td><td>-</td><td>-</td></tr>
<tr><td>7</td><td>-</td><td>-</td><td>-</td></tr>
<tr><td>8</td><td>-</td><td>-</td><td>-</td></tr>
</table>

<br>

<table border="1">
<tr><th colspan="4">(Virtual) Port Index to Game View Mappings for Multitap only on PSX Port 2</th></tr>
<tr><th>Port Index:</th><th>Multitap-Compatible-Game:</th><th>Multitap-Incompatible-Game:</th><th>Physical Port Name:</th></tr>
<tr><td>1</td><td>1</td><td>1</td><td>1</td></tr>
<tr><td>2</td><td>2</td><td>2</td><td>2A</td></tr>
<tr><td>3</td><td>3</td><td>-</td><td>2B</td></tr>
<tr><td>4</td><td>4</td><td>-</td><td>2C</td></tr>
<tr><td>5</td><td>5</td><td>-</td><td>2D</td></tr>
<tr><td>6</td><td>-</td><td>-</td><td>-</td></tr>
<tr><td>7</td><td>-</td><td>-</td><td>-</td></tr>
<tr><td>8</td><td>-</td><td>-</td><td>-</td></tr>
</table>
<br>
<table border="1">
<tr><th colspan="4">(Virtual) Port Index to Game View Mappings for Multitap on both PSX Ports</th></tr>
<tr><th>Port Index:</th><th>Multitap-Compatible-Game:</th><th>Multitap-Incompatible-Game:</th><th>Physical Port Name:</th></tr>
<tr><td>1</td><td>1</td><td>1</td><td>1A</td></tr>
<tr><td>2</td><td>2</td><td>-</td><td>1B</td></tr>
<tr><td>3</td><td>3</td><td>-</td><td>1C</td></tr>
<tr><td>4</td><td>4</td><td>-</td><td>1D</td></tr>
<tr><td>5</td><td>5</td><td>2</td><td>2A</td></tr>
<tr><td>6</td><td>6</td><td>-</td><td>2B</td></tr>
<tr><td>7</td><td>7</td><td>-</td><td>2C</td></tr>
<tr><td>8</td><td>8</td><td>-</td><td>2D</td></tr>
</table>


</p>
<?php EndSection(); ?>


<?php PrintSettings(); ?>

<?php EndPage(); ?>


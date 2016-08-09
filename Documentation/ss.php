<?php require("docgen.inc"); ?>

<?php BeginPage('ss', 'Sega Saturn'); ?>

<?php BeginSection('Introduction', "", FALSE, FALSE, "Section_intro"); ?>
<p>
<font color="orange" size="+1"><b>NOTE:</b></font> The Sega Saturn emulation is currently experimental, and under active development, and save states are not currently supported.  By default(and for the official releases for Windows), Saturn emulation is only compiled in for <b>x86_64</b> builds.  The separate <a href="ssfplay.html">SSF playback module</a> does not have this limitation.
</p>
<p>
Mednafen's Sega Saturn emulation is extremely CPU intensive.  The minimum recommended CPU is a quad-core Intel Haswell-microarchitecture CPU with
a base frequency of >= 3.3GHz and a turbo frequency of >= 3.7GHz(e.g. Xeon E3-1226 v3).
</p>

<p>
Enabling CD image preloading into memory via the <a href="mednafen.html#cd.image_memcache">cd.image_memcache</a> setting is recommended, to
avoid short emulator pauses and audio pops due to waiting for disk accesses to complete when the emulated CD is accessed.
</p>
<?php EndSection(); ?>

<?php BeginSection("Firmware/BIOS", "", FALSE, FALSE, "Section_firmware_bios"); ?>
<p>
Place the correct BIOS image files in the <a href="mednafen.html#Section_firmware_bios">correct location</a>.
</p>

<p>
The filenames listed below are per default ss.bios_* settings.
</p>
<table border>
 <tr><th>Filename:</th><th>Purpose:</th><th>SHA-256 Hash:</tr>
 <tr><td>sega_101.bin</td><td>BIOS image.<br>Required for Japan-region games.</td><td>dcfef4b99605f872b6c3b6d05c045385cdea3d1b702906a0ed930df7bcb7deac</td></tr>
 <tr><td>mpr-17933.bin</td><td>BIOS image.<br>Required for North America/US-region and Europe-region games.</td><td>96e106f740ab448cf89f0dd49dfbac7fe5391cb6bd6e14ad5e3061c13330266f</td></tr>
</table>
<?php EndSection(); ?>

<?php BeginSection('Default Input Mappings'); ?>

 <?php BeginSection('Digital Gamepad on Virtual Port 1'); ?>
  <p>
  <table border>
   <tr><th>Key:</th><th nowrap>Emulated Button:</th></tr>

   <tr><td>W</td><td>Up</td></tr>
   <tr><td>S</td><td>Down</td></tr>
   <tr><td>A</td><td>Left</td></tr>
   <tr><td>D</td><td>Right</td></tr>

   <tr><td>Enter</td><td>START</td></tr>

   <tr><td>Keypad 1</td><td>A</td></tr>
   <tr><td>Keypad 2</td><td>B</td></tr>
   <tr><td>Keypad 3</td><td>C</td></tr>

   <tr><td>Keypad 4</td><td>X</td></tr>
   <tr><td>Keypad 5</td><td>Y</td></tr>
   <tr><td>Keypad 6</td><td>Z</td></tr>

   <tr><td>Keypad 7</td><td>Left Shoulder</td></tr>
   <tr><td>Keypad 9</td><td>Right Shoulder</td></tr>
  </table>
  </p>
 <?php EndSection(); ?>

<?php EndSection(); ?>


<?php PrintSettings(); ?>

<?php EndPage(); ?>

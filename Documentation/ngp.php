<?php require("docgen.inc"); ?>

<?php BeginPage('ngp', 'Neo Geo Pocket (Color)'); ?>

Mednafen's Neo Geo Pocket emulation is based off of <a href="http://neopop.emuxhaven.net/">NeoPop</a>.

<?php BeginSection('Default Key Assignments'); ?>
 <table border>
  <tr><th>Key(s):</th><th>Action:</th><th>Configuration String:</th></tr>
  <tr><td>ALT + SHIFT + 1</td><td>Activate in-game input configuration process for Neo Geo Pocket pad.</td><td>input_config1</td></tr>
 </table>
 </p>
 <p>
 <table border>
  <tr><th>Key:</th><th nowrap>Action/Button:</th></tr>
  <tr><td>Keypad 2</td><td>A</td></tr>
  <tr><td>Keypad 3</td><td>B</td></tr>
  <tr><td>Enter/Return</td><td>Option</td></tr>
  <tr><td>W</td><td>Up</td></tr>
  <tr><td>S</td><td>Down</td></tr>
  <tr><td>A</td><td>Left</td></tr>
  <tr><td>D</td><td>Right</td></tr>
 </table>
<?php EndSection(); ?>


<?php BeginSection('Game-specific Emulation Hacks'); ?>
 <table border width="100%">
  <tr><th>Title:</th><th>Description:</th><th>Source code files affected:</th></tr>
  <tr><td>Metal Slug - 2nd Mission</td><td>Patch to work around lack of one of the side effects of a real BIOS(to be fixed in a future version of Mednafen).</td><td>src/ngp/rom.cpp</td></tr>
  <tr><td>Puyo Pop</td><td></td><td>src/ngp/rom.cpp</td></tr>
  </table>
<?php EndSection(); ?>

<?php PrintSettings(); ?>

<?php EndPage(); ?>


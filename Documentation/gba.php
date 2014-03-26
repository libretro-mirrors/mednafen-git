<?php require("docgen.inc"); ?>

<?php BeginPage('gba', 'Game Boy Advance'); ?>

<?php BeginSection('Introduction'); ?>

Mednafen's GameBoy Advance emulation is based off of <a href="http://vba.sf.net/">VisualBoy Advance</a>.

<?php BeginSection('BIOS'); ?>
Built-in high-level BIOS emulation is used by default; however, a real BIOS can be used by setting the <a href="#gba.bios">gba.bios</a> setting.
<?php EndSection(); ?>

<?php BeginSection('Custom Palettes'); ?>
<p>
Custom palettes should contain 32768 8-bit-per-color-component RGB triplets.
</p>
<?php EndSection(); ?>

<?php BeginSection('Backup Memory Type'); ?>
   <p>
   To specify the backup memory type on a per-game basis, create a file with the same name as the ROM image but with the extension replaced with "type",
   in the "sav" directory under the Mednafen base directory.
   </p>
   
   <p>
     Example: SexyPlumbers.gba -> SexyPlumbers.type
   </p>

   <p>One or more of the following strings(on separate lines) should appear in this file:
    <ul>
        <li>sram
        <li>flash
        <li>eeprom
        <li>sensor
	<li>rtc
    </ul>
   </p>
   <p>Additionally, the flash size can be specified by specifying the size(real size, or divided by 1024) after the type, like "flash 128" or "flash 131072".</p>
    </blockquote>
<?php EndSection(); ?>

<?php EndSection(); ?>

<?php BeginSection('Default Key Assignments'); ?>
 <table border>
  <tr><th>Key(s):</th><th>Action:</th><th>Configuration String:</th></tr>
  <tr><td>ALT + SHIFT + 1</td><td>Activate in-game input configuration process for GameBoy Advance pad.</td><td>input_config1</td></tr>
 </table>
 </p>
 <p>
 <table border>
  <tr><th>Key:</th><th nowrap>Action/Button:</th></tr>
  <tr><td>Keypad 2</td><td>B</td></tr>
  <tr><td>Keypad 3</td><td>A</td></tr>
  <tr><td>Enter/Return</td><td>Start</td></tr>
  <tr><td>Tab</td><td>Select</td></tr>
  <tr><td>Keypad 5</td><td>Shoulder Left</td></tr>
  <tr><td>Keypad 6</td><td>Shoulder Right</td></tr>
  <tr><td>W</td><td>Up</td></tr>
  <tr><td>S</td><td>Down</td></tr>
  <tr><td>A</td><td>Left</td></tr>
  <tr><td>D</td><td>Right</td></tr>
 </table>
<?php EndSection(); ?>


<?php PrintSettings(); ?>

<?php EndPage(); ?>


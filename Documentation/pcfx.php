<?php require("docgen.inc"); ?>

<?php BeginPage('pcfx', 'PC-FX'); ?>

<?php BeginSection('Introduction'); ?>
  <ul>
   <li>Internal backup memory and external backup memory are emulated.</li>
   <li>Motion decoder RLE and JPEG-like modes are emulated.</li>
   <li>KING Background 0 scaling+rotation mode is supported.</li>
   <li>Mouse emulation.</li>
  </ul>
<?php EndSection(); ?>


<?php BeginSection('Default Key Assignments'); ?>

 <table border>
  <tr><th>Key(s):</th><th>Action:</th><th>Configuration String:</th></tr>
  <tr><td>ALT + SHIFT + 1</td><td>Activate in-game input configuration process for device on virtual inport port 1.</td><td>input_config1</td></tr>
  <tr><td>ALT + SHIFT + 2</td><td>Activate in-game input configuration process for device on virtual inport port 2.</td><td>input_config2</td></tr>
 </table>
 </p>
 <p>
 <table border>
  <tr><th>Key:</th><th nowrap>Action/Button:</th></tr>
  <tr><td>Keypad 4</td><td>IV</td></tr>
  <tr><td>Keypad 5</td><td>V</td></tr>
  <tr><td>Keypad 6</td><td>VI</td></tr>
  <tr><td>Keypad 1</td><td>III</td></tr>
  <tr><td>Keypad 2</td><td>II</td></tr>
  <tr><td>Keypad 3</td><td>I</td></tr>
  <tr><td>Keypad 8</td><td>MODE 1</td></tr>
  <tr><td>Keypad 9</td><td>MODE 2</td></tr>
  <tr><td>Enter/Return</td><td>Run</td></tr>
  <tr><td>Tab</td><td>Select</td></tr>
  <tr><td>W</td><td>Up</td></tr>
  <tr><td>S</td><td>Down</td></tr>
  <tr><td>A</td><td>Left</td></tr>
  <tr><td>D</td><td>Right</td></tr>
 </table>

<?php EndSection(); ?>

<?php PrintSettings(); ?>

<?php EndPage(); ?>


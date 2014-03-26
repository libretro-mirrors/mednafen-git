<?php require("docgen.inc"); ?>

<?php BeginPage('psx', 'Sony PlayStation'); ?>

<?php BeginSection('Introduction'); ?>
PlayStation 1 emulation is currently in a buggy alpha state and not all hardware and features are emulated.
<p>
A dual-core Phenom II or Athlon II at 3GHz or higher, or rough equivalent(in terms of single-core IPC), is recommended for
running Mednafen's PlayStation 1 emulation on.  For better performance, the binary should be compiled for a 64-bit
target(for example, x86_64) rather than 32-bit, if available.<br>
</p>

<p>
PSF1 playback is supported.
</p>

<?php EndSection(); ?>

<?php BeginSection("Multitap Usage"); ?>
<p>
By default, no multitap is enabled.  Be aware that if you enable multitap on PSX port 1, game view mapping will be
inconsistent between games that support multitap and those that do not.
</p>

<table border="1">
<tr><th colspan="3">Port Index to Emulated Port Mappings</th></tr>
<tr><th>Port Index:</th><th>Emulated PSX Port:</th><th>Emulated Multitap Port:</th></tr>
<tr><td>1</td><td>1</td><td>1A</td></tr>
<tr><td>2</td><td>2</td><td>2A</td></tr>
<tr><td>3</td><td>-</td><td>1B</td></tr>
<tr><td>4</td><td>-</td><td>1C</td></tr>
<tr><td>5</td><td>-</td><td>1D</td></tr>
<tr><td>6</td><td>-</td><td>2B</td></tr>
<tr><td>7</td><td>-</td><td>2C</td></tr>
<tr><td>8</td><td>-</td><td>2D</td></tr>
</table>
<br>

<table border="1">
<tr><th colspan="3">Port Index to Game View Mappings for Multitap only on PSX Port 1</th></tr>
<tr><th>Port Index:</th><th>Multitap-Compatible-Game:</th><th>Multitap-Incompatible-Game:</th></tr>
<tr><td>1</td><td>1</td><td>1</td></tr>
<tr><td>2</td><td>5</td><td>2</td></tr>
<tr><td>3</td><td>2</td><td>-</td></tr>
<tr><td>4</td><td>3</td><td>-</td></tr>
<tr><td>5</td><td>4</td><td>-</td></tr>
<tr><td>6</td><td>-</td><td>-</td></tr>
<tr><td>7</td><td>-</td><td>-</td></tr>
<tr><td>8</td><td>-</td><td>-</td></tr>
</table>

<br>

<table border="1">
<tr><th colspan="3">Port Index to Game View Mappings for Multitap only on PSX Port 2</th></tr>
<tr><th>Port Index:</th><th>Multitap-Compatible-Game:</th><th>Multitap-Incompatible-Game:</th></tr>
<tr><td>1</td><td>1</td><td>1</td></tr>
<tr><td>2</td><td>2</td><td>2</td></tr>
<tr><td>3</td><td>-</td><td>-</td></tr>
<tr><td>4</td><td>-</td><td>-</td></tr>
<tr><td>5</td><td>-</td><td>-</td></tr>
<tr><td>6</td><td>3</td><td>-</td></tr>
<tr><td>7</td><td>4</td><td>-</td></tr>
<tr><td>8</td><td>5</td><td>-</td></tr>
</table>
<br>
<table border="1">
<tr><th colspan="3">Port Index to Game View Mappings for Multitap on both PSX Ports</th></tr>
<tr><th>Port Index:</th><th>Multitap-Compatible-Game:</th><th>Multitap-Incompatible-Game:</th></tr>
<tr><td>1</td><td>1</td><td>1</td></tr>
<tr><td>2</td><td>5</td><td>2</td></tr>
<tr><td>3</td><td>2</td><td>-</td></tr>
<tr><td>4</td><td>3</td><td>-</td></tr>
<tr><td>5</td><td>4</td><td>-</td></tr>
<tr><td>6</td><td>6</td><td>-</td></tr>
<tr><td>7</td><td>7</td><td>-</td></tr>
<tr><td>8</td><td>8</td><td>-</td></tr>
</table>


</p>
<?php EndSection(); ?>


<?php PrintSettings(); ?>

<?php EndPage(); ?>


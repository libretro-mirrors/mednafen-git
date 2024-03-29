<?php

$mednafen_version = "";
$mod_list = array();

$sections = array();

$section_aname_usage = array();

$curmod = "";
$cursection = "";

$ps_string = "";
$toc_string = "";
$settings_g = array();
$section_depth = 0;
$section_endhtml = '';

function GetMTime()
{
 $sf_mt = filemtime($_SERVER["SCRIPT_FILENAME"]);
 $sd_mt = filemtime("settings.def");
 $dg_mt = filemtime("docgen.inc");

 $mt = $sf_mt;

 if($sd_mt > $mt)
  $mt = $sd_mt;

 if($dg_mt > $mt)
  $mt = $dg_mt;

 return($mt);
}

function GameDBEntriesSortCompare($a, $b)
{
 if($a['name'] == $b['name'])
  return strcmp($a['id'], $b['id']);

 return strcmp($a['name'], $b['name']);
}

function ReadModules()
{
 global $mod_list, $mednafen_version;

 $fp = fopen("modules.def", "rb");

 $mednafen_version = trim(fgets($fp));

 while(!feof($fp))
 {
  $mod = array();

  $mod['name'] = trim(fgets($fp));
  if($mod['name'] == "")
   break;

  $mod['fullname'] = trim(fgets($fp));
  $mod['nominal_width'] = (int)fgets($fp);
  $mod['nominal_height'] = (int)fgets($fp);  

  $mod['cpinfo'] = array();
  $cpicount = (int)trim(fgets($fp));
  while($cpicount--)
  {
   $gfn = trim(fgets($fp));
   $mod['cpinfo'][$gfn] = array();
   $mod['cpinfo'][$gfn]['gfn'] = $gfn;
   $mod['cpinfo'][$gfn]['description'] = trim(fgets($fp));
   $mod['cpinfo'][$gfn]['valid_entry_count'] = explode(' ', trim(fgets($fp)));
  }

  asort($mod['cpinfo']);
  //
  //
  //
  $mod['dbinfo'] = array();
  $dbicount = (int)trim(fgets($fp));

  for($dbi = 0; $dbi < $dbicount; $dbi++)
  {
   $mod['dbinfo'][$dbi] = array();
   $mod['dbinfo'][$dbi]['shortname'] = stripcslashes(trim(fgets($fp)));
   $mod['dbinfo'][$dbi]['fullname'] = stripcslashes(trim(fgets($fp)));
   $mod['dbinfo'][$dbi]['description'] = stripcslashes(trim(fgets($fp)));
   $mod['dbinfo'][$dbi]['entries'] = array();
 
   $eicount = (int)trim(fgets($fp));
   for($ei = 0; $ei < $eicount; $ei++)
   {
    $mod['dbinfo'][$dbi]['entries'][$ei] = array();
    $mod['dbinfo'][$dbi]['entries'][$ei]['name'] = stripcslashes(trim(fgets($fp)));;
    $mod['dbinfo'][$dbi]['entries'][$ei]['id'] = stripcslashes(trim(fgets($fp)));
    $mod['dbinfo'][$dbi]['entries'][$ei]['idishash'] = (int)trim(fgets($fp));
    $mod['dbinfo'][$dbi]['entries'][$ei]['setting'] = stripcslashes(trim(fgets($fp)));
    $mod['dbinfo'][$dbi]['entries'][$ei]['purpose'] = stripcslashes(trim(fgets($fp)));
   }

   usort($mod['dbinfo'][$dbi]['entries'], "GameDBEntriesSortCompare");
  }
  //
  //
  //
  $mod_list[$mod['name']] = $mod;
 }

 fclose($fp);
}

function ReadSettings()
{
 global $settings_g;

 $settings_g = array();

 $fp = fopen("settings.def", "rb");

 while(!feof($fp))
 {
  $setting = array();

  $setting['name'] = trim(fgets($fp));
  $setting['flags'] = trim(fgets($fp));
  $setting['description'] = stripcslashes(trim(fgets($fp)));
  $setting['description_extra'] = stripcslashes(trim(fgets($fp)));
  $setting['type'] = trim(fgets($fp));
  $setting['default_value'] = trim(fgets($fp));
  $setting['minimum'] = trim(fgets($fp));
  $setting['maximum'] = trim(fgets($fp));

  $enum_count = (int)fgets($fp);
  $enum_list = array();

  while($enum_count--)
  {
   $ele = array();

   $ele['string'] = trim(fgets($fp));
   $ele['description'] = stripcslashes(trim(fgets($fp)));
   $ele['description_extra'] = stripcslashes(trim(fgets($fp)));

   array_push($enum_list, $ele);
  }

  $setting['enum_list'] = $enum_list;
  //
  //
  $alias_count = (int)fgets($fp);
  $alias_list = array();

  while($alias_count--)
   array_push($alias_list, trim(fgets($fp)));

  $setting['aliases'] = $alias_list;
  //
  //
  if($setting['name'] != "")
   $settings_g[$setting['name']] = $setting;
  //array_push($settings_g, $setting);
 }


 fclose($fp);
}


function BeginPage($module_name, $module_description)
{
 global $mednafen_version, $curmod, $cursection, $ps_string, $toc_string, $section_depth;

 ReadSettings();
 ReadModules();

 $curmod = $module_name;

 $section_depth++;

 $ps_string = '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
 <head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
  <title>Mednafen ' . htmlspecialchars($module_description) . ' Documentation</title>
  <link rel="stylesheet" type="text/css" media="screen, projection" href="mednafen.css">
 </head>
 <body>
  <center><h1><img alt="Mednafen" src="mednafen.png" width="696" height="204"></h1></center>
  <center><h1>' . htmlspecialchars($module_description) . ' Documentation</h1></center>
  <center><i>Last updated ' . strftime("%B %e, %Y", GetMTime()) . '<br>Valid as of ' . htmlspecialchars($mednafen_version) . '</i></center>';


 $toc_string = '
 <p></p>
 <b>Table of Contents:</b>
 <ul>';

 ob_start();
}


function EndPage()
{
 global $mednafen_version, $curmod, $cursection, $ps_string, $toc_string, $section_depth;

 $toc_string .= '</ul><hr width="100%">';

 $end_string = '';

 $end_string .= ' </body>
</html>';

 $section_depth--;
 $section_string = ob_get_contents();
 ob_end_clean();

 if($section_depth != 0)
 {
  fprintf(STDERR, "Mismatched BeginSection()/EndSection(): %d\n", $section_depth);
  exit(1);
 }


 echo($ps_string);
 echo($toc_string);

 echo($section_string);
 echo($end_string);

 $section_depth--;
}

function ExternalSection($name, $link, $nonlink_text = "")
{
 global $mednafen_version, $curmod, $cursection, $ps_string, $toc_string, $section_depth, $sections, $section_endhtml;

 $toc_string .= '<li><a href="' . urlencode($link) . '">' . htmlspecialchars($name) . '</a>' . htmlspecialchars($nonlink_text);
}

function BeginSection($name, $aname, $header_link = FALSE)
{
 global $mednafen_version, $curmod, $cursection, $ps_string, $toc_string, $section_depth, $sections, $section_endhtml;
 global $section_aname_usage;


 if(isset($section_aname_usage[$aname]))
 {
  fprintf(STDERR, "Section aname \"" . $aname . "\" already used!");
  exit(1);
 }
 $section_aname_usage[$aname] = true;

 array_push($sections, $name);
 $section_depth++;

 if($aname === FALSE)
  $toc_string .= '<li>' . htmlspecialchars($name) . '<ul>';
 else
 {
  $toc_string .= '<li><a href="#' . urlencode($aname) . '">' . htmlspecialchars($name) . '</a>' . '<ul>';

  echo('<h' . $section_depth . '>');
  echo('<a name="' . urlencode($aname) . '">');

  if($header_link !== FALSE)
   echo('<a href="' . $header_link . '">');

  echo(htmlspecialchars($name));

  if($header_link !== FALSE)
   echo('</a>');

   echo('</a>');

  echo('</h' . $section_depth . '><p></p>');

  $section_endhtml = '<hr width="' . (100 - ($section_depth - 2) * 25) . '%">';
 }
}


function EndSection()
{
 global $mednafen_version, $curmod, $cursection, $ps_string, $toc_string, $section_depth, $sections, $section_endhtml;

 $toc_string .= '</ul></li>';

 echo($section_endhtml);

 array_pop($sections);
 $section_depth--;
}


function spis($haystack, $needle)
{
 if(strpos($haystack, $needle) === FALSE)
  return(0);
 return(1);
}

function ProcessDesc($desc, $mod = "")
{
 global $mednafen_version, $curmod, $cursection, $ps_string, $toc_string, $section_string, $settings_g, $mod_list;

 $nde = "";
 $sname = "";
 $in_sname = false;

 for($i = 0; $i < strlen($desc); $i++)
 {
  $c = $desc[$i];

  if($in_sname)
  {
   if($c == "\"")
   {
    if($curmod == "" && $mod != "")
    {
     $nde .= "<a href=\"" . $mod . ".html#" . htmlspecialchars($sname) . "\">" . htmlspecialchars($sname) . "</a>";
    }
    else if($curmod == "" || !strncmp($sname, $curmod . '.', strlen($curmod) + 1))
    {
     $nde .= "<a href=\"#" . htmlspecialchars($sname) . "\">" . htmlspecialchars($sname) . "</a>";
    }
    else if(!strncmp($sname, "<system>", strlen("<system>")))
    {
     $sname = $curmod . substr($sname, strlen("<system>"));
     $nde .= "<a href=\"#" . htmlspecialchars($sname) . "\">" . htmlspecialchars($sname) . "</a>";
    }
    else
     $nde .= "<a href=\"mednafen.html#" . htmlspecialchars($sname) . "\">" . htmlspecialchars($sname) . "</a>";
    //
    $nde .= $c;
    $in_sname = false;
   }
   else
    $sname .= $c;
  }
  else if($c == "\x05")
  {
   $in_sname = true;
   $sname = "";
  }
  else if($c == "<")
   $nde .= "&lt;";
  else if($c == ">")
   $nde .= "&gt;";
  else if($c == "&")
   $nde .= "&amp;";
  else if($c == "\n")
   $nde .= "<br>\n";
  else
   $nde .= $c;
 }

 $de_search = array("fname_format.txt", "Caution:", "WARNING:" );
 $de_replace = array('<a href="fname_format.txt">fname_format.txt</a>', '<font color="yellow"><b>Caution:</b></font>', '<font color="red"><b>WARNING:</b></font>');

 $nde = str_replace($de_search, $de_replace, $nde);

 return $nde;
}

function PrintCustomPalettes()
{
 global $mednafen_version, $curmod, $cursection, $ps_string, $toc_string, $section_string, $settings_g, $mod_list;

 if($curmod != "")
  BeginSection("Custom Palettes", "Section_custom_palettes");

 echo('<p></p><table border><tr class="TableHeader">');

 if($curmod == "")
  echo('<th>System Module:</th>');

 echo('<th>Global Filename:</th>');
 echo('<th>Description:</th>');
 echo('</tr>');
 echo("\n");

 $mls = $mod_list;
 asort($mls);

 foreach($mls as $mod=>$mod_def)
 {
  if($mod != $curmod && $curmod != "")
   continue;

  $cpicounter = 0;
  foreach($mod_def['cpinfo'] as $gfn=>$cpdef)
  {
   echo('<tr>');

   if($curmod == "" && $cpicounter == 0)
    echo('<td align="center" rowspan="' . sizeof($mod_def['cpinfo']) . '"><b>' . htmlspecialchars($mod) . '</b></td>');

   echo('<td>' . htmlspecialchars($cpdef['gfn']) . '</td>');
   echo('<td>');

   echo(ProcessDesc($cpdef['description'], $mod) . '<br><br>');

   for($i = 0; $i < sizeof($cpdef['valid_entry_count']); $i++)
   {
    if($i)
     echo(' <i>or</i> ');
    echo($cpdef['valid_entry_count'][$i]);
   }
   echo(' RGB triplets');
   echo('</td>');
   // valid_entry_count
   echo('</tr>');
   echo("\n");
   $cpicounter++;
  }
 }

 echo('</table>');

 if($curmod != "")
  EndSection();
}

function PrintInternalDatabases()
{
 global $mednafen_version, $curmod, $cursection, $ps_string, $toc_string, $section_string, $settings_g, $mod_list;

 BeginSection("Internal Databases", "Section_internal_databases");

 foreach($mod_list as $mod=>$mod_def)
 {
  if($mod != $curmod)
   continue;

  foreach($mod_def['dbinfo'] as $db)
  {
   BeginSection($db['fullname'], "Section_internal_databases_" . $db['shortname']);

   echo(ProcessDesc($db['description']));

   echo('<p></p><table border><tr class="TableHeader">');
   echo('<th>Game:</th>');
   echo('<th width="1%">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;ID:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</th>');
   echo('<th>' . htmlspecialchars($db['fullname']) . ':' . '</th>');
   echo('<th>Purpose:</th>');
   echo('</tr>');
   echo("\n");

   foreach($db['entries'] as $gdbe)
   {
    echo('<tr>');
    echo('<td>' . htmlspecialchars($gdbe['name']) . '</td>');
    if($gdbe['idishash'])
     echo('<td><font size="-1"><i>' . htmlspecialchars($gdbe['id']) . '</i></font></td>');
    else
     echo('<td>' . str_replace("\n", "<br>", htmlspecialchars($gdbe['id'])) . '</td>');
    echo('<td>' . htmlspecialchars($gdbe['setting']) . '</td>');
    echo('<td>' . htmlspecialchars($gdbe['purpose']) . '</td>');
    echo('</tr>');
    echo("\n");
   }
   echo('</table>');

   EndSection();
  }
 }

 EndSection();
}

function PrintSettings($section_name = "Settings Reference")
{
 global $mednafen_version, $curmod, $cursection, $ps_string, $toc_string, $section_string, $settings_g, $mod_list;

 $magic_settings = array();
 $magic_settings_temp = array();
 $magic_moo = array();

 BeginSection($section_name, $section_name);
?>
<p>
<b>Caution:</b> Any settings that change emulation behavior as seen from the emulated game are shown with the setting name bolded in the table. One should be conscious of these settings if one has changed them from the default values when using network play or movies, as they can cause desychronizations if not set the same among hosts(or between the recording emulator and the playing emulator). 
</p>
<?php
 $value_types = array();
 $value_types['MDFNST_INT'] = "integer";
 $value_types['MDFNST_UINT'] = "integer";
 $value_types['MDFNST_BOOL'] = "boolean";
 $value_types['MDFNST_FLOAT'] = "real";
 $value_types['MDFNST_STRING'] = "string";
 $value_types['MDFNST_ENUM'] = "enum";
 $value_types['MDFNST_MULTI_ENUM'] = "multi-enum";

for($display_ct = 0; $display_ct < 2; $display_ct++)
{
 echo('<p></p><table border><tr class="TableHeader"><th>Setting:</th><th>Value Type:</th><th>Possible Values:</th><th>Default Value:</th><th>Description:</th></tr>');

 $settings = $settings_g;

 if($curmod == "" && $display_ct == 1)
  $settings = $magic_settings;

 $row_ff = 0;
 foreach($settings as $setting)
 {
  $name = $setting['name'];
  $type = $setting['type'];
  $flags = $setting['flags'];

  if(strpos($flags, "MDFNSF_SUPPRESS_DOC") !== FALSE)
   continue;

  if($curmod == "")
  {
   $should_continue = 0;
   foreach($mod_list as $mod=>$mod_def)
   {
    if(!strncmp($setting['name'], $mod . '.', strlen($mod) + 1))
    {
     $new_setting = $setting;

     $new_name = "<system>." . substr($setting['name'], strlen($mod) + 1);
     $new_setting['name'] = $new_name;

     if(isset($magic_settings_temp[$new_name]))
     {
      $m = &$magic_settings_temp[$new_name];

      if($new_setting['type'] != $m['type'])
       $m['type'] = "<i>(variable)</i>";

      if($new_setting['default_value'] != $m['default_value'])
       $m['default_value'] = "<i>(variable)</i>";

      if($new_setting['minimum'] != $m['minimum'])
       $m['minimum'] = "<i>(variable)</i>";

      if($new_setting['maximum'] != $m['maximum'])
       $m['maximum'] = "<i>(variable)</i>";

      $magic_settings[$new_name] = $m;
     }
     else
      $magic_settings_temp[$new_name] = $new_setting;

     if(!isset($magic_moo[$new_name]))
      $magic_moo[$new_name] = array();

     array_push($magic_moo[$new_name], $mod);


     $should_continue = 1;
     break;
    }

   }
   if($should_continue)
    continue;
  }
  else
  {
   if(strncmp($setting['name'], $curmod . '.', strlen($curmod) + 1))
    continue;
  }

  if($display_ct != spis($flags, "MDFNSF_COMMON_TEMPLATE"))
   continue;

  $magiccowpower = "";

  if(isset($magic_moo[$name]) && sizeof($magic_moo[$name]) != sizeof($mod_list))
  {
   $magiccowpower = "<br><ul>";
   foreach($magic_moo[$name] as $moomod)
   {
    $magiccowpower .= "<li>" . $moomod . "</li>";
   }
   $magiccowpower .= "</ul>";
  }
  //
  //
  //
  $name = htmlspecialchars($name, ENT_NOQUOTES);
  $description = ProcessDesc($setting['description']);
  $description_extra = ProcessDesc($setting['description_extra']);

  printf("<tr class=\"%s\"><td class=\"ColA\">%s%s%s%s</td><td class=\"ColB\">%s</td><td class=\"ColC\">", 
	($row_ff ? "RowB" : "RowA"), (spis($flags, "MDFNSF_EMU_STATE") ? "<b>" : ""), $name,
                                        (spis($flags, "MDFNSF_EMU_STATE") ? "</b>" : ""), $magiccowpower,
                                        $value_types[$type]);



  switch($type)
  {
   case 'MDFNST_INT':
   case 'MDFNST_UINT':
   case 'MDFNST_FLOAT': printf("%s <i>through</i> %s", $setting['minimum'], $setting['maximum']);
                      break;

   case 'MDFNST_BOOL': printf("0<br>1");
                     break;

   case 'MDFNST_MULTI_ENUM':
   case 'MDFNST_ENUM':
                    {
                     $enum_list = $setting['enum_list'];
		     $pending_br = '';

		     foreach($enum_list as $ele)
                     {
                      if($ele['description'] == '')
                       continue;

		      echo($pending_br);
                      printf("%s", $ele['string']);
		      $pending_br = '<br>';
                     }
                    }
                    break;

   case 'MDFNST_STRING': printf("&nbsp;");
                       break;
  }

  $default_value = $setting['default_value'];

  printf("</td><td class=\"ColD\">%s</td><td class=\"ColE\"><a name=\"%s\">%s</a>", $default_value,
                                        $name, $description);

  foreach($setting['aliases'] as $alias)
   printf("<a name=\"%s\"></a>", $alias);

  if($description_extra != "")
  {
   printf("<p>%s</p>", $description_extra);
  }

  if($type == 'MDFNST_ENUM' || $type == 'MDFNST_MULTI_ENUM')
  {
   printf("<ul>");

   $enum_list = $setting['enum_list'];
   $pending_br = '';
   foreach($enum_list as $ele)
   {
    if($ele['description'] == '')
     continue;

    echo($pending_br);
    printf("<li><b>%s</b> - %s<br>%s</li>", $ele['string'], ProcessDesc($ele['description']), ProcessDesc($ele['description_extra']));

    $pending_br = '<br>';
   }

   printf("</ul>");
  }


  printf("</td></tr>");

  printf("<tr><td class=\"RowSpacer\" colspan=\"5\">&nbsp</td></tr>");

  printf("\n");



  $row_ff = !$row_ff;
 } 

 echo('</table>');
}


 EndSection();
}



function DoModDocLinks()
{
 global $mod_list;
 
 BeginSection("Emulation Module Documentation", FALSE);

 $doctor_sort = array();

 foreach($mod_list as $name=>$mod_def)
  $doctor_sort[$mod_def['name']] = $mod_def['fullname'];

 asort($doctor_sort);

 foreach($doctor_sort as $name=>$fullname)
 {
  $mod_def = $mod_list[$name];

  if($name == "demo")
   continue;

  ExternalSection($mod_def['fullname'], $name . ".html", " [" . $name . "]");
 }

 EndSection();
}

?>

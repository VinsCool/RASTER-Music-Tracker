<html>
<head>
<title>RMT - RASTER MUSIC TRACKER - ATARI XE/XL</title>
<?

 $lang = "CZ"; //EN nebo CZ

?>
<? if ($lang=="CZ") echo '<meta http-equiv="Content-Type" content="text/html; charset=windows-1250">'."\n" ?>
<STYLE>
<!--
font.k
{
 background-color: #E0E0E0;
 white-space: nowrap;
}
-->
</STYLE>
</head>

<body BGCOLOR="#FFFFFF">

<p align="center"><img border="0" src="rmt.gif" width="325" height="51" alt="RMT - RASTER MUSIC TRACKER"></p>
<?
 function ht($str)
 {
  return htmlspecialchars(trim($str));
 }

 function Table($n)
 {
  echo "<table border=0 cellspacing=4>";
  return $n;
 }

 function htkey($str)
 {
    if ($str=="") return;
    $ks = explode(",",$str);
    $r="";
    reset($ks);
    do
    {
      $a = trim(current($ks));
      //if ($a!="") $r.= ", <font style=\"background-color: #E0E0E0\">$a</font>";
      if ($a!="") $r.= ", <nobr><font class=k>&nbsp;$a&nbsp;</font></nobr>";
    } while (next($ks));
    return substr($r,2); //bez carky na zacatku
 }

 $fo1 = fopen("rmt_manual.txt", "r");     //otevre zdroj
 $table=0;
 $lastKOD="";

 fgets($fo1,10000);                       //zahodi prvni radek

 while(!feof($fo1))
 {
  $line=explode("\t",fgets($fo1,10000));      //radek
  $pKOD = trim($line[0]);

  if ($pKOD=="") continue;

  $pSHIFT = ht($line[1]);
  $pCONTROL = ht($line[2]);
  $pKEY = ht($line[3]);
  $pDESC = ($lang=="CZ")? ht($line[4]) : ht($line[5]);       //cesky nebo anglicky popis

  if ($pKOD!=$lastKOD && $table)
  {
   echo "</table>";
   $table=0;
  }
  $lastKOD = $pKOD;

  switch($pKOD)
  {
  case "TITLE":
  echo $pDESC;
  break;

  case "TXT":
  echo $pDESC."<br>";
  break;

  case "H3":
  echo "<hr>";
  echo "<h3>$pDESC</h3>";
  break;

  case "H4":
  echo "<h4>$pDESC</h4>";
  break;

  case "KEY":
  if (!$table=="KEY") $table = Table("KEY");
  echo "<tr>";
  if ($pSHIFT && $pCONTROL)
     echo "<td valign=top>".htkey($pSHIFT)."<td valign=top>".htkey($pCONTROL);
  else
     echo "<td colspan=2 valign=top align=right>".htkey($pSHIFT).htkey($pCONTROL);

  echo "<td valign=top width=\"50\">";
  echo htkey($pKEY);
  echo "<td valign=top width=\"*\">$pDESC";
  break;

  case "PRE":
  echo "<PRE>$pKEY</PRE>";
  break;

  case "PAR":
  if (!$table=="PAR") $table = Table("PAR");
  echo "<tr><td valign=top width=150>$pKEY<td valign=top>$pDESC";
  break;
  }

  echo "\n";
 }

 fclose($fo1);

 //$fo2 = fopen("rmt_manual_$lang.htm", "wt");    //cilovy soubor
 //fputs($fo2,$out);
 //fclose($fo2);

?>
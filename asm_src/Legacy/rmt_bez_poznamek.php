<?
        $fin=fopen("rmt.a65","r");
        $fou=fopen("rmtplayr.a65","w");
        $eol=chr(13).chr(10);
        while(!feof($fin))
        {
                $line=chop(fgets($fin,1024));
                $a=substr($out,0,1);
                if ($line=="") continue;
                $pos = strpos($line,";");
                if (is_integer($pos))
                {
                 $pos2 = strpos($line,";*");
                 if (is_integer($pos2))
                 {
                  fwrite($fou,$line.$eol);
                 }
                 else
                 {
                  $out = chop(substr($line,0,$pos));
                  if ($out=="") continue;
                  fwrite($fou,$out.$eol);
                 }
                }
                else
                {
                 fwrite($fou,$line.$eol);
                }
        }
        fclose($fou);
        fclose($fin);
?>
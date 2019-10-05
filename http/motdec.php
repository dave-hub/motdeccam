<?php
    if ($_GET["get"] == "img") {
        $pic = "/tmp/motdecimg.bmp";
    
        //set header values for mime type and no cache
        header("Content-Type: image/bmp");
        header("Content-Length: ".filesize($pic));
        header("Cache-Control: no-cache, no-store, max-age=0, must-revalidate");
        header("Pragma: no-cache");
        
        //Read the image and send it directly to the output.
        echo readfile($pic);
    } else if ($_GET["get"] == "running") {
        //path to info file
        $inf = "/tmp/motdec.info";

        //send motdec.info as
        header("Content-Type: text/xml");
        echo "<?xml version='1.0'?>";

        //read file to output
        if (filesize($inf) > 0) {
            $xml = simplexml_load_file($inf);
            echo "<info><running>" . $xml->running . "</running></info>";

        //otherwise send default
        } else {
            echo "<info><running>false</running></info>";
        }
    } else if ($_GET["get"] == "log") {
        //load info to get logs path
        $xml = simplexml_load_file("/tmp/motdec.info");
        $i = 0;
        header("Content-Type: text/plain");

        //check if index specified
        if (isset($_GET["index"])) {
            $i = intval($_GET["index"]);
        }

        //open file
        $logfile = fopen($xml->logfile, "r");

        //skip to index
        for ($j = 0; $j < $i; ++$j) {
            fgets($logfile);            
        }

        //read out rest of file to output
        while(!feof($logfile)) {
            $line = fgets($logfile);
            if ($line[0] != '\n') {
                echo $line;                
            } else {
                break;                
            }
        }

        fclose($logfile);
    } else if ($_GET["get"] == "eventdates") {
        //load info to get logs path
        $xml = simplexml_load_file("/tmp/motdec.info");
        
        //get dated directories in logs
        $dirs = array_filter(glob($xml->logsdir . "*"), 'is_dir');

        //return list of dates of events separated by newlines
        header("Content-Type: text/plain");
        foreach ($dirs as $value) {
            echo basename($value) . "\n";
        }
    } else if ($_GET["get"] == "eventtimes") {
        if (isset($_GET["date"])) {
            //load info to get logs path
            $xml = simplexml_load_file("/tmp/motdec.info");

            //get dated directories in logs
            $dirs = array_filter(glob($xml->logsdir . basename($_GET["date"]) . "/*"), 'is_dir');

            //return list of times of events separated by newlines
            header("Content-Type: text/plain");
            foreach ($dirs as $value) {
                echo basename($value) . "\n";
            }
        }
    } else if ($_GET["get"] == "event") {
        if (isset($_GET["date"]) &&
            isset($_GET["time"]) &&
            isset($_GET["img"])) {
            //load info to get logs path
            $xml = simplexml_load_file("/tmp/motdec.info");

            $imgpath = $xml->logsdir . basename($_GET["date"]) . "/" . basename($_GET["time"]) . "/";
            
            //check if image specified
            if ($_GET["img"] == "background") {
                $imgpath .= "bg.bmp";
            } else if ($_GET["img"] == "change") {
                $imgpath .= "change.bmp";
            } else if ($_GET["img"] == "difference") {
                $imgpath .= "diff.bmp";
            } else if ($_GET["img"] == "segmap") {
                $imgpath .= "segmap.bmp";
            } else {
                $imgpath .= "change.bmp";
            }

            //return image
            header("Content-Type: image/bmp");
            header("Content-Length: ".filesize($imgpath));
            header("Cache-Control: no-cache, no-store, max-age=0, must-revalidate");
            header("Pragma: no-cache");
            
            //Read the image and send it directly to the output.
            echo readfile($imgpath);
        }
    }
?>

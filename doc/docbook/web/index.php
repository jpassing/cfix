<?php
/*
$i=0;
do {
require ('xml/RSS.php');
$feed = 'http://sourceforge.net/export/rss2_projnews.php?group_id=218233&rss_fulltext=1';
$rss =& new XML_RSS ($feed);
$rss->parse();
} while( count( $rss->getItems() ) == 0 );
$items = $rss->getItems();

header( "Last-Modified: " . $items[ 0 ][ "pubdate" ] );

$ver = "0.4";
*/
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
	<head>
		<title>cfix - Unit testing for C and C++ on Win32</title>
		<link rel="stylesheet" type="text/css" href="styleweb.css" />
		<meta name="description" content="cfix is an xUnit testing framework for C/C++, specialized for Win32 (32/64 bit), written by Johannes Passing." />
	    <meta name="keywords" content="cfix, unit testing, c, c++, win32, windows, native code" />
		<link rel="alternate" type="application/rss+xml" title="RSS 2.0" href="http://sourceforge.net/export/rss2_projnews.php?group_id=218233&rss_fulltext=1" />
	</head>
	<body>
	<div id='head'>
			<img src='header.jpg' id='logo' alt='Cfix - Unit testing for Win32' />
		</div>
		<div id='subhead'></div>
		<div id='main'>
			<div id='content'>
				<br />
				<span class='heading'>CFIX AT A GLANCE</span>
				<hr />
				
				<h2>cfix &#151; unit testing for C and C++ on Win32</h2>
				<p>
				<img src='cfix-tick.png' align='left' />
				cfix is an xUnit testing framework for C/C++, specialized for Windows development (32/64 bit). cfix supports 
				development of <b>both user and kernel mode</b> unit tests.
				</p>
				<p>
				cifx unit tests are compiled and linked into a DLL. The testrunner application provided by cfix allows selectively running tests of one or more of 
				such test-DLLs. Execution and behaviour in case of failing testcases can be highly customized. Moreover, cfix has been designed 
				to work well in conjunction with the Windows Debuggers (Visual Studio, WinDBG). 
				</p>
				
				<br /><br />
				
				<span class='heading'>FIRST STEPS</span>
				<hr />
				
				<table width='100%'>
				<tr><td width='49%' valign='top'>
					<img src='download.png' align='left' vspace='5' hspace='5'/><a href='http://sourceforge.net/project/showfiles.php?group_id=218233&package_id=263204'>Download the current release of cfix.</a>
					The release contains both 32 bit (i386) and 64 bit (amd64) binaries. 
					cfix runs on Windows 2000, Windows XP, Server 2003, Vista and Server 2008.
					
				</td>
				<td width='2%'>
				</td>
				<td width='49%' valign='top'>
					<img src='help.png' align='left' vspace='5' hspace='5'/><a href='doc/ch02.html'/>Read the tutorial</a> to see how to properly configure Visual Studio and
					how to create and run a test suite. All documentation is also contained in the download package.
				</td></tr>
				</table>
				
				
				<br /><br />
				
				<table width='100%'>
				<tr>
				<tr><td width='49%' valign='top'>
					<a href='http://sourceforge.net/export/rss2_projnews.php?group_id=218233&rss_fulltext=1'><img src='feed.png' align='right' border='0' /></a>
					<span class='heading'>NEWS</span>
					<hr />
				
<?php
/*
        foreach ($rss->getItems() as $item) {
			preg_match( '/(.*?)@(.*?) (.*)/', $item['author'], $m );
			$author = $m[ 1 ] . " at " . str_replace( ".", " ", $m[ 2 ] ) . " " . $m[ 3 ];
*/
?>
					<h3 class='rss'><?= $item['title'] ?></h3>
					<p>
						<?= $item['description'] ?>
					</p>
					<p class='rssfooter'><?=$author?>, <?= date( "Y-m-d", strtotime( $item['pubdate'] ) ) ?></p>
<?php
//        }
?>
	
				</td></tr>
				</table>
				
				<div id='cfixfooter'>
					(C) 2008 Johannes Passing
				</div>
				
				<div class='footer'>
					<a href="http://sourceforge.net"><img src="http://sflogo.sourceforge.net/sflogo.php?group_id=218233&amp;type=2" width="125" height="37" border="0" alt="SourceForge.net Logo" /></a>
					<br />Feedback? Send to passing at users.sourceforge.net.
				</div>
			</div>
			<div id='menu'>
				<div id='submenu'>
					<div id='submenuheader'>
						Main
					</div>
					<div id='submenusep'></div>
					<ul>
						<li><a href='#'>Front Page</a></li>
						<li><a href='http://sourceforge.net/projects/cfix/'>Project Page</a></li>
					</ul>
				</div>
				<div id='submenu'>
					<div id='submenuheader'>
						Download cfix
					</div>
					<div id='submenusep'></div>
					<ul>
						<li><a href='http://sourceforge.net/project/showfiles.php?group_id=218233&package_id=263204'>Download</a></li>
					</ul>
				</div>
				<div id='submenu'>
					<div id='submenuheader'>
						Documentation				
					</div>
					<div id='submenusep'></div>
					<ul>
						<li><a href='doc/index.html'>Table of Contents</a></li>
						<li><a href='doc/Background.html'>Background</a></li>
						<li><a href='doc/Usage.html'>Usage</a></li>
						<li><a href='doc/TutorialUserVs.html'>Tutorial (User Mode)</a></li>
						<li><a href='doc/TutorialKernelWdk.html'>Tutorial (Kernel Mode)</a></li>
						<li><a href='doc/API.html'>API Reference</a></li>
					</ul>   
				</div>
				<div id='submenu'>
					<div id='submenuheader'>
						Support					
					</div>
					<div id='submenusep'></div>
					<ul>
						<li><a href='https://sourceforge.net/tracker/?func=browse&amp;group_id=218233&amp;atid=1043037'>Bug Database</a></li>
						<li><a href='http://jpassing.wordpress.com/'>Blog</a></li>
					</ul>   
				</div>
				
				<div id='submenu'>
					<div id='submenusep'></div>
					<div style='text-align: center'>
						<a href='http://www.gnu.org/licenses/lgpl.html'><img src='lgpl.png' hspace='7' vspace='7' border='0' /></a>
					</div>
				</div>
			</div>
		</div>
		
    </body>
</html>

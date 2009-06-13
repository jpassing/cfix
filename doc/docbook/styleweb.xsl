<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  <xsl:import href="D:/prog/docbook-xsl-1.73.2/html/chunk.xsl"/>
  <xsl:param name="admon.graphics" select="1"/>
  <xsl:param name="html.stylesheet" select="'../styleweb.css'"/>
  <xsl:param name="chunk.section.depth" select="4"></xsl:param>
  <xsl:param name="chunk.first.sections" select="1"></xsl:param>
  <xsl:param name="use.id.as.filename" select="1"></xsl:param>
  <xsl:param name="highlight.source" select="0"></xsl:param>
  <xsl:param name="toc.section.depth" select="4"></xsl:param>
  
  <xsl:template name="user.header.navigation">
  </xsl:template>
  <xsl:template name="user.footer.navigation">
	<div id='cfixfooter'>
		<hr />
		cfix &#151; C and C++ Unit Testing for Win32 and the NT Kernel<br />
		Build <xsl:value-of select="$buildnumber" /><br />
		(C) 2008 Johannes Passing
    </div>
  </xsl:template>
  
  
  <xsl:template name="chunk-element-content">
  <xsl:param name="prev"/>
  <xsl:param name="next"/>
  <xsl:param name="nav.context"/>
  <xsl:param name="content">
    <xsl:apply-imports/>
  </xsl:param>

  <xsl:call-template name="user.preroot"/>

  <html>
    <xsl:call-template name="html.head">
      <xsl:with-param name="prev" select="$prev"/>
      <xsl:with-param name="next" select="$next"/>
    </xsl:call-template>

    <body>
	<div id='head'>
			<img src='header.jpg' id='logo' alt='cfix -- C and C++ Unit Testing for Win32 and the NT Kernel' />
		</div>
		<div id='subhead'></div>
		<div id='main'>
			<div id='content'>
	
      <xsl:call-template name="body.attributes"/>
      <xsl:call-template name="user.header.navigation"/>

      <xsl:call-template name="header.navigation">
        <xsl:with-param name="prev" select="$prev"/>
        <xsl:with-param name="next" select="$next"/>
        <xsl:with-param name="nav.context" select="$nav.context"/>
      </xsl:call-template>

      <xsl:call-template name="user.header.content"/>

      <xsl:copy-of select="$content"/>

      <xsl:call-template name="user.footer.content"/>

      <xsl:call-template name="footer.navigation">
        <xsl:with-param name="prev" select="$prev"/>
        <xsl:with-param name="next" select="$next"/>
        <xsl:with-param name="nav.context" select="$nav.context"/>
      </xsl:call-template>

      <xsl:call-template name="user.footer.navigation"/>
	  
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
						<li><a href='../'>Front Page</a></li>
						<li><a href='http://sourceforge.net/projects/cfix/'>Project Page</a></li>
					</ul>
				</div>
				<div id='submenu'>
					<div id='submenuheader'>
						Download cfix
					</div>
					<div id='submenusep'></div>
					<ul>
						<li><a href='http://sourceforge.net/project/showfiles.php?group_id=218233&amp;package_id=263204'>Download</a></li>
					</ul>
				</div>
				<div id='submenu'>
					<div id='submenuheader'>
						Documentation						
					</div>
					<div id='submenusep'></div>
					<ul>
						<li><a href='index.html'>Table of Contents</a></li>
						<li><a href='Background.html'>Background</a></li>
						<li><a href='Usage.html'>Usage</a></li>
						<li><a href='TutorialUserVsCc.html'>Tutorial (User Mode, C++)</a></li>
						<li><a href='TutorialUserVs.html'>Tutorial (User Mode, C)</a></li>
						<li><a href='TutorialKernelWdk.html'>Tutorial (Kernel Mode, C)</a></li>
						<li><a href='API.html'>API Reference</a></li>
						<li><a href='WinUnitAPI.html'>WinUnit Compatibility</a></li>
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
  <xsl:value-of select="$chunk.append"/>
</xsl:template>

  
</xsl:stylesheet>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  <xsl:import href="D:/prog/docbook-xsl-1.73.2/html/chunk.xsl"/>
  <xsl:param name="admon.graphics" select="1"/>
  <xsl:param name="html.stylesheet" select="'../styleweb.css'"/>
  <xsl:template name="user.header.navigation">
  </xsl:template>
  <xsl:template name="user.footer.navigation">
	<div id='cfixfooter'>
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
			<img src='header.png' id='logo' alt='Cfix - Unit testing for Win32' />
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
	  
	  </div>
			<div id='menu'>
				<div id='submenu'>
					<div id='submenuheader'>
						Main
					</div>
					<div id='submenusep'></div>
					<ul>
						<li><a href='../index.html'>Front Page</a></li>
					</ul>
				</div>
				<div id='submenu'>
					<div id='submenuheader'>
						Download cfix
					</div>
					<div id='submenusep'></div>
					<ul>
						<li><a href='#'>Binaries (Zip, 32 and 64 bit)</a></li>
						<li><a href='#'>Source (Zip)</a></li>
					</ul>
				</div>
				<div id='submenu'>
					<div id='submenuheader'>
						About cfix						
					</div>
					<div id='submenusep'></div>
					<ul>
						<li><a href='ch01.html'>Documentation</a></li>
						<li><a href='ch02.html'>Tutorial</a></li>
						<li><a href='ch03.html'>API</a></li>
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
						<a href='http://www.gnu.org/licenses/gpl.html'><img src='gpl.png' hspace='5' vspace='5' border='0' /></a>
					</div>
				</div>
			</div>
		</div>
		
    </body>
  </html>
  <xsl:value-of select="$chunk.append"/>
</xsl:template>

  
</xsl:stylesheet>
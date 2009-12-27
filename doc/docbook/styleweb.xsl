<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  <xsl:import href="D:/prog/docbook-xsl-1.75.2/html/chunk.xsl"/>
  <xsl:param name="admon.graphics" select="1"/>
  <xsl:param name="html.stylesheet" select="'../styleweb.css'"/>
  <xsl:param name="chunk.section.depth" select="4"></xsl:param>
  <xsl:param name="chunk.first.sections" select="1"></xsl:param>
  <xsl:param name="suppress.header.navigation" select="1"></xsl:param>
  <xsl:param name="use.id.as.filename" select="1"></xsl:param>
  <xsl:param name="highlight.source" select="0"></xsl:param>
  <xsl:param name="toc.section.depth" select="4"></xsl:param>
  <xsl:param name="chunker.output.doctype-public">-//W3C//DTD XHTML 1.0 Transitional//EN</xsl:param>
  <xsl:param name="generate.toc">
	appendix  toc,title
	article/appendix  nop
	article   toc,title
	book      toc,title
	chapter   toc,title
	part      toc,title
	preface   toc,title
	qandadiv  toc
	qandaset  toc
	reference toc,title
	sect1     toc,title
	sect2     toc,title
	sect3     toc,title
	sect4     toc,title
	sect5     toc,title
	section   toc,title
	set       toc,title
  </xsl:param>
  <xsl:param name="generate.section.toc.level" select="4"></xsl:param>
  
  <xsl:template name="user.header.navigation">
  </xsl:template>
  <xsl:template name="user.footer.navigation">
  </xsl:template>
  
  <xsl:template name="user.head.content">
	<!-- Mind the whitespace! -->
	<xsl:comment>[if lt IE 7]&gt;
		&lt;link rel="stylesheet" href="../assets/master-ie6patch.css" type="text/css" /&gt;
	&lt;![endif]</xsl:comment>
  </xsl:template>
  
  <xsl:template name="head.content">
	  <xsl:param name="node" select="."/>
	  <xsl:param name="title">
		<xsl:apply-templates select="$node" mode="object.title.markup.textonly"/>
	  </xsl:param>

	  <title>
		<xsl:copy-of select="$title"/> &#151; [cfix Testing Framework]
	  </title>

	  <meta name="description" content="{$title}: cfix &#151; C and C++ Unit Testing Framework for Windows" />
	  <meta name="keywords" lang="en" content="unit testing framework, c, c++, windows, win32, nt, native, kernel, test" />
	
	  <xsl:if test="$html.stylesheet != ''">
		<xsl:call-template name="output.html.stylesheets">
		  <xsl:with-param name="stylesheets" select="normalize-space($html.stylesheet)"/>
		</xsl:call-template>
	  </xsl:if>
  </xsl:template>
  
  <xsl:template name="chunk-element-content">
	  <xsl:param name="prev"/>
	  <xsl:param name="next"/>
	  <xsl:param name="nav.context"/>
	  <xsl:param name="content">
		<xsl:apply-imports/>
	  </xsl:param>
	  <xsl:param name="node" select="."/>
	  <xsl:param name="title">
		<xsl:apply-templates select="$node" mode="object.title.markup.textonly"/>
	  </xsl:param>

	  <xsl:call-template name="user.preroot"/>

	  <html>
	  
    <xsl:call-template name="html.head">
      <xsl:with-param name="prev" select="$prev"/>
      <xsl:with-param name="next" select="$next"/>
    </xsl:call-template>

	<body>
		<div id='tab'>
			<div class='tab_passive'><a href='http://www.visualassert.com/'>Visual Assert &#x2013; The Unit Testing Add-In for Visual C++</a></div>
			<div class='tab_passive2active'><img src='../assets/img/tab_passive2active.png' alt=''/></div>
			<div class='tab_active'><a href='../'>cfix &#x2013;  C/C++ Unit Testing for Win32 and NT</a></div>
			<div class='tab_activeend'><img src='../assets/img/tab_activeend.png' alt=''/></div>
			<div class='tab_pad'>&#xA0;</div>
			<div class='tab_clear'></div>
	    </div>
		<div id='header'>
			<img src='../assets/img/logo-cfix.gif' alt='cfix &#151; C and C++ Unit Testing Framework for Win32 and the NT Kernel' style="margin-left: 10px"/>
	    </div>
	    <div id='menu'>
	        <div id='menu_box'>
		        <ul id='mainmenu'>
		            <li><a href='../'>Home</a></li>
					<li><a href='index.html'>Documentation</a></li>
					<li><a href='http://sourceforge.net/projects/cfix/'>Project Page</a></li>
					<li><a href='http://sourceforge.net/project/showfiles.php?group_id=218233&amp;package_id=263204'>Download</a></li>
					<li><a href='https://sourceforge.net/tracker/?func=browse&amp;group_id=218233&amp;atid=1043037'>Bug Tracker</a></li>
		        </ul>
		    </div>
	    </div>
	    <div id='main'>
	        <div id='main_sidebar'>
			<!--
				<div class='download'>
				    <div style='text-align: center' class='submenu_content'>
						<a href='http://sourceforge.net/project/showfiles.php?group_id=218233&amp;package_id=263204'><img src='../assets/img/download.png' alt='Download cfix'/></a>
				    </div> 
			    </div>
			-->
	            <div class='submenu'>
				    <div class='submenu_box'>
					    Documentation				
				    </div>
				    <div class='submenu_content'>
				        <ul>
					        <li><a href='index.html'>Table of Contents</a></li>
					        <li><a href='Background.html'>Background</a></li>
					        <li><a href='Usage.html'>Usage</a></li>
					        <li><a href='TutorialUserVsCc.html'>Tutorial (User Mode, C++)</a></li>
					        <li><a href='TutorialUserVs.html'>Tutorial (User Mode, C)</a></li>
					        <li><a href='TutorialKernelWdk.html'>Tutorial (Kernel Mode, C)</a></li>
					        <li>
								<a href='API.html'>API Reference</a>
								<ul>
									<li><a href='WhatsNew.html'>What's new</a></li>
									<li><a href='TestAPI.html'>Base API (C/C++)</a></li>
									<li><a href='CcAPI.html'>C++ API</a></li>
									<li><a href='WinUnitAPI.html'>WinUnit Compatibility API</a></li>
								</ul>
							</li>
					        
				        </ul>  
				    </div> 
			    </div>
				
    			<div class='submenu'>
				    <div style='text-align: center' class='submenu_content'>
						<a href='http://www.gnu.org/licenses/lgpl.html'><img src='lgpl.png' hspace='7' vspace='7' border='0' alt='LGPL' /></a>
						<br /><br />
						<a href="http://sourceforge.net"><img src="http://sflogo.sourceforge.net/sflogo.php?group_id=218233&amp;type=2" width="125" height="37" border="0" alt="SourceForge.net Logo" /></a>
				    </div> 
			    </div>
	        </div>
    	    
			<div id='main_content'>
		
			<xsl:call-template name="body.attributes"/>
			<xsl:call-template name="user.header.navigation"/>

			<xsl:call-template name="header.navigation">
				<xsl:with-param name="prev" select="$prev"/>
				<xsl:with-param name="next" select="$next"/>
				<xsl:with-param name="nav.context" select="$nav.context"/>
			</xsl:call-template>

			<h1><xsl:copy-of select="$title"/></h1>
	
	
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
	        <div id='main_clear'></div>
    	</div>
	  
		<div id='footer'>
			cfix &#151; C and C++ Unit Testing Framework for Win32 and Windows NT Kernel Mode<br />
			Build <xsl:value-of select="$buildnumber" /><br />
			(C) 2009 <a href='http://jpassing.com/'>Johannes Passing</a>
			<br />
			<br />Feedback? Send to passing at users.sourceforge.net.
			<br />
		</div>
		
		<script type="text/javascript">
			var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www.");
			document.write(unescape("%3Cscript src='" + gaJsHost + "google-analytics.com/ga.js' type='text/javascript'%3E%3C/script%3E"));
		</script>
		<script type="text/javascript">
			try {
			var pageTracker = _gat._getTracker("UA-9197378-2");
			pageTracker._trackPageview();
			} catch(err) {}
		</script>
    </body>
  </html>
  <xsl:value-of select="$chunk.append"/>
</xsl:template>

  
</xsl:stylesheet>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  <xsl:import href="D:/prog/docbook-xsl-1.73.2/htmlhelp/htmlhelp.xsl"/>
  <xsl:param name="admon.graphics" select="1"/>
  <xsl:param name="html.stylesheet" select="'stylechm.css'"/>
  <xsl:param name="htmlhelp.chm" select="'cfix.chm'"/>
  <xsl:param name="htmlhelp.hhc.folders.instead.books" select="0"/>
  <xsl:template name="user.header.navigation">
    <div id='cfixheader'>
		cfix
    </div>
  </xsl:template>
  <xsl:template name="user.footer.navigation">
	<div id='cfixfooter'>
		(C) 2008 Johannes Passing
    </div>
  </xsl:template>
</xsl:stylesheet>
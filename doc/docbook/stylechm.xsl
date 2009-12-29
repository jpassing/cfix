<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  <xsl:import href="D:/prog/docbook-xsl-1.75.2/htmlhelp/htmlhelp.xsl"/>
  <xsl:param name="admon.graphics" select="1"/>
  <xsl:param name="html.stylesheet" select="'stylechm.css'"/>
  <xsl:param name="htmlhelp.chm" select="'cfix.chm'"/>
  <xsl:param name="htmlhelp.hhc.folders.instead.books" select="0"/>
  <xsl:param name="chunk.section.depth" select="4"></xsl:param>
  <xsl:param name="chunk.first.sections" select="1"></xsl:param>
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
	sect1     toc
	sect2     toc
	sect3     toc
	sect4     toc
	sect5     toc
	section   toc
	set       toc,title
  </xsl:param>
  
  <xsl:template name="user.header.navigation">
    <div id='cfixheader'>
		cfix
    </div>
  </xsl:template>
  <xsl:template name="user.footer.navigation">
	<hr />
	<div id='cfixfooter'>
		Build <xsl:value-of select="$buildnumber" /><br />
		(C) 2009 Johannes Passing<br />
		<br />
		Feedback? Send to passing at users.sourceforge.net.
    </div>
  </xsl:template>
</xsl:stylesheet>
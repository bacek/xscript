<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

	<xsl:output method="xml" encodind="utf-8"/>
	<xsl:strip-space elements="*"/>
	<xsl:param name="test"/>
	

	<xsl:template match="/">
		<xsl:apply-templates/>
	</xsl:template>

	<xsl:template match="/page/xscript"/>
	<xsl:template match="/page/item">
		<xsl:variable name="expected" select="value"/>
		<result>
		<xsl:choose>
			<xsl:when test="string($test) = string($expected)">
				<status>success</status>
			</xsl:when>
			<xsl:otherwise>
				<status>failed</status>
				<actual><xsl:value-of select="$test"/></actual>
				<expected><xsl:value-of select="$expected"/></expected>
			</xsl:otherwise>
		</xsl:choose>
		</result>
	</xsl:template>
</xsl:stylesheet>

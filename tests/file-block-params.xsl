<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

	<xsl:output method="xml" encodind="utf-8"/>
	<xsl:strip-space elements="*"/>
	<xsl:param name="test"/>
	

	<xsl:template match="/">
		<xsl:choose>
			<xsl:when test="string($test) = 'Hello, World!'">
				<status>success</status>
				<actual><xsl:value-of select="$test"/></actual>
			</xsl:when>
			<xsl:otherwise>
				<status>failed</status>
				<actual><xsl:value-of select="$test"/></actual>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>
</xsl:stylesheet>

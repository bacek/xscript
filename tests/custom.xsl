<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:output method="xml" encoding="utf-8" content-type="image/swg"/>
	<xsl:template match="/state">
		<state-results>
			<type><xsl:value-of select="@type"/></type>
			<name><xsl:value-of select="@name"/></name>
			<result><xsl:value-of select="."/></result>
		</state-results>
	</xsl:template>
</xsl:stylesheet>
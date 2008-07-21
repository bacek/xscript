<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
    xmlns:x="http://www.yandex.ru/xscript"
    extension-element-prefixes="x">
    
    <xsl:import href="x-nodeset.xsl"/>

    <xsl:template match="/tests/test">
        <xsl:variable name="nodeset" select="x:nl2br(source)"/>
        <xsl:apply-templates select="result" mode="node_set">
	    <xsl:with-param name="NodeSet" select="$nodeset"/>
	</xsl:apply-templates>
    </xsl:template>

</xsl:stylesheet>

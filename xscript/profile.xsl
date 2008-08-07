<?xml version="1.0"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

    <xsl:output method="text" omit-xml-declaration="yes"/>

    <xsl:template match="profile">
        <xsl:text>rank&#09;calls&#09;time&#09;average&#09;match&#09;name&#09;mode&#10;</xsl:text>
        <xsl:apply-templates select="template"/>
        <xsl:value-of select="concat('Total&#09;', sum(template/@calls), '&#09;', sum(template/@time), '&#09;')"/>
        <xsl:value-of select="concat(substring-after(total-time, &quot;&#39; &quot;), '&#10;')"/>
    </xsl:template>

    <xsl:template match="template">
        <xsl:apply-templates select="@rank | @calls | @time | @average"/>
        <xsl:apply-templates select="@match | @name | @mode"/>
        <xsl:text>&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="template/@*">
        <xsl:value-of select="."/>
        <xsl:text>&#09;</xsl:text>
    </xsl:template>

</xsl:stylesheet>

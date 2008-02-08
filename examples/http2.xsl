<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

    <xsl:output method="html" encoding="utf-8"/>

    <xsl:template match="doc">
        <html>
            <xsl:apply-templates select="page"/>
            <xsl:apply-templates select="current_date"/>
            <xsl:apply-templates select="mist_result"/>
        </html>
    </xsl:template>

    <xsl:template match="page">
        <title><xsl:value-of select="@title"/></title>
        <body>
            <xsl:apply-templates/>
        </body>
    </xsl:template>

    <xsl:template match="para">
        <p href="#{@id}">
            <xsl:apply-templates/>
        </p>
    </xsl:template>

    <xsl:template match="mist_result">
        <p>
            Loaded page: <xsl:value-of select=".//state"/>
        </p>
    </xsl:template>
    
    <xsl:template match="current_date">
        <p>
            Build date: <xsl:value-of select=".//state"/>
        </p>
    </xsl:template>
</xsl:stylesheet>

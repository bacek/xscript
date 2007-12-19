<?xml version="1.0"?>

<xsl:stylesheet
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    xmlns:x="http://www.yandex.ru/xscript"
    exclude-result-prefixes=" x exsl "
    extension-element-prefixes=" x "
    version="1.0">

<xsl:output method="xml" indent="yes"/>

<xsl:template match="page">
    <xsl:variable name="q-tmp">
        <x:mist>
            <method>set_state_string</method>
            <param type="String">a</param>
            <param type="String">1</param>
        </x:mist>
        <x:mist>
            <method>set_state_string</method>
            <param type="String">b</param>
            <param type="StateArg">a</param>
        </x:mist>
        <x:mist>
            <method>set_state_string</method>
            <param type="String">d</param>
            <param type="StateArg">c</param>
        </x:mist>
    </xsl:variable>
    <xsl:variable name="q" select="exsl:node-set($q-tmp)"/>
    <xml>
        <xsl:apply-templates select="$q" mode="copy"/>
    </xml>
</xsl:template>

<xsl:template match="*" mode="copy">
    <xsl:copy>
        <xsl:copy-of select="@*"/>
        <xsl:apply-templates mode="copy"/>
    </xsl:copy>
</xsl:template>

</xsl:stylesheet>

<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" xmlns:x="http://www.yandex.ru/xscript">
  <xsl:import href="x-tests.xsl"/>
  <xsl:template name="test-method">
    <xsl:variable name="pos" select="../../@pos"/>
    <xsl:variable name="length" select="../../@length"/>
    <xsl:value-of select="x:substring(., $pos, $length)"/>
  </xsl:template>
</xsl:stylesheet>
